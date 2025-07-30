#include "SKTFOrder.h"
#include "../json.h"
#include "../Utility.h"

CSKTFOrder::CSKTFOrder(const string& strServer) : CSKBaseGWOrder(strServer)
{
    m_tSKLoggerGW = {};
}

CSKTFOrder::~CSKTFOrder()
{
}

/// <summary>
/// 送出委託
/// </summary>
void CSKTFOrder::Run(const TSKTFOrder& tSKOrder, const struct timeval& tvStart, const string& strThreadID)
{
    try
    {
        m_tvStart = tvStart;
        time_t nowtime = tvStart.tv_sec;
        struct tm* nowtm = localtime(&nowtime);
        char tmbuf[64], buf[64];
        strftime(tmbuf, sizeof tmbuf, "%Y %m %d %H:%M:%S", nowtm);
        snprintf(buf, sizeof buf, "%s.%06ld", tmbuf, tvStart.tv_usec);
        m_strStartT = buf;

        // 先取得當前Log模式, 如果邊讀模式邊紀錄, 中途改變模式會有問題
        int nLogModel = g_nLogMode;

        // Debug Log
      /*  if (nLogModel == 0)
        {*/
            struct timeval tvEnd[6];
            long nTime[6] = { 0 };

            gettimeofday(&tvEnd[0], NULL); // Session Verify + SendReply(Pending Order) + EnQ + DeQ

            bool bSuccess = SetPostData(tSKOrder);
            if (bSuccess == false)
            {
                gettimeofday(&tvEnd[1], NULL); // SetPostData(false)

                SetReplyData();

                gettimeofday(&tvEnd[2], NULL); //SetReplyData

                nTime[0] = 1000000 * (tvEnd[0].tv_sec - tvStart.tv_sec) + (tvEnd[0].tv_usec - tvStart.tv_usec);
                nTime[1] = 1000000 * (tvEnd[1].tv_sec - tvEnd[0].tv_sec) + (tvEnd[1].tv_usec - tvEnd[0].tv_usec);
                nTime[2] = 1000000 * (tvEnd[2].tv_sec - tvEnd[1].tv_sec) + (tvEnd[2].tv_usec - tvEnd[1].tv_usec);
                m_strLogTime += to_string(nTime[0]) + "," + to_string(nTime[1]) + ",,," + to_string(nTime[2]) + "#,";

                m_tvSendT = tvEnd[2];

                SetLogger();

                SendClientCallback();
            }
            else
            {
                gettimeofday(&tvEnd[1], NULL); // SetPostData(true)

                nTime[0] = 1000000 * (tvEnd[0].tv_sec - tvStart.tv_sec) + (tvEnd[0].tv_usec - tvStart.tv_usec);
                nTime[1] = 1000000 * (tvEnd[1].tv_sec - tvEnd[0].tv_sec) + (tvEnd[1].tv_usec - tvEnd[0].tv_usec);

                if (nTime[0] >= g_bMaxWaitTime) //如果在queue裡待超過maxWaitTime則丟棄，回傳error
                {
                    m_strErrCode = g_strECExceedMaxTime;
                    m_strErrMsg = g_strEMExceedMaxTime;

                    SetReplyData();

                    gettimeofday(&tvEnd[2], NULL); //SetReplyData
                    nTime[2] = 1000000 * (tvEnd[2].tv_sec - tvEnd[1].tv_sec) + (tvEnd[2].tv_usec - tvEnd[1].tv_usec);
                    m_strLogTime += to_string(nTime[0]) + "," + to_string(nTime[1]) + ",,," + to_string(nTime[2]) + "#,";
                    m_tvSendT = tvEnd[2];
                    write_log_(e_ltServerData, "Order OverTime!", false);
                    g_b_mtxTimeOutOrderNumPer30s.lock();
                    g_bTimeOutOrderNumPer30s = g_bTimeOutOrderNumPer30s + 1;
                    g_b_mtxTimeOutOrderNumPer30s.unlock();
                }
                else
                {
                    string strServer = m_strServer + "?=" + m_strORKEY;
                    m_hRequest.Post(strServer, m_strPostJson, m_ssResponse, strThreadID);

                    gettimeofday(&tvEnd[2], NULL); // Post GW

                    nowtime = tvEnd[2].tv_sec;
                    nowtm = localtime(&nowtime);
                    strftime(tmbuf, sizeof tmbuf, "%Y-%m-%d %H:%M:%S", nowtm);
                    snprintf(buf, sizeof buf, "%s.%06ld", tmbuf, tvEnd[2].tv_usec);
                    string strAfterGWTime = buf;

                    ParseGWData(m_ssResponse.str());

                    gettimeofday(&tvEnd[3], NULL); // Parse GW Data

                    SetReplyData();

                    gettimeofday(&tvEnd[4], NULL); // Set Reply Data 

                    nowtime = tvEnd[4].tv_sec;
                    nowtm = localtime(&nowtime);
                    strftime(tmbuf, sizeof tmbuf, "%Y-%m-%d %H:%M:%S", nowtm);
                    snprintf(buf, sizeof buf, "%s.%06ld", tmbuf, tvEnd[4].tv_usec);
                    string strSendQTime = buf;

                    //nTime[0] = 1000000 * (tvEnd[0].tv_sec - tvStart.tv_sec) + (tvEnd[0].tv_usec - tvStart.tv_usec);
                    //nTime[1] = 1000000 * (tvEnd[1].tv_sec - tvEnd[0].tv_sec) + (tvEnd[1].tv_usec - tvEnd[0].tv_usec);
                    nTime[2] = 1000000 * (tvEnd[2].tv_sec - tvEnd[1].tv_sec) + (tvEnd[2].tv_usec - tvEnd[1].tv_usec);
                    nTime[3] = 1000000 * (tvEnd[3].tv_sec - tvEnd[2].tv_sec) + (tvEnd[3].tv_usec - tvEnd[2].tv_usec);
                    nTime[4] = 1000000 * (tvEnd[4].tv_sec - tvEnd[3].tv_sec) + (tvEnd[4].tv_usec - tvEnd[3].tv_usec);

                    m_tvSendT = tvEnd[4];

                    m_strLogTime += to_string(nTime[0]) + "," + to_string(nTime[1]) + "," + to_string(nTime[2])
                        + "," + to_string(nTime[3]) + "," + to_string(nTime[4]) + "#" + strAfterGWTime + "," + strSendQTime;
                }
                SetLogger();

                SendClientCallback();
            }
        //}
        //// Release Log
        //else
        //{
        //    struct timeval tvEnd;
        //    long nTime = 0;

        //    bool bSuccess = SetPostData(tSKOrder);
        //    if (bSuccess == false)
        //    {
        //        SetReplyData();
        //        SendClientCallback();

        //        gettimeofday(&tvEnd, NULL); // End
        //        nTime = 1000000 * (tvEnd.tv_sec - tvStart.tv_sec) + (tvEnd.tv_usec - tvStart.tv_usec);
        //        m_strLogTime += to_string(nTime);
        //    }
        //    else
        //    {
        //        m_hRequest.Post(m_strServer, m_strPostJson, m_ssResponse);
        //        ParseGWData(m_ssResponse.str());
        //        SetReplyData();
        //        SendClientCallback();

        //        gettimeofday(&tvEnd, NULL); // End
        //        nTime = 1000000 * (tvEnd.tv_sec - tvStart.tv_sec) + (tvEnd.tv_usec - tvStart.tv_usec);
        //        m_strLogTime += to_string(nTime);
        //    }
        //}
    }
    catch (...)
    {
        m_strErrCode = g_strECFatal;
        m_strErrMsg = "CSKTFOrder Run " + g_strEMFatal;
        SetReplyData();
        SetLogger();
        SendClientCallback();
    }
}

/// <summary>
/// 確認User委託資料
/// </summary>
bool CSKTFOrder::SetPostData(const TSKTFOrder& tSKOrder)
{
    try
    {
        m_strORKEY = CUtility::TrimCA(tSKOrder.caORKEY, sizeof(tSKOrder.caORKEY));
        if (m_strORKEY.empty())
            throw string("No ORKEY");

        string strMarketType = CUtility::TrimCA(tSKOrder.caMarketType, sizeof(tSKOrder.caMarketType));
        if (strMarketType.empty())
            throw string("No MarketType");
        else if (strMarketType != "TF" && strMarketType != "TO")
            throw string("MarketType Error");

        string strLoginID = CUtility::TrimCA(tSKOrder.caLoginID, sizeof(tSKOrder.caLoginID));
        if (strLoginID.empty())
            throw string("No LoginID");

        string strIDNO = CUtility::TrimCA(tSKOrder.caIDNO, sizeof(tSKOrder.caIDNO));
        if (strIDNO.empty())
            throw string("No IDNO");

        string strBrokerID = CUtility::TrimCA(tSKOrder.caBrokerID, sizeof(tSKOrder.caBrokerID));
        if (strBrokerID.empty())
            throw string("No BrokerID");

        string strIBNO = CUtility::TrimCA(tSKOrder.caIBNO, sizeof(tSKOrder.caIBNO));
        if (strIBNO.empty())
            throw string("No IBNO");

        string strACNO = CUtility::TrimCA(tSKOrder.caACNO, sizeof(tSKOrder.caACNO));
        if (strACNO.empty())
            throw string("No ACNO");

        //盤中單可帶空值
        string strPreOrder = CUtility::TrimCA(tSKOrder.caPreOrder, sizeof(tSKOrder.caPreOrder));
        //if (strPreOrder.empty())
        //    throw string("No PreOrder");

        string strORDERType = CUtility::TrimCA(tSKOrder.caORDERType, sizeof(tSKOrder.caORDERType));
        if (strORDERType.empty())
            throw string("No ORDERType");

        string strCOMID1 = CUtility::TrimCA(tSKOrder.caCOMID1, sizeof(tSKOrder.caCOMID1));
        if (strCOMID1.empty())
            throw string("No COMID1");

        string strCP1 = CUtility::TrimCA(tSKOrder.caCP1, sizeof(tSKOrder.caCP1));
        if (strMarketType == "TO" && strCP1.empty())
            throw string("No CP1");

        string strSettleYMD1 = CUtility::TrimCA(tSKOrder.caSettleYMD1, sizeof(tSKOrder.caSettleYMD1));
        if (strSettleYMD1.empty())
            throw string("No SettleYMD1");

        string strStrike1 = CUtility::TrimCA(tSKOrder.caStrike1, sizeof(tSKOrder.caStrike1));
        if (strMarketType == "TO" && strStrike1.empty())
            throw string("No Strike1");

        string strBS1 = CUtility::TrimCA(tSKOrder.caBS1, sizeof(tSKOrder.caBS1));
        if (strBS1.empty())
            throw string("No BS1");

        //非必帶
        string strCOMID2 = CUtility::TrimCA(tSKOrder.caCOMID2, sizeof(tSKOrder.caCOMID2));
        //if (strCOMID2.empty())
        //    throw string("No COMID2");

        //非必帶
        string strCP2 = CUtility::TrimCA(tSKOrder.caCP2, sizeof(tSKOrder.caCP2));
        //if (strCP2.empty())
        //    throw string("No CP2");

        //非必帶
        string strSettleYMD2 = CUtility::TrimCA(tSKOrder.caSettleYMD2, sizeof(tSKOrder.caSettleYMD2));
        //if (strSettleYMD2.empty())
        //    throw string("No SettleYMD2");

        //非必帶
        string strStrike2 = CUtility::TrimCA(tSKOrder.caStrike2, sizeof(tSKOrder.caStrike2));
        //if (strStrike2.empty())
        //    throw string("No Strike2");

        //非必帶
        string strBS2 = CUtility::TrimCA(tSKOrder.caBS2, sizeof(tSKOrder.caBS2));
        //if (strBS2.empty())
        //    throw string("No BS2");

        string strQty = CUtility::TrimCA(tSKOrder.caQty, sizeof(tSKOrder.caQty));
        if (strQty.empty())
            throw string("No Qty");

        string strPriceflag = CUtility::TrimCA(tSKOrder.caPriceflag, sizeof(tSKOrder.caPriceflag));
        if (strPriceflag.empty())
            throw string("No Priceflag");

        //改量、刪單、市價 可帶空值
        string strPrice = CUtility::TrimCA(tSKOrder.caPrice, sizeof(tSKOrder.caPrice));
        //if (strPrice.empty())
        //    throw string("No Price");

        //ROD 可帶空值
        string strCOND = CUtility::TrimCA(tSKOrder.caCOND, sizeof(tSKOrder.caCOND));
        //if (strCOND.empty())
        //    throw string("No COND");

        //非當沖可帶空值
        string strDayTrade = CUtility::TrimCA(tSKOrder.caDayTrade, sizeof(tSKOrder.caDayTrade));
        //if (strDayTrade.empty())
        //    throw string("No DayTrade");

        string strTranType = CUtility::TrimCA(tSKOrder.caTranType, sizeof(tSKOrder.caTranType));
        //if (strTranType.empty())
        //    throw string("No TranType");

        string strRawData = CUtility::TrimCA(tSKOrder.caRawData, sizeof(tSKOrder.caRawData));
        //if (strRawData.empty())
        //    throw string("No RawData");

        string strSign = CUtility::TrimCA(tSKOrder.caSign, sizeof(tSKOrder.caSign));
        //if (strSign.empty())
        //    throw string("No Sign");

        string strCN = CUtility::TrimCA(tSKOrder.caCN, sizeof(tSKOrder.caCN));
        //if (strCN.empty())
        //    throw string("No CN");

        string strCertID = CUtility::TrimCA(tSKOrder.caCertID, sizeof(tSKOrder.caCertID));
        //if (strCertID.empty())
        //    throw string("No CertID");

        string strCertType = CUtility::TrimCA(tSKOrder.caCertType, sizeof(tSKOrder.caCertType));
        //if (strCertType.empty())
        //    throw string("No CertType");

        string strSource = CUtility::TrimCA(tSKOrder.caSource, sizeof(tSKOrder.caSource));
        if (strSource.empty())
            throw string("No Source");

        string strDeviceID = CUtility::TrimCA(tSKOrder.caDeviceID, sizeof(tSKOrder.caDeviceID));
        if (strDeviceID.empty())
            throw string("No DeviceID");

        string strSellerNo = CUtility::TrimCA(tSKOrder.caSellerNo, sizeof(tSKOrder.caSellerNo));

        m_strPostJson += "{\"LoginID\":\"";
        m_strPostJson += strLoginID;
        m_strPostJson += "\",\"IDNO\":\"";
        m_strPostJson += strIDNO;
        m_strPostJson += "\",\"BrokerID\":\"";
        m_strPostJson += strBrokerID;
        m_strPostJson += "\",\"IBNO\":\"";
        m_strPostJson += strIBNO;
        m_strPostJson += "\",\"ACNO\":\"";
        m_strPostJson += strACNO;
        m_strPostJson += "\",\"PreOrder\":\"";
        m_strPostJson += strPreOrder;
        m_strPostJson += "\",\"OType\":\"";
        m_strPostJson += strORDERType;
        m_strPostJson += "\",\"COMID1\":\"";
        m_strPostJson += strCOMID1;
        m_strPostJson += "\",\"CP1\":\"";
        m_strPostJson += strCP1;
        m_strPostJson += "\",\"SettleYMD1\":\"";
        m_strPostJson += strSettleYMD1;
        m_strPostJson += "\",\"Strike1\":\"";
        m_strPostJson += strStrike1;
        m_strPostJson += "\",\"BS1\":\"";
        m_strPostJson += strBS1;
        m_strPostJson += "\",\"COMID2\":\"";
        m_strPostJson += strCOMID2;
        m_strPostJson += "\",\"CP2\":\"";
        m_strPostJson += strCP2;
        m_strPostJson += "\",\"SettleYMD2\":\"";
        m_strPostJson += strSettleYMD2;
        m_strPostJson += "\",\"Strike2\":\"";
        m_strPostJson += strStrike2;
        m_strPostJson += "\",\"BS2\":\"";
        m_strPostJson += strBS2;
        m_strPostJson += "\",\"Qty\":\"";
        m_strPostJson += strQty;
        m_strPostJson += "\",\"Priceflag\":\"";
        m_strPostJson += strPriceflag;
        m_strPostJson += "\",\"Price\":\"";
        m_strPostJson += strPrice;
        m_strPostJson += "\",\"COND\":\"";
        m_strPostJson += strCOND;
        m_strPostJson += "\",\"DayTrade\":\"";
        m_strPostJson += strDayTrade;
        m_strPostJson += "\",\"ORKEY\":\"";
        m_strPostJson += m_strORKEY;
        m_strPostJson += "\",\"TranType\":\"";
        m_strPostJson += strTranType;
        m_strPostJson += "\",\"RawData\":\"";
        m_strPostJson += strRawData;
        m_strPostJson += "\",\"Sign\":\"";
        m_strPostJson += strSign;
        m_strPostJson += "\",\"CN\":\"";
        m_strPostJson += strCN;
        m_strPostJson += "\",\"CertID\":\"";
        m_strPostJson += strCertID;
        m_strPostJson += "\",\"CertType\":\"";
        m_strPostJson += strCertType;
        m_strPostJson += "\",\"Source\":\"";
        m_strPostJson += strSource;
        m_strPostJson += "\",\"ClientIP\":\"";
        m_strPostJson += m_strIP;
        m_strPostJson += "\",\"TradeDeviceID\":\"";
        m_strPostJson += strDeviceID;
        m_strPostJson += "\",\"SellerNo\":\"";
        m_strPostJson += strSellerNo;
        m_strPostJson += "\"}";
        
        /*m_strLogMsg += "{LoginID:";
        m_strLogMsg += strLoginID;
        m_strLogMsg += ",IDNO:";
        m_strLogMsg += strIDNO;
        m_strLogMsg += ",BrokerID:";
        m_strLogMsg += strBrokerID;
        m_strLogMsg += ",IBNO:";
        m_strLogMsg += strIBNO;
        m_strLogMsg += ",ACNO:";
        m_strLogMsg += strACNO;
        m_strLogMsg += ",PreOrder:";
        m_strLogMsg += strPreOrder;
        m_strLogMsg += ",OType:";
        m_strLogMsg += strORDERType;
        m_strLogMsg += ",COMID1:";
        m_strLogMsg += strCOMID1;
        m_strLogMsg += ",CP1:";
        m_strLogMsg += strCP1;
        m_strLogMsg += ",SettleYMD1:";
        m_strLogMsg += strSettleYMD1;
        m_strLogMsg += ",Strike1:";
        m_strLogMsg += strStrike1;
        m_strLogMsg += ",BS1:";
        m_strLogMsg += strBS1;
        m_strLogMsg += ",COMID2:";
        m_strLogMsg += strCOMID2;
        m_strLogMsg += ",CP2:";
        m_strLogMsg += strCP2;
        m_strLogMsg += ",SettleYMD2:";
        m_strLogMsg += strSettleYMD2;
        m_strLogMsg += ",Strike2:";
        m_strLogMsg += strStrike2;
        m_strLogMsg += ",BS2:";
        m_strLogMsg += strBS2;
        m_strLogMsg += ",Qty:";
        m_strLogMsg += strQty;
        m_strLogMsg += ",Priceflag:";
        m_strLogMsg += strPriceflag;
        m_strLogMsg += ",Price:";
        m_strLogMsg += strPrice;
        m_strLogMsg += ",COND:";
        m_strLogMsg += strCOND;
        m_strLogMsg += ",DayTrade:";
        m_strLogMsg += strDayTrade;
        m_strLogMsg += ",ORKEY:";
        m_strLogMsg += m_strORKEY;
        m_strLogMsg += ",TranType:";
        m_strLogMsg += strTranType;
        m_strLogMsg += ",RawData:";
        m_strLogMsg += strRawData;
        m_strLogMsg += ",CN:";
        m_strLogMsg += strCN;
        m_strLogMsg += ",CertID:";
        m_strLogMsg += strCertID;
        m_strLogMsg += ",CertType:";
        m_strLogMsg += strCertType;
        m_strLogMsg += ",Source:";
        m_strLogMsg += strSource;
        m_strLogMsg += ",ClientIP:";
        m_strLogMsg += m_strIP;
        m_strLogMsg += ",TradeDeviceID:";
        m_strLogMsg += strDeviceID;
        m_strLogMsg += "}";*/

        /*m_strORKEY.append(tSKTFOrder.caORKEY, 100);
        CUtility::TrimByRef(m_strORKEY);
        if (m_strORKEY.empty())
            throw string("No ORKEY");

        string strMarketType(tSKTFOrder.caMarketType, 2);
        CUtility::TrimByRef(strMarketType);
        if (strMarketType.empty())
            throw string("No MarketType");
        else if(strMarketType != "TF" && strMarketType != "TO")
            throw string("MarketType Error");

        string strLoginID(tSKTFOrder.caLoginID, 10);
        CUtility::TrimByRef(strLoginID);
        if (strLoginID.empty())
            throw string("No LoginID");

        string strIDNO(tSKTFOrder.caIDNO, 10);
        CUtility::TrimByRef(strIDNO);
        if (strIDNO.empty())
            throw string("No IDNO");

        string strBrokerID(tSKTFOrder.caBrokerID, 7);
        CUtility::TrimByRef(strBrokerID);
        if (strBrokerID.empty())
            throw string("No BrokerID");

        string strIBNO(tSKTFOrder.caIBNO, 3);
        CUtility::TrimByRef(strIBNO);
        if (strIBNO.empty())
            throw string("No IBNO");

        string strACNO(tSKTFOrder.caACNO, 7);
        CUtility::TrimByRef(strACNO);
        if (strACNO.empty())
            throw string("No ACNO");

        //盤中單可帶空值
        string strPreOrder(tSKTFOrder.caPreOrder, 1);
        CUtility::TrimByRef(strPreOrder);
        //if (strPreOrder.empty())
        //    throw string("No PreOrder");

        string strORDERType(tSKTFOrder.caORDERType, 1);
        CUtility::TrimByRef(strORDERType);
        if (strORDERType.empty())
            throw string("No ORDERType");

        string strCOMID1(tSKTFOrder.caCOMID1, 7);
        CUtility::TrimByRef(strCOMID1);
        if (strCOMID1.empty())
            throw string("No COMID1");

        string strCP1(tSKTFOrder.caCP1, 1);
        CUtility::TrimByRef(strCP1);
        if (strMarketType == "TO" && strCP1.empty())
            throw string("No CP1");

        string strSettleYMD1(tSKTFOrder.caSettleYMD1, 6);
        CUtility::TrimByRef(strSettleYMD1);
        if (strSettleYMD1.empty())
            throw string("No SettleYMD1");

        string strStrike1(tSKTFOrder.caStrike1, 8);
        CUtility::TrimByRef(strStrike1);
        if (strMarketType == "TO" && strStrike1.empty())
            throw string("No Strike1");

        string strBS1(tSKTFOrder.caBS1, 1);
        CUtility::TrimByRef(strBS1);
        if (strBS1.empty())
            throw string("No BS1");

        //非必帶
        string strCOMID2(tSKTFOrder.caCOMID2, 7);
        CUtility::TrimByRef(strCOMID2);
        //if (strCOMID2.empty())
        //    throw string("No COMID2");

        //非必帶
        string strCP2(tSKTFOrder.caCP2, 1);
        CUtility::TrimByRef(strCP2);
        //if (strCP2.empty())
        //    throw string("No CP2");

        //非必帶
        string strSettleYMD2(tSKTFOrder.caSettleYMD2, 6);
        CUtility::TrimByRef(strSettleYMD2);
        //if (strSettleYMD2.empty())
        //    throw string("No SettleYMD2");

        //非必帶
        string strStrike2(tSKTFOrder.caStrike2, 8);
        CUtility::TrimByRef(strStrike2);
        //if (strStrike2.empty())
        //    throw string("No Strike2");

        //非必帶
        string strBS2(tSKTFOrder.caBS2, 1);
        CUtility::TrimByRef(strBS2);
        //if (strBS2.empty())
        //    throw string("No BS2");

        string strQty(tSKTFOrder.caQty, 8);
        CUtility::TrimByRef(strQty);
        if (strQty.empty())
            throw string("No Qty");

        string strPriceflag(tSKTFOrder.caPriceflag, 1);
        CUtility::TrimByRef(strPriceflag);
        if (strPriceflag.empty())
            throw string("No Priceflag");

        //改量、刪單、市價 可帶空值
        string strPrice(tSKTFOrder.caPrice, 8);
        CUtility::TrimByRef(strPrice);
        //if (strPrice.empty())
        //    throw string("No Price");

        //ROD 可帶空值
        string strCOND(tSKTFOrder.caCOND, 1);
        CUtility::TrimByRef(strCOND);
        //if (strCOND.empty())
        //    throw string("No COND");

        //非當沖可帶空值
        string strDayTrade(tSKTFOrder.caDayTrade, 1);
        CUtility::TrimByRef(strDayTrade);
        //if (strDayTrade.empty())
        //    throw string("No DayTrade");

        string strTranType(tSKTFOrder.caTranType, 2);
        CUtility::TrimByRef(strTranType);
        if (strTranType.empty())
            throw string("No TranType");

        string strRawData(tSKTFOrder.caRawData, 150);
        CUtility::TrimByRef(strRawData);
        if (strRawData.empty())
            throw string("No RawData");

        string strSign(tSKTFOrder.caSign, 2700);
        CUtility::TrimByRef(strSign);
        if (strSign.empty())
            throw string("No Sign");

        string strCN(tSKTFOrder.caCN, 30);
        CUtility::TrimByRef(strCN);
        if (strCN.empty())
            throw string("No CN");

        string strCertID(tSKTFOrder.caCertID, 8);
        CUtility::TrimByRef(strCertID);
        if (strCertID.empty())
            throw string("No CertID");

        string strCertType(tSKTFOrder.caCertType, 1);
        CUtility::TrimByRef(strCertType);
        if (strCertType.empty())
            throw string("No CertType");

        string strSource(tSKTFOrder.caSource, 2);
        CUtility::TrimByRef(strSource);
        if (strSource.empty())
            throw string("No Source");

        string strDeviceID(tSKTFOrder.caDeviceID, 50);
        CUtility::TrimByRef(strDeviceID);
        if (strDeviceID.empty())
            throw string("No DeviceID");*/

        /*char caBuf[4096];
        sprintf(caBuf, "{\"LoginID\":\"%s\",\"IDNO\":\"%s\",\"BrokerID\":\"%s\",\"IBNO\":\"%s\",\"ACNO\":\"%s\""
            ",\"PreOrder\":\"%s\",\"OType\":\"%s\",\"COMID1\":\"%s\",\"CP1\":\"%s\",\"SettleYMD1\":\"%s\""
            ",\"Strike1\":\"%s\",\"BS1\":\"%s\",\"COMID2\":\"%s\",\"CP2\":\"%s\",\"SettleYMD2\":\"%s\""
            ",\"Strike2\":\"%s\",\"BS2\":\"%s\",\"Qty\":\"%s\",\"Priceflag\":\"%s\",\"Price\":\"%s\""
            ",\"COND\":\"%s\",\"DayTrade\":\"%s\",\"ORKEY\":\"%s\",\"TranType\":\"%s\",\"RawData\":\"%s\""
            ",\"Sign\":\"%s\",\"CN\":\"%s\",\"CertID\":\"%s\",\"CertType\":\"%s\",\"Source\":\"%s\""
            ",\"ClientIP\":\"%s\",\"TradeDeviceID\":\"%s\"}"
            , strLoginID.c_str()
            , strIDNO.c_str()
            , strBrokerID.c_str()
            , strIBNO.c_str()
            , strACNO.c_str()
            , strPreOrder.c_str()
            , strORDERType.c_str()
            , strCOMID1.c_str()
            , strCP1.c_str()
            , strSettleYMD1.c_str()
            , strStrike1.c_str()
            , strBS1.c_str()
            , strCOMID2.c_str()
            , strCP2.c_str()
            , strSettleYMD2.c_str()
            , strStrike2.c_str()
            , strBS2.c_str()
            , strQty.c_str()
            , strPriceflag.c_str()
            , strPrice.c_str()
            , strCOND.c_str()
            , strDayTrade.c_str()
            , m_strORKEY.c_str()
            , strTranType.c_str()
            , strRawData.c_str()
            , strSign.c_str()
            , strCN.c_str()
            , strCertID.c_str()
            , strCertType.c_str()
            , strSource.c_str()
            , m_strIP.c_str()
            , strDeviceID.c_str());
        m_strPostJson = string(caBuf);*/

        return true;
    }
    catch (string strMsg)
    {
        m_strErrCode = g_strECUserData;
        m_strErrMsg = "CSKTFOrder SetPostData " + strMsg;
    }
    catch (...)
    {
        m_strErrCode = g_strECFatal;
        m_strErrMsg = "CSKTFOrder SetPostData " + g_strEMFatal;
    }

    return false;
}

/// <summary>
/// 確認回傳資料
/// </summary>
void CSKTFOrder::ParseGWData(const string& strData)
{
    web::json::Parser parser;
    web::json::JsonObject::iterator iter1st;
    web::json::Node* __Node = NULL;

    try
    {
        /*
        {
            "Code": "000",
            "Message": "委託書:o0-001 賣出FITX   201712       新倉  30口    委價 10600.000",
            "datas": {
                "BrokerID": "F020000",
                "IBNO": "000",
                "ACNO": "8931353",
                "TDate": "20200918",
                "OrderNO": "o0001",
                "SeqNo": "2300000358711"
            }
        }
        */

        // Parse Json
        if (!parser.Parse(strData, &__Node))
        {

            if (strData.find("!DOCTYPE") != string::npos && strData.find("html") != string::npos)
            {
                write_log_(e_ltServerError, "(TFOrder)連不上srvgw或是proxy server呼叫的url有誤", false);
                throw g_strEMGWConnFail;
            }
            else
            {
                write_log_(e_ltServerError, "receive Data from srvgw(TFOrder) = " + strData, false);
                throw g_strEMParseJson;
            }
        }

        // Code
        iter1st = __Node->GetObj()->find("Code");
        if (iter1st != __Node->GetObj()->end())
        {
            if (iter1st->second->GetVlaueType() == web::json::kString)
            {
                m_strErrCode =*(iter1st->second->GetString());
                CUtility::TrimByRef(m_strErrCode);
            }
            else
                m_strErrCode = "";
        }

        // Message
        iter1st = __Node->GetObj()->find("Message");
        if (iter1st != __Node->GetObj()->end())
        {
            if (iter1st->second->GetVlaueType() == web::json::kString)
            {
                m_strErrMsg = *(iter1st->second->GetString());
                CUtility::TrimByRef(m_strErrMsg);
            }
            else
                m_strErrMsg = "";
        }

        // datas
        iter1st = __Node->GetObj()->find("datas");
        if (iter1st != __Node->GetObj()->end())
        {
            if (iter1st->second->GetVlaueType() == web::json::kObject)
            {
                web::json::JsonObject::iterator iter2nd;

                // OrderNo
                iter2nd = iter1st->second->GetObj()->find("OrderNO");
                if (iter2nd != iter1st->second->GetObj()->end())
                {
                    if (iter2nd->second->GetVlaueType() == web::json::kString)
                    {
                        m_strOrderNo = *(iter2nd->second->GetString());
                        CUtility::TrimByRef(m_strOrderNo);
                    }
                    else
                        m_strOrderNo = "";
                }

                // SeqNo
                iter2nd = iter1st->second->GetObj()->find("SeqNo");
                if (iter2nd != iter1st->second->GetObj()->end())
                {
                    if (iter2nd->second->GetVlaueType() == web::json::kString)
                    {
                        m_strSeqNo = *(iter2nd->second->GetString());
                        CUtility::TrimByRef(m_strSeqNo);
                    }
                    else
                        m_strSeqNo = "";
                }
            }
        }
    }
    catch (string strMsg)
    {
        m_strErrCode = g_strECGWRet;
        m_strErrMsg = "CSKTFOrder Parse GW Data " + strMsg;
    }
    catch (...)
    {
        m_strErrCode = g_strECFatal;
        m_strErrMsg = "CSKTFOrder Parse GW Data " + g_strEMFatal;
    }

    if (__Node)
    {
        delete __Node;
        __Node = NULL;
    }
}

/// <summary>
/// 設定回傳資料
/// </summary>
void CSKTFOrder::SetReplyData()
{
    m_uncaReplyData[0] = ESCAPE_BYTE;
    m_uncaReplyData[1] = TFORDER_REP_BYTE;
    memcpy(m_uncaReplyData + 2, m_strErrCode.c_str(), m_strErrCode.size() > 4 ? 4 : m_strErrCode.size());
    memcpy(m_uncaReplyData + 6, m_strErrMsg.c_str(), m_strErrMsg.size() > 512 ? 512 : m_strErrMsg.size());
    memcpy(m_uncaReplyData + 518, m_strOrderNo.c_str(), m_strOrderNo.size() > 20 ? 20 : m_strOrderNo.size());
    memcpy(m_uncaReplyData + 538, m_strSeqNo.c_str(), m_strSeqNo.size() > 20 ? 20 : m_strSeqNo.size());
    memcpy(m_uncaReplyData + 558, m_strORKEY.c_str(), m_strORKEY.size() > 100 ? 100 : m_strORKEY.size());
}

void CSKTFOrder::SetLogger()
{
    m_tSKLoggerGW.strFunction.assign("AddOrderLog");
    m_tSKLoggerGW.strUserID.assign(m_shpClient->GetLoginID());
    m_tSKLoggerGW.strFunc.assign("TFOrder");
    m_tSKLoggerGW.strType.assign("0");
    m_tSKLoggerGW.strUserData.assign(m_strPostJson);
    m_tSKLoggerGW.strCode.assign(m_strErrCode);
    m_tSKLoggerGW.strMessage.assign(m_strErrMsg);
    m_tSKLoggerGW.strORKEY.assign(m_strORKEY);
    m_tSKLoggerGW.strSeqNo.assign(m_strSeqNo);
    m_tSKLoggerGW.strOrderNo.assign(m_strOrderNo);
    m_tSKLoggerGW.strUseTime.assign(m_strLogTime);
    m_tSKLoggerGW.strClientIP.assign(m_strIP);
    m_tSKLoggerGW.strServerIP.assign(g_strLocalIp);
    m_tSKLoggerGW.strStartTime.assign(m_strStartT);
}

/// <summary>
/// check 現在queue裡的委託是否有超過等待時間上限 回傳1表示超過
/// </summary>
int CSKTFOrder::CheckOverTimeToReply(const TSKTFOrder& tSKOrder, const struct timeval& tvStart)
{
    try
    {
        m_tvStart = tvStart;
        time_t nowtime = tvStart.tv_sec;
        struct tm* nowtm = localtime(&nowtime);
        char tmbuf[64], buf[64];
        strftime(tmbuf, sizeof tmbuf, "%Y %m %d %H:%M:%S", nowtm);
        snprintf(buf, sizeof buf, "%s.%06ld", tmbuf, tvStart.tv_usec);
        m_strStartT = buf;

        struct timeval tvEnd[6];
        long nTime[6] = { 0 };

        gettimeofday(&tvEnd[0], NULL); // Session Verify + SendReply(Pending Order) + EnQ + DeQ

        nTime[0] = 1000000 * (tvEnd[0].tv_sec - tvStart.tv_sec) + (tvEnd[0].tv_usec - tvStart.tv_usec);

        if (nTime[0] >= g_bMaxWaitTime) //如果在queue裡待超過maxWaitTime則丟棄，回傳error
        {
            m_strErrCode = g_strECExceedMaxTime;
            m_strErrMsg = g_strEMExceedMaxTime;


            bool bSuccess = SetPostData(tSKOrder);
            if (bSuccess == false)
            {
                gettimeofday(&tvEnd[1], NULL); // SetPostData(false)

                SetReplyData();

                gettimeofday(&tvEnd[2], NULL); //SetReplyData

                nTime[1] = 1000000 * (tvEnd[1].tv_sec - tvEnd[0].tv_sec) + (tvEnd[1].tv_usec - tvEnd[0].tv_usec);
                nTime[2] = 1000000 * (tvEnd[2].tv_sec - tvEnd[1].tv_sec) + (tvEnd[2].tv_usec - tvEnd[1].tv_usec);
                m_strLogTime += to_string(nTime[0]) + "," + to_string(nTime[1]) + ",,," + to_string(nTime[2]) + "#,";

                m_tvSendT = tvEnd[2];

                SetLogger();

                SendClientCallback();
            }
            else
            {
                gettimeofday(&tvEnd[1], NULL); // SetPostData(true)

                nTime[1] = 1000000 * (tvEnd[1].tv_sec - tvEnd[0].tv_sec) + (tvEnd[1].tv_usec - tvEnd[0].tv_usec);

                SetReplyData();

                gettimeofday(&tvEnd[2], NULL); //SetReplyData
                nTime[2] = 1000000 * (tvEnd[2].tv_sec - tvEnd[1].tv_sec) + (tvEnd[2].tv_usec - tvEnd[1].tv_usec);
                m_strLogTime += to_string(nTime[0]) + "," + to_string(nTime[1]) + ",,," + to_string(nTime[2]) + "#,";
                m_tvSendT = tvEnd[2];

                SetLogger();

                SendClientCallback();
            }
            return 1;
        }
    }
    catch (...)
    {
    }
    return 0;
}