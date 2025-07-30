#include "SKPreOFOrder.h"
#include "../json.h"
#include "../Utility.h"

CSKPreOFOrder::CSKPreOFOrder(const string& strServer) : CSKBaseGWOrder(strServer)
{
    m_tSKLoggerGW = {};
}

CSKPreOFOrder::~CSKPreOFOrder()
{
}

/// <summary>
/// 送出委託
/// </summary>
void CSKPreOFOrder::Run(const TSKOFOrder& tSKOrder, const struct timeval& tvStart, const string& strThreadID)
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
   /*     if (nLogModel == 0)
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

                   // nTime[0] = 1000000 * (tvEnd[0].tv_sec - tvStart.tv_sec) + (tvEnd[0].tv_usec - tvStart.tv_usec);
                   // nTime[1] = 1000000 * (tvEnd[1].tv_sec - tvEnd[0].tv_sec) + (tvEnd[1].tv_usec - tvEnd[0].tv_usec);
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
        m_strErrMsg = "CSKPreOFOrder Run " + g_strEMFatal;
        SetReplyData();

        SetLogger();

        SendClientCallback();
    }
}

/// <summary>
/// 確認User委託資料
/// </summary>
bool CSKPreOFOrder::SetPostData(const TSKOFOrder& tSKOrder)
{
    try
    {
        m_strORKEY = CUtility::TrimCA(tSKOrder.caORKEY, sizeof(tSKOrder.caORKEY));
        if (m_strORKEY.empty())
            throw string("No ORKEY");

        //刪改單可帶空
 /*       string strMarketType = CUtility::TrimCA(tSKOrder.caMarketType, sizeof(tSKOrder.caMarketType));
        if (!strMarketType.empty())
            if(strMarketType != "OF" && strMarketType != "OO")
                throw string("MarketType Error");*/

        string strLoginID = CUtility::TrimCA(tSKOrder.caLoginID, sizeof(tSKOrder.caLoginID));
        if (strLoginID.empty())
            throw string("No LoginID");

        string strIDNO = CUtility::TrimCA(tSKOrder.caIDNO, sizeof(tSKOrder.caIDNO));
        if (strIDNO.empty())
            throw string("No IDNO");

        string strBrokerID = CUtility::TrimCA(tSKOrder.caBrokerID, sizeof(tSKOrder.caBrokerID));
     /*   if (strBrokerID.empty())
            throw string("No BrokerID");*/

        string strIBNO = CUtility::TrimCA(tSKOrder.caIBNO, sizeof(tSKOrder.caIBNO));
      /*  if (strIBNO.empty())
            throw string("No IBNO");*/

        string strACNO = CUtility::TrimCA(tSKOrder.caACNO, sizeof(tSKOrder.caACNO));
     /*   if (strACNO.empty())
            throw string("No ACNO");*/

        string strFunCode = CUtility::TrimCA(tSKOrder.caFunCode, sizeof(tSKOrder.caFunCode));
        if (strFunCode.empty())
            throw string("No FunCode");

        //下單必帶, 刪單可帶空
        string strOrderType = CUtility::TrimCA(tSKOrder.caOrderType, sizeof(tSKOrder.caOrderType));
       /* if (strMarketType != "" && strOrderType.empty())
            throw string("No OrderType");*/

        //下單必帶, 改價可帶空
        string strQty1 = CUtility::TrimCA(tSKOrder.caQty1, sizeof(tSKOrder.caQty1));
        //if (strMarketType != "" && strQty1.empty())
        //    throw string("No Qty1");

        //非必帶
        string strQty2 = CUtility::TrimCA(tSKOrder.caQty2, sizeof(tSKOrder.caQty2));
        //if (strQty2.empty())
        //    throw string("No Qty2");

        string strOrderNo = CUtility::TrimCA(tSKOrder.caOrderNo, sizeof(tSKOrder.caOrderNo));
        //if (strMarketType == "" && strOrderNo.empty()) //改單不可空
        //    throw string("No OrderNo");

        //非必帶
        string strInputKey = CUtility::TrimCA(tSKOrder.caInputKey, sizeof(tSKOrder.caInputKey));
        //if (strInputKey.empty())
        //    throw string("No InputKey");

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

        //刪單、改量 可帶空值
        string strOrderPrice = CUtility::TrimCA(tSKOrder.caOrderPrice, sizeof(tSKOrder.caOrderPrice));

        //非必帶
        string strOrderPrice_M = CUtility::TrimCA(tSKOrder.caOrderPrice_M, sizeof(tSKOrder.caOrderPrice_M));
        //if (strOrderPrice_M.empty()) 
        //    throw string("No OrderPrice_M");

        //非必帶
        string strOrderPrice_D = CUtility::TrimCA(tSKOrder.caOrderPrice_D, sizeof(tSKOrder.caOrderPrice_D));
        //if (strOrderPrice_D.empty()) 
        //    throw string("No OrderPrice_D");

        string strFindTouchPrice = CUtility::TrimCA(tSKOrder.caTouchPrice, sizeof(tSKOrder.caTouchPrice));
        //if (strTouchPrice.empty()) 
        //    throw string("No TouchPrice");

        //非必帶
        string strFindTouchPrice_M = CUtility::TrimCA(tSKOrder.caTouchPrice_M, sizeof(tSKOrder.caTouchPrice_M));
        //if (strTouchPrice_M.empty()) 
        //    throw string("No TouchPrice_M");

        //非必帶
        string strFindTouchPrice_D = CUtility::TrimCA(tSKOrder.caTouchPrice_D, sizeof(tSKOrder.caTouchPrice_D));
        //if (strTouchPrice_D.empty()) 
        //    throw string("No TouchPrice_D");

        string strFindCOMID = CUtility::TrimCA(tSKOrder.caCOMID1, sizeof(tSKOrder.caCOMID1));

        string strFindPrice = CUtility::TrimCA(tSKOrder.caFindPrice, sizeof(tSKOrder.caFindPrice));

        string strFindPrice_D = CUtility::TrimCA(tSKOrder.caFindPrice_D, sizeof(tSKOrder.caFindPrice_D));

        string strFindPrice_M = CUtility::TrimCA(tSKOrder.caFindPrice_M, sizeof(tSKOrder.caFindPrice_M));

        string strFindBS = CUtility::TrimCA(tSKOrder.caBS1, sizeof(tSKOrder.caBS1));

        string strFindStrike = CUtility::TrimCA(tSKOrder.caStrike1, sizeof(tSKOrder.caStrike1));

        string strFindCallPut = CUtility::TrimCA(tSKOrder.caCP1, sizeof(tSKOrder.caCP1));

        string strFindSettleYMD = CUtility::TrimCA(tSKOrder.caSettleYMD1, sizeof(tSKOrder.caSettleYMD1));

        string strFindOExchange = CUtility::TrimCA(tSKOrder.caOexchange, sizeof(tSKOrder.caOexchange));

        string strDeviceID = CUtility::TrimCA(tSKOrder.caDeviceID, sizeof(tSKOrder.caDeviceID));

        if (strDeviceID.empty())
            throw string("No DeviceID");

        m_strPostJson += "{\"LOGID\":\"";
        m_strPostJson += strLoginID;
        m_strPostJson += "\",\"IDNO\":\"";
        m_strPostJson += strIDNO;
        m_strPostJson += "\",\"BrokerID\":\"";
        m_strPostJson += strBrokerID;
        m_strPostJson += "\",\"IBNO\":\"";
        m_strPostJson += strIBNO;
        m_strPostJson += "\",\"ACNO\":\"";
        m_strPostJson += strACNO;
        m_strPostJson += "\",\"FunCode\":\"";
        m_strPostJson += strFunCode;
        m_strPostJson += "\",\"OrderType\":\"";
        m_strPostJson += strOrderType;
        m_strPostJson += "\",\"Qty1\":\"";
        m_strPostJson += strQty1;
        m_strPostJson += "\",\"Qty2\":\"";
        m_strPostJson += strQty2;
        m_strPostJson += "\",\"FindCOMID\":\"";
        m_strPostJson += strFindCOMID;
        m_strPostJson += "\",\"FindPrice\":\"";
        m_strPostJson += strFindPrice;
        m_strPostJson += "\",\"FindPrice_M\":\"";
        m_strPostJson += strFindPrice_M;
        m_strPostJson += "\",\"FindPrice_D\":\"";
        m_strPostJson += strFindPrice_D;
        m_strPostJson += "\",\"FindBS\":\"";
        m_strPostJson += strFindBS;
        m_strPostJson += "\",\"FindOExchange\":\"";
        m_strPostJson += strFindOExchange;
        m_strPostJson += "\",\"FindSettleYMD\":\"";
        m_strPostJson += strFindSettleYMD;
        m_strPostJson += "\",\"FindStrike\":\"";
        m_strPostJson += strFindStrike;
        m_strPostJson += "\",\"FindCallPut\":\"";
        m_strPostJson += strFindCallPut;
        m_strPostJson += "\",\"OrderNo\":\"";
        m_strPostJson += strOrderNo;
        m_strPostJson += "\",\"KEY_NO\":\"";
        m_strPostJson += strInputKey;
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
        m_strPostJson += "\",\"OrderPrice\":\"";
        m_strPostJson += strOrderPrice;
        m_strPostJson += "\",\"OrderPrice_M\":\"";
        m_strPostJson += strOrderPrice_M;
        m_strPostJson += "\",\"OrderPrice_D\":\"";
        m_strPostJson += strOrderPrice_D;
        m_strPostJson += "\",\"FindTouchPrice\":\"";
        m_strPostJson += strFindTouchPrice;
        m_strPostJson += "\",\"FindTouchPrice_M\":\"";
        m_strPostJson += strFindTouchPrice_M;
        m_strPostJson += "\",\"FindTouchPrice_D\":\"";
        m_strPostJson += strFindTouchPrice_D;
        m_strPostJson += "\",\"Source\":\"";
        m_strPostJson += strSource;
        m_strPostJson += "\",\"ClientIP\":\"";
        m_strPostJson += m_strIP;
        m_strPostJson += "\",\"TradeDeviceID\":\"";
        m_strPostJson += strDeviceID;
        m_strPostJson += "\"}";

        /*m_strLogMsg += "{LOGID:";
        m_strLogMsg += strLoginID;
        m_strLogMsg += ",IDNO:";
        m_strLogMsg += strIDNO;
        m_strLogMsg += ",BrokerID:";
        m_strLogMsg += strBrokerID;
        m_strLogMsg += ",IBNO:";
        m_strLogMsg += strIBNO;
        m_strLogMsg += ",ACNO:";
        m_strLogMsg += strACNO;
        m_strLogMsg += ",FunCode:";
        m_strLogMsg += strFunCode;
        m_strLogMsg += ",OExchange:";
        m_strLogMsg += strOexchange;
        m_strLogMsg += ",OpenKind:";
        m_strLogMsg += strOpenKind;
        m_strLogMsg += ",DayTrade:";
        m_strLogMsg += strDayTrade;
        m_strLogMsg += ",OrderType:";
        m_strLogMsg += strOrderType;
        m_strLogMsg += ",COMID1:";
        m_strLogMsg += strCOMID1;
        m_strLogMsg += ",CallPut1:";
        m_strLogMsg += strCP1;
        m_strLogMsg += ",SettleYMD1:";
        m_strLogMsg += strSettleYMD1;
        m_strLogMsg += ",Strike1:";
        m_strLogMsg += strStrike1;
        m_strLogMsg += ",BS1:";
        m_strLogMsg += strBS1;
        m_strLogMsg += ",COMID2:";
        m_strLogMsg += strCOMID2;
        m_strLogMsg += ",CallPut2:";
        m_strLogMsg += strCP2;
        m_strLogMsg += ",SettleYMD2:";
        m_strLogMsg += strSettleYMD2;
        m_strLogMsg += ",Strike2:";
        m_strLogMsg += strStrike2;
        m_strLogMsg += ",BS2:";
        m_strLogMsg += strBS2;
        m_strLogMsg += ",COND:";
        m_strLogMsg += strCOND;
        m_strLogMsg += ",Qty1:";
        m_strLogMsg += strQty1;
        m_strLogMsg += ",Qty2:";
        m_strLogMsg += strQty2;
        m_strLogMsg += ",OrderNo:";
        m_strLogMsg += strOrderNo;
        m_strLogMsg += ",InputKey:";
        m_strLogMsg += strInputKey;
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
        m_strLogMsg += ",OrderPrice:";
        m_strLogMsg += strOrderPrice;
        m_strLogMsg += ",OrderPrice_M:";
        m_strLogMsg += strOrderPrice_M;
        m_strLogMsg += ",OrderPrice_D:";
        m_strLogMsg += strOrderPrice_D;
        m_strLogMsg += ",TouchPrice:";
        m_strLogMsg += strTouchPrice;
        m_strLogMsg += ",TouchPrice_M:";
        m_strLogMsg += strTouchPrice_M;
        m_strLogMsg += ",TouchPrice_D:";
        m_strLogMsg += strTouchPrice_D;
        m_strLogMsg += ",Source:";
        m_strLogMsg += strSource;
        m_strLogMsg += ",ClientIP:";
        m_strLogMsg += m_strIP;
        m_strLogMsg += ",TradeDeviceID:";
        m_strLogMsg += strDeviceID;
        m_strLogMsg += "}";*/

        /*m_strORKEY.append(tSKOFOrder.caORKEY, 100);
        CUtility::TrimByRef(m_strORKEY);
        if (m_strORKEY.empty())
            throw string("No ORKEY");

        //刪改單可帶空
        string strMarketType(tSKOFOrder.caMarketType, 2);
        CUtility::TrimByRef(strMarketType);
        if (!strMarketType.empty())
            if (strMarketType != "OF" && strMarketType != "OO")
                throw string("MarketType Error");

        string strLoginID(tSKOFOrder.caLoginID, 10);
        CUtility::TrimByRef(strLoginID);
        if (strLoginID.empty())
            throw string("No LoginID");

        string strIDNO(tSKOFOrder.caIDNO, 10);
        CUtility::TrimByRef(strIDNO);
        if (strIDNO.empty())
            throw string("No IDNO");

        string strBrokerID(tSKOFOrder.caBrokerID, 7);
        CUtility::TrimByRef(strBrokerID);
        if (strBrokerID.empty())
            throw string("No BrokerID");

        string strIBNO(tSKOFOrder.caIBNO, 3);
        CUtility::TrimByRef(strIBNO);
        if (strIBNO.empty())
            throw string("No IBNO");

        string strACNO(tSKOFOrder.caACNO, 7);
        CUtility::TrimByRef(strACNO);
        if (strACNO.empty())
            throw string("No ACNO");

        string strFunCode(tSKOFOrder.caFunCode, 1);
        CUtility::TrimByRef(strFunCode);
        if (strFunCode.empty())
            throw string("No FunCode");

        string strOexchange(tSKOFOrder.caOexchange, 7);
        CUtility::TrimByRef(strOexchange);
        if (strOexchange.empty())
            throw string("No Oexchange");

        string strOpenKind(tSKOFOrder.caOpenKind, 1);
        CUtility::TrimByRef(strOpenKind);
        if (strOpenKind.empty())
            throw string("No OpenKind");

        //不當沖, 刪改單 可帶空值
        string strDayTrade(tSKOFOrder.caDayTrade, 1);
        CUtility::TrimByRef(strDayTrade);
        //if (strDayTrade.empty())
        //    throw string("No DayTrade");

        string strOrderType(tSKOFOrder.caOrderType, 1);
        CUtility::TrimByRef(strOrderType);
        if (strOrderType.empty())
            throw string("No OrderType");

        string strCOMID1(tSKOFOrder.caCOMID1, 7);
        CUtility::TrimByRef(strCOMID1);
        if (strCOMID1.empty())
            throw string("No COMID1");

        string strSettleYMD1(tSKOFOrder.caSettleYMD1, 6);
        CUtility::TrimByRef(strSettleYMD1);
        if (strSettleYMD1.empty())
            throw string("No SettleYMD1");

        string strStrike1(tSKOFOrder.caStrike1, 8);
        CUtility::TrimByRef(strStrike1);
        if (strMarketType == "OO" && strStrike1.empty())
            throw string("No Strike1");

        string strCP1(tSKOFOrder.caCP1, 1);
        CUtility::TrimByRef(strCP1);
        if (strMarketType == "OO" && strCP1.empty())
            throw string("No CP1");

        //刪改單不用帶
        string strBS1(tSKOFOrder.caBS1, 1);
        CUtility::TrimByRef(strBS1);
        if (strMarketType != "" && strBS1.empty()) //下單不可空
            throw string("No BS1");

        //非必帶
        string strCOMID2(tSKOFOrder.caCOMID2, 7);
        CUtility::TrimByRef(strCOMID2);
        //if (strCOMID2.empty())
        //    throw string("No COMID2");

        //非必帶
        string strSettleYMD2(tSKOFOrder.caSettleYMD2, 6);
        CUtility::TrimByRef(strSettleYMD2);
        //if (strSettleYMD2.empty())
        //    throw string("No SettleYMD2");

        //非必帶
        string strStrike2(tSKOFOrder.caStrike2, 8);
        CUtility::TrimByRef(strStrike2);
        //if (strStrike2.empty())
        //    throw string("No Strike2");

        //非必帶
        string strCP2(tSKOFOrder.caCP2, 1);
        CUtility::TrimByRef(strCP2);
        //if (strCP2.empty())
        //    throw string("No CP2");

        //非必帶
        string strBS2(tSKOFOrder.caBS2, 1);
        CUtility::TrimByRef(strBS2);
        //if (strBS2.empty())
        //    throw string("No BS2");

        string strCOND(tSKOFOrder.caCOND, 1);
        CUtility::TrimByRef(strCOND);
        if (strCOND.empty())
            throw string("No COND");

        //下單必帶, 改價可帶空
        string strQty1(tSKOFOrder.caQty1, 8);
        CUtility::TrimByRef(strQty1);
        if (strMarketType != "" && strQty1.empty()) //下單不可空
            throw string("No Qty1");

        //非必帶
        string strQty2(tSKOFOrder.caQty2, 8);
        CUtility::TrimByRef(strQty2);
        //if (strQty2.empty())
        //    throw string("No Qty2");

        string strOrderNo(tSKOFOrder.caOrderNo, 5);
        CUtility::TrimByRef(strOrderNo);
        if (strMarketType == "" && strOrderNo.empty()) //改單不可空
            throw string("No OrderNo");

        //非必帶
        string strInputKey(tSKOFOrder.caInputKey, 13);
        CUtility::TrimByRef(strInputKey);
        //if (strInputKey.empty())
        //    throw string("No InputKey");

        string strTranType(tSKOFOrder.caTranType, 2);
        CUtility::TrimByRef(strTranType);
        if (strTranType.empty())
            throw string("No TranType");

        string strRawData(tSKOFOrder.caRawData, 150);
        CUtility::TrimByRef(strRawData);
        if (strRawData.empty())
            throw string("No RawData");

        string strSign(tSKOFOrder.caSign, 2700);
        CUtility::TrimByRef(strSign);
        if (strSign.empty())
            throw string("No Sign");

        string strCN(tSKOFOrder.caCN, 30);
        CUtility::TrimByRef(strCN);
        if (strCN.empty())
            throw string("No CN");

        string strCertID(tSKOFOrder.caCertID, 8);
        CUtility::TrimByRef(strCertID);
        if (strCertID.empty())
            throw string("No CertID");

        string strCertType(tSKOFOrder.caCertType, 1);
        CUtility::TrimByRef(strCertType);
        if (strCertType.empty())
            throw string("No CertType");

        string strSource(tSKOFOrder.caSource, 2);
        CUtility::TrimByRef(strSource);
        if (strSource.empty())
            throw string("No Source");

        //刪單、改量 可帶空值
        string strOrderPrice(tSKOFOrder.caOrderPrice, 14);
        CUtility::TrimByRef(strOrderPrice);
        if (strMarketType != "" && strOrderPrice.empty()) //下單不可空
            throw string("No OrderPrice");

        //非必帶
        string strOrderPrice_M(tSKOFOrder.caOrderPrice_M, 5);
        CUtility::TrimByRef(strOrderPrice_M);
        //if (strOrderPrice_M.empty()) 
        //    throw string("No OrderPrice_M");

        //非必帶
        string strOrderPrice_D(tSKOFOrder.caOrderPrice_D, 4);
        CUtility::TrimByRef(strOrderPrice_D);
        //if (strOrderPrice_D.empty()) 
        //    throw string("No OrderPrice_D");

        //非必帶
        string strTouchPrice(tSKOFOrder.caTouchPrice, 14);
        CUtility::TrimByRef(strTouchPrice);
        //if (strTouchPrice.empty()) 
        //    throw string("No TouchPrice");

        //非必帶
        string strTouchPrice_M(tSKOFOrder.caTouchPrice_M, 5);
        CUtility::TrimByRef(strTouchPrice_M);
        //if (strTouchPrice_M.empty()) 
        //    throw string("No TouchPrice_M");

        //非必帶
        string strTouchPrice_D(tSKOFOrder.caTouchPrice_D, 4);
        CUtility::TrimByRef(strTouchPrice_D);
        //if (strTouchPrice_D.empty()) 
        //    throw string("No TouchPrice_D");

        string strDeviceID(tSKOFOrder.caDeviceID, 50);
        CUtility::TrimByRef(strDeviceID);
        if (strDeviceID.empty())
            throw string("No DeviceID");*/

        /*char caBuf[4096];
        sprintf(caBuf, "{\"LOGID\":\"%s\",\"IDNO\":\"%s\",\"BrokerID\":\"%s\",\"IBNO\":\"%s\",\"ACNO\":\"%s\""
            ",\"FunCode\":\"%s\",\"OExchange\":\"%s\",\"OpenKind\":\"%s\",\"DayTrade\":\"%s\",\"OrderType\":\"%s\""
            ",\"COMID1\":\"%s\",\"CallPut1\":\"%s\",\"SettleYMD1\":\"%s\",\"Strike1\":\"%s\",\"BS1\":\"%s\""
            ",\"COMID2\":\"%s\",\"CallPut2\":\"%s\",\"SettleYMD2\":\"%s\",\"Strike2\":\"%s\",\"BS2\":\"%s\""
            ",\"COND\":\"%s\",\"Qty1\":\"%s\",\"Qty2\":\"%s\",\"OrderNo\":\"%s\",\"InputKey\":\"%s\""
            ",\"ORKEY\":\"%s\",\"TranType\":\"%s\",\"RawData\":\"%s\",\"Sign\":\"%s\",\"CN\":\"%s\""
            ",\"CertID\":\"%s\",\"CertType\":\"%s\",\"OrderPrice\":\"%s\",\"OrderPrice_M\":\"%s\",\"OrderPrice_D\":\"%s\""
            ",\"TouchPrice\":\"%s\",\"TouchPrice_M\":\"%s\",\"TouchPrice_D\":\"%s\",\"Source\":\"%s\",\"ClientIP\":\"%s\""
            ",\"TradeDeviceID\":\"%s\"}"
            , strLoginID.c_str()
            , strIDNO.c_str()
            , strBrokerID.c_str()
            , strIBNO.c_str()
            , strACNO.c_str()
            , strFunCode.c_str()
            , strOexchange.c_str()
            , strOpenKind.c_str()
            , strDayTrade.c_str()
            , strOrderType.c_str()
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
            , strCOND.c_str()
            , strQty1.c_str()
            , strQty2.c_str()
            , strOrderNo.c_str()
            , strInputKey.c_str()
            , m_strORKEY.c_str()
            , strTranType.c_str()
            , strRawData.c_str()
            , strSign.c_str()
            , strCN.c_str()
            , strCertID.c_str()
            , strCertType.c_str()
            , strOrderPrice.c_str()
            , strOrderPrice_M.c_str()
            , strOrderPrice_D.c_str()
            , strTouchPrice.c_str()
            , strTouchPrice_M.c_str()
            , strTouchPrice_D.c_str()
            , strSource.c_str()
            , m_strIP.c_str()
            , strDeviceID.c_str());
        m_strPostJson = string(caBuf);*/

        return true;
    }
    catch (string strMsg)
    {
        m_strErrCode = g_strECUserData;
        m_strErrMsg = "CSKPreOFOrder SetPostData " + strMsg;
    }
    catch (...)
    {
        m_strErrCode = g_strECFatal;
        m_strErrMsg = "CSKPreOFOrder SetPostData " + g_strEMFatal;
    }

    return false;
}

/// <summary>
/// 確認回傳資料
/// </summary>
void CSKPreOFOrder::ParseGWData(const string& strData)
{
    web::json::Parser parser;
    web::json::JsonObject::iterator iter1st;
    web::json::Node* __Node = NULL;

    try
    {
        /*
        {
            "Code": "0",
            "Message": "x0-031 8931353ＯＯＯＯ買ES02645C9      新   5口委:     67.75000          /",
            "datas": {
                "Messages": "x0-031 8931353ＯＯＯＯ買ES02645C9      新   5口委:     67.75000          /",
                "BrokerID": "F020000",
                "IBNO": "000",
                "ACNO": "8931353",
                "OrderNO": "x0031",
                "UppNO": "",
                "AccountBalance": "51190.50",
                "VarInCome": "59685.56",
                "UsableMargin": "-54289.19",
                "UsedMargin": "",
                "OffsetMargin": "-71301.69",
                "IniFee": "",
                "Equity": "124036.50",
                "TodayPremium": "75.00",
                "Premium": "93863.56",
                "MainatinRate": "",
                "SeqNo": "9830000066868",
                "strTXDate": ""
            }
        }
        */

        // Parse Json
        if (!parser.Parse(strData, &__Node))
        {
            if (strData.find("!DOCTYPE") != string::npos && strData.find("html") != string::npos)
            {
                write_log_(e_ltServerError, "(PreOFOrder)連不上srvgw或是proxy server呼叫的url有誤", false);
                throw g_strEMGWConnFail;
            }
            else
            {
                write_log_(e_ltServerError, "receive Data from srvgw(PreOFOrder) = " + strData, false);
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
        m_strErrMsg = "CSKPreOFOrder Parse GW Data " + strMsg;
    }
    catch (...)
    {
        m_strErrCode = g_strECFatal;
        m_strErrMsg = "CSKPreOFOrder Parse GW Data " + g_strEMFatal;
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
void CSKPreOFOrder::SetReplyData()
{
    m_uncaReplyData[0] = ESCAPE_BYTE;
    m_uncaReplyData[1] = OFORDER_REP_BYTE;
    memcpy(m_uncaReplyData + 2, m_strErrCode.c_str(), m_strErrCode.size() > 4 ? 4 : m_strErrCode.size());
    memcpy(m_uncaReplyData + 6, m_strErrMsg.c_str(), m_strErrMsg.size() > 512 ? 512 : m_strErrMsg.size());
    memcpy(m_uncaReplyData + 518, m_strOrderNo.c_str(), m_strOrderNo.size() > 20 ? 20 : m_strOrderNo.size());
    memcpy(m_uncaReplyData + 538, m_strSeqNo.c_str(), m_strSeqNo.size() > 20 ? 20 : m_strSeqNo.size());
    memcpy(m_uncaReplyData + 558, m_strORKEY.c_str(), m_strORKEY.size() > 100 ? 100 : m_strORKEY.size());
}

void CSKPreOFOrder::SetLogger()
{
    m_tSKLoggerGW.strFunction.assign("AddOrderLog");
    m_tSKLoggerGW.strUserID.assign(m_shpClient->GetLoginID());
    m_tSKLoggerGW.strFunc.assign("PreOFOrder");
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
int CSKPreOFOrder::CheckOverTimeToReply(const TSKOFOrder& tSKOrder, const struct timeval& tvStart)
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