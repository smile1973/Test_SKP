#include "SKLogin.h"
#include "../json.h"
#include "../Utility.h"
//#include "../SKGlobal.h"

CSKLogin::CSKLogin(const string& strServer, const string& strConnURL, const string& strLoginSalesURL) : CSKBaseGW(strServer)
{
    m_strLoginID = "";
    m_strSessionID = "";
    m_strSource = "";
    m_strConnPostJson = "";
    m_strConnErrCode = "";
    m_strConnErrMsg = "";
    m_strConnErrData = "";
    
    m_strServerConnectionURL = strConnURL;
    m_strLoginSalesURL = strLoginSalesURL;
    memset(m_uncaReplyData, 0, sizeof(m_uncaReplyData));
    m_tSKLoggerGW = {};
}

CSKLogin::~CSKLogin()
{
}

void CSKLogin::SetLogger()
{
    m_tSKLoggerGW.strFunction.assign("AddFuncLog");
    m_tSKLoggerGW.strUserID.assign(m_strLoginID);
    m_tSKLoggerGW.strFunc.assign("Login");
    m_tSKLoggerGW.strType.assign("0");
    m_tSKLoggerGW.strUserData.assign(m_strPostJson);
    m_tSKLoggerGW.strCode.assign(m_strErrCode);
    m_tSKLoggerGW.strMessage.assign(m_strErrMsg);
    m_tSKLoggerGW.strReplyData.assign("ErrorCode:" + m_strErrCode + ";ErrorMsg:" + m_strErrMsg + ";SessionID:" + m_strSessionID);
    m_tSKLoggerGW.strUseTime.assign(m_strLogTime);
    m_tSKLoggerGW.strClientIP.assign(m_strIP);
    m_tSKLoggerGW.strServerIP.assign(g_strLocalIp);
    m_tSKLoggerGW.strStartTime.assign(m_strStartT);
}

/// <summary>
/// 登入
/// </summary>
/// <param name="tSKLogin">登入資料</param>
/// <param name="tvStart">開始時間</param>
void CSKLogin::Run(const TSKLogin& tSKLogin, const struct timeval& tvStart)
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
     /*   if (nLogModel == 0)
        {*/
            struct timeval tvEnd[6];
            long nTime[6] = { 0 };

            gettimeofday(&tvEnd[0], NULL); // EnQ + DeQ
            
            bool bSuccess = SetPostData(tSKLogin);
            if (bSuccess == false)
            {
                m_shpClient->SetLoginResult(false);
                SetReplyData();

                gettimeofday(&tvEnd[1], NULL); // SetPostData(false)
                nTime[0] = 1000000 * (tvEnd[0].tv_sec - tvStart.tv_sec) + (tvEnd[0].tv_usec - tvStart.tv_usec);
                nTime[1] = 1000000 * (tvEnd[1].tv_sec - tvEnd[0].tv_sec) + (tvEnd[1].tv_usec - tvEnd[0].tv_usec);
                m_strLogTime += to_string(nTime[0]) + "," + to_string(nTime[1]) + ",,,,,,";

                SetLogger();
                SendClientCallback();
            }
            else
            {
                gettimeofday(&tvEnd[1], NULL); // SetPostData(true)

                if (m_strLoginID.find("SALE") != string::npos)
                    m_hRequest.Post(m_strLoginSalesURL, m_strPostJson, m_ssResponse,"");
                else
                    m_hRequest.Post(m_strServer, m_strPostJson, m_ssResponse,"");

                gettimeofday(&tvEnd[2], NULL); // Post GW

                nowtime = tvEnd[2].tv_sec;
                nowtm = localtime(&nowtime);
                strftime(tmbuf, sizeof tmbuf, "%Y-%m-%d %H:%M:%S", nowtm);
                snprintf(buf, sizeof buf, "%s.%06ld", tmbuf, tvStart.tv_usec);
                string strAfterGWTime = buf;

                ParseGWData(m_ssResponse.str());

                gettimeofday(&tvEnd[3], NULL); // Parse GW Data
                
                if (m_strErrCode == "000") // Login Success
                {
                    m_shpClient->SetUserData(m_strSource, m_strSessionID,  m_strLoginID);
                    SetShmAndConn();
                }
                else
                    m_shpClient->SetLoginResult(false);

                SetReplyData();

                gettimeofday(&tvEnd[4], NULL); // Set Client Data + Set Reply Data 

                nowtime = tvEnd[4].tv_sec;
                nowtm = localtime(&nowtime);
                strftime(tmbuf, sizeof tmbuf, "%Y-%m-%d %H:%M:%S", nowtm);
                snprintf(buf, sizeof buf, "%s.%06ld", tmbuf, tvStart.tv_usec);
                string strSendQTime = buf;

                nTime[0] = 1000000 * (tvEnd[0].tv_sec - tvStart.tv_sec) + (tvEnd[0].tv_usec - tvStart.tv_usec);
                nTime[1] = 1000000 * (tvEnd[1].tv_sec - tvEnd[0].tv_sec) + (tvEnd[1].tv_usec - tvEnd[0].tv_usec);
                nTime[2] = 1000000 * (tvEnd[2].tv_sec - tvEnd[1].tv_sec) + (tvEnd[2].tv_usec - tvEnd[1].tv_usec);
                nTime[3] = 1000000 * (tvEnd[3].tv_sec - tvEnd[2].tv_sec) + (tvEnd[3].tv_usec - tvEnd[2].tv_usec);
                nTime[4] = 1000000 * (tvEnd[4].tv_sec - tvEnd[3].tv_sec) + (tvEnd[4].tv_usec - tvEnd[3].tv_usec);
              
                m_tvSendT = tvEnd[4];

                m_strLogTime += to_string(nTime[0]) + "," + to_string(nTime[1]) + "," + to_string(nTime[2])
                    + "," + to_string(nTime[3]) + "," + to_string(nTime[4]) + "#" + strAfterGWTime + "," + strSendQTime;

                SetLogger();

                SendClientCallback();
            }
        //}
        //// Release Log
        //else
        //{
        //    struct timeval tvEnd;
        //    long nTime = 0;

        //    bool bSuccess = SetPostData(tSKLogin);
        //    if (bSuccess == false)
        //    {
        //        m_shpClient->SetLoginResult(false);
        //        SetReplyData();
        //        SendClientCallback();

        //        gettimeofday(&tvEnd, NULL); // End
        //        nTime = 1000000 * (tvEnd.tv_sec - tvStart.tv_sec) + (tvEnd.tv_usec - tvStart.tv_usec);
        //        m_strLogTime += to_string(nTime);
        //    }
        //    else
        //    {
        //        if (m_strLoginID.find("SALE") != string::npos)
        //            m_hRequest.Post(m_strLoginSalesURL, m_strPostJson, m_ssResponse);
        //        else
        //            m_hRequest.Post(m_strServer, m_strPostJson, m_ssResponse);

        //        ParseGWData(m_ssResponse.str());

        //        if (m_strErrCode == "000") // Login Success
        //        {
        //            m_shpClient->SetUserData(m_strSource, m_strSessionID, m_strLoginID);
        //            SetShmAndConn();
        //        }
        //        else
        //            m_shpClient->SetLoginResult(false);

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
        m_strErrMsg = "Login Run " + g_strEMFatal;
        SetReplyData();
        SetLogger();
        SendClientCallback();
    }
}

/// <summary> 
/// 確認客戶取得 Shm Index 是否正常 
/// </summary>
void CSKLogin::SetShmAndConn()
{
    try
    {
        // 客戶ShmIndex是否正常: 正常>=0, 異常<0
        bool bCheck = m_shpClient->CheckShmIndex();
        if (bCheck)
        {
            bool bSetData = SetPostConnectionData();
            if (bSetData)
            {
                //string strResult_Connection = "";
                m_ssResponse.clear();
                m_ssResponse.str("");
                m_hConnRequest.Post(m_strServerConnectionURL, m_strConnPostJson, m_ssResponse,"");
                ParseConnectionGWData(m_ssResponse.str());
            }

            // 紀錄LOG在本地端
            if (m_strConnErrCode == "000"|| (m_strConnErrCode == "403"&& m_strConnErrData[0] == '3'))
                write_log_(e_ltServerData, "Login IncreaseConn Code:" + m_strConnErrCode + 
                    "; Msg:" + m_strConnErrMsg + "; PostJson:" + m_strConnPostJson, false);
            else
                write_log_(e_ltServerError, "Login IncreaseConn ErrorCode:" + m_strConnErrCode + 
                    "; ErrorMsg:" + m_strConnErrMsg + "; PostJson:" + m_strConnPostJson, false);

            // 403:連線數超過上限, 當作登入失敗
            // 其它就當成功囉, 避免GW異常造成客戶登入不了
            if (m_strConnErrCode == "403" && m_strConnErrData[0] == '3')
                m_shpClient->SetLoginResult(false);
            else
                m_shpClient->SetLoginResult(true);
        }
        else
        {
            m_strErrCode = g_strECShm;
            m_strErrMsg = g_strEMShm;

            m_shpClient->SetLoginResult(false);
        }
    }
    catch (...)
    {
        m_strErrCode = g_strECFatal;
        m_strErrMsg = "Session Check Shm Index " + g_strEMFatal;
    }
}

/// <summary>
/// 清除資料
/// </summary>
void CSKLogin::CleanData()
{
    CSKBaseGW::CleanData();

    m_strLoginID = "";
    m_strSessionID = "";
    m_strSource = "";
    m_strConnPostJson = "";
    m_strConnErrCode = "";
    m_strConnErrMsg = "";
    m_strConnErrData = "";
    memset(m_uncaReplyData, 0, sizeof(m_uncaReplyData));
}

/// <summary>
/// 確認User登入資料後編成PostJson
/// </summary>
/// <param name="tSKLogin">User登入資料</param>
/// <returns>true/false</returns>
bool CSKLogin::SetPostData(const TSKLogin& tSKLogin)
{
    try
    {
        m_strLoginID = CUtility::TrimCA(tSKLogin.caIDNO, sizeof(tSKLogin.caIDNO));
        if (m_strLoginID.empty())
            throw string("No IDNO");

        string strFunc = CUtility::TrimCA(tSKLogin.caFunc, sizeof(tSKLogin.caFunc));
        if (strFunc.empty())
            throw string("No Func");

        string strPawdType = CUtility::TrimCA(tSKLogin.caPawdType, sizeof(tSKLogin.caPawdType));
        if (strPawdType.empty())
            throw string("No PawdType");

        string strPassword = CUtility::TrimCA(tSKLogin.caPassword, sizeof(tSKLogin.caPassword));
        if (strPassword.empty())
            throw string("No Password");

        // 一般用戶可帶空值
        string strCustID = CUtility::TrimCA(tSKLogin.caCustID, sizeof(tSKLogin.caCustID));
        //if (strCustID.empty())
        //    throw string("No CustID");

        string strTranType = CUtility::TrimCA(tSKLogin.caTranType, sizeof(tSKLogin.caTranType));
        if (strTranType.empty())
            throw string("No TranType");

        string strCertType = CUtility::TrimCA(tSKLogin.caCertType, sizeof(tSKLogin.caCertType));
        if (strCertType.empty())
            throw string("No CertType");

        string strCN = CUtility::TrimCA(tSKLogin.caCN, sizeof(tSKLogin.caCN));
        if (strCN.empty())
            throw string("No CN");

        string strCertID = CUtility::TrimCA(tSKLogin.caCertID, sizeof(tSKLogin.caCertID));
        if (strCertID.empty())
            throw string("No CertID");

        string strSign = CUtility::TrimCA(tSKLogin.caSign, sizeof(tSKLogin.caSign));
        if (strSign.empty())
            throw string("No Sign");

        string strRawData = CUtility::TrimCA(tSKLogin.caRawData, sizeof(tSKLogin.caRawData));
        if (strRawData.empty())
            throw string("No RawData");

        // 有憑證可帶空值
        string strUUID = CUtility::TrimCA(tSKLogin.caUUID, sizeof(tSKLogin.caUUID));
        //if (strUUID.empty())
        //    throw string("No UUID");

        m_strSource = CUtility::TrimCA(tSKLogin.caSource, sizeof(tSKLogin.caSource));
        if (m_strSource.empty())
            throw string("No Source");

        // VersionNum
        //string strVersionNum(tSKLogin.caVersionNum, sizeof(tSKLogin.caVersionNum));
        //CUtility::TrimByRef(strVersionNum);
        //if (strVersionNum.empty())
        //   throw string("No VersionNum");

        //int count = 0;
        //for (count = 0; count < sizeof(g_strAcceptClientVersion); count++)
        //{
        //    if (strVersionNum == g_strAcceptClientVersion[count])
        //        break;
        //}
        //if(count == sizeof(g_strAcceptClientVersion))
        //    throw string("不支持此版本的lib，請更新至最新版本");

        m_strPostJson += "{\"IDNO\":\"";
        m_strPostJson += m_strLoginID;
        m_strPostJson += "\",\"Func\":\"";
        m_strPostJson += strFunc;
        m_strPostJson += "\",\"PawdType\":\"";
        m_strPostJson += strPawdType;
        m_strPostJson += "\",\"Password\":\"";
        m_strPostJson += strPassword;
        m_strPostJson += "\",\"CustID\":\"";
        m_strPostJson += strCustID;
        m_strPostJson += "\",\"TranType\":\"";
        m_strPostJson += strTranType;
        m_strPostJson += "\",\"CertType\":\"";
        m_strPostJson += strCertType;
        m_strPostJson += "\",\"CN\":\"";
        m_strPostJson += strCN;
        m_strPostJson += "\",\"CertID\":\"";
        m_strPostJson += strCertID;
        m_strPostJson += "\",\"Sign\":\"";
        m_strPostJson += strSign;
        m_strPostJson += "\",\"RawData\":\"";
        m_strPostJson += strRawData;
        m_strPostJson += "\",\"UUID\":\"";
        m_strPostJson += strUUID;
        m_strPostJson += "\",\"Source\":\"";
        m_strPostJson += m_strSource;
        m_strPostJson += "\",\"ClientIP\":\"";
        m_strPostJson += m_strIP;
        m_strPostJson += "\"}";
       
        /*m_strLogMsg += "{IDNO:";
        m_strLogMsg += m_strLoginID;
        m_strLogMsg += ",Func:";
        m_strLogMsg += strFunc;
        m_strLogMsg += ",PawdType:";
        m_strLogMsg += strPawdType;
        m_strLogMsg += ",CustID:";
        m_strLogMsg += strCustID;
        m_strLogMsg += ",TranType:";
        m_strLogMsg += strTranType;
        m_strLogMsg += ",CertType:";
        m_strLogMsg += strCertType;
        m_strLogMsg += ",CN:";
        m_strLogMsg += strCN;
        m_strLogMsg += ",CertID:";
        m_strLogMsg += strCertID;
        m_strLogMsg += ",RawData:";
        m_strLogMsg += strRawData;
        m_strLogMsg += ",UUID:";
        m_strLogMsg += strUUID;
        m_strLogMsg += ",Source:";
        m_strLogMsg += m_strSource;
        m_strLogMsg += ",ClientIP:";
        m_strLogMsg += m_strIP;
        m_strLogMsg += "}";*/

        /*m_strLoginID.append(tSKLogin.caIDNO, 10);
        CUtility::TrimByRef(m_strLoginID);
        if (m_strLoginID.empty())
            throw string("No IDNO");

        string strFunc(tSKLogin.caFunc, 1);
        CUtility::TrimByRef(strFunc);
        if (strFunc.empty())
            throw string("No Func");

        string strPawdType(tSKLogin.caPawdType, 1);
        CUtility::TrimByRef(strPawdType);
        if (strPawdType.empty())
            throw string("No PawdType");

        string strPassword(tSKLogin.caPassword, 50);
        CUtility::TrimByRef(strPassword);
        if (strPassword.empty())
            throw string("No Password");

        // 一般用戶可帶空值
        string strCustID(tSKLogin.caCustID, 10);
        CUtility::TrimByRef(strCustID);
        //if (strCustID.empty())
        //    throw string("No CustID");

        string strTranType(tSKLogin.caTranType, 2);
        CUtility::TrimByRef(strTranType);
        if (strTranType.empty())
            throw string("No TranType");

        string strCertType(tSKLogin.caCertType, 1);
        CUtility::TrimByRef(strCertType);
        if (strCertType.empty())
            throw string("No CertType");

        string strCN(tSKLogin.caCN, 30);
        CUtility::TrimByRef(strCN);
        if (strCN.empty())
            throw string("No CN");

        string strCertID(tSKLogin.caCertID, 8);
        CUtility::TrimByRef(strCertID);
        if (strCertID.empty())
            throw string("No CertID");

        string strSign(tSKLogin.caSign, 2700);
        CUtility::TrimByRef(strSign);
        if (strSign.empty())
            throw string("No Sign");

        string strRawData(tSKLogin.caRawData, 150);
        CUtility::TrimByRef(strRawData);
        if (strRawData.empty())
            throw string("No RawData");

        // 有憑證可帶空值
        string strUUID(tSKLogin.caUUID, 256);
        CUtility::TrimByRef(strUUID);
        //if (strUUID.empty())
        //    throw string("No UUID");

        m_strSource.append(tSKLogin.caSource, 2);
        CUtility::TrimByRef(m_strSource);
        if (m_strSource.empty())
            throw string("No Source");

        // VersionNum
        //string strVersionNum(tSKLogin.caVersionNum, 10);
        //CUtility::TrimByRef(strVersionNum);
        //if (strVersionNum.empty())
        //   throw string("No VersionNum");*/

        /*char caBuf[4096];
        sprintf(caBuf, "{\"IDNO\":\"%s\",\"Func\":\"%s\",\"PawdType\":\"%s\",\"Password\":\"%s\",\"CustID\":\"%s\""
            ",\"TranType\":\"%s\",\"CertType\":\"%s\",\"CN\":\"%s\",\"CertID\":\"%s\",\"Sign\":\"%s\""
            ",\"RawData\":\"%s\",\"UUID\":\"%s\",\"Source\":\"%s\",\"ClientIP\":\"%s\"}"
            , m_strLoginID.c_str()
            , strFunc.c_str()
            , strPawdType.c_str()
            , strPassword.c_str()
            , strCustID.c_str()
            , strTranType.c_str()
            , strCertType.c_str()
            , strCN.c_str()
            , strCertID.c_str()
            , strSign.c_str()
            , strRawData.c_str()
            , strUUID.c_str()
            , m_strSource.c_str()
            , m_strIP.c_str());
        m_strPostJson = string(caBuf);*/

        return true;
    }
    catch (string strMsg)
    {
        m_strErrCode = g_strECUserData;
        m_strErrMsg = "Login Parse User Data " + strMsg;
    }
    catch (...)
    {
        m_strErrCode = g_strECFatal;
        m_strErrMsg = "Login Parse User Data " + g_strEMFatal;
    }
    return false;
}

/// <summary>
/// 確認GW回傳資料
/// </summary>
/// <param name="strData">GW回傳資料</param>
void CSKLogin::ParseGWData(const string& strData)
{
    web::json::Parser parser;
    web::json::JsonObject::iterator iter1st;
    web::json::Node* __Node = NULL;

    try
    {
        /*
        {
            "Code":"000",
            "Message":"密碼必須大於8個字元、小於30個字元、不可使用連續升冪或降冪或全部相同的文、數字為維護網路資料安全，建議立即修改！",
            "datas":
            {
                "ErrTimes":"0",
                "CustClass":"1",
                "ActCode":"",
                "CAStatus":"ED",
                "SessionID":"{F3BFF38D-75FF-43B9-BAA4-5ED38F08568C}",
                "Authority":
                [
                    {
                    "Unit":"99",
                    "Acno":"9827666",
                    "MarketType":"001",
                    "OrderType":"3"
                    }
                ]
            }
        }
        */

        // Parse Json
        if (!parser.Parse(strData, &__Node))
            throw g_strEMParseJson;

        // Code
        iter1st = __Node->GetObj()->find("Code");
        if (iter1st != __Node->GetObj()->end())
        {
            if (iter1st->second->GetVlaueType() == web::json::kString)
            {
                m_strErrCode = *(iter1st->second->GetString());
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
                m_strErrMsg =*(iter1st->second->GetString());
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

                // SessionID
                iter2nd = iter1st->second->GetObj()->find("SessionID");
                if (iter2nd != iter1st->second->GetObj()->end())
                {
                    if (iter2nd->second->GetVlaueType() == web::json::kString)
                    {
                        m_strSessionID = *(iter2nd->second->GetString());
                        CUtility::TrimSessionIDByRef(m_strSessionID);
                    }
                    else
                        m_strSessionID = "";
                }
            }
        }
    }
    catch (string strMsg)
    {
        m_strErrCode = g_strECGWRet;
        m_strErrMsg = "Login Parse GW Data " + strMsg;
        m_strSessionID = "";
    }
    catch (...)
    {
        m_strErrCode = g_strECFatal;
        m_strErrMsg = "Login Parse GW Data " + g_strEMFatal;
        m_strSessionID = "";
    }

    if (__Node)
    {
        delete __Node;
        __Node = NULL;
    }
}

/// <summary>
/// 確認UserShm後編成PostConnectionJson
/// </summary>
bool CSKLogin::SetPostConnectionData()
{
    string strFunc = "";
    string strSocketID = "";
    string strServerIP = "";
    try
    {
        strFunc = "IncreaseConn";

        strSocketID = to_string(m_shpClient->GetClientSocketFd());
        if (strSocketID.empty())
            throw string("No SocketID");

        strServerIP = g_strLocalIp;
        if (strServerIP.empty())
            throw string("No ServerIP");

        m_strConnPostJson = "{\"function\":\"" + strFunc + "\",\"UserID\":\"" + m_strLoginID + "\",\"SocketID\":\"" + strSocketID +
            "\",\"ServerIP\":\"" + strServerIP + "\",\"ClientIP\":\"" + m_strIP + "\"}";

        return true;
    }
    catch (string strMsg)
    {
        m_strConnErrCode = g_strECUserData;
        m_strConnErrMsg = "Login Parse User Connection Data " + strMsg;
    }
    catch (...)
    {
        m_strConnErrCode = g_strECFatal;
        m_strConnErrMsg = "Login Parse User Connection Data " + g_strEMFatal;
    }
    return false;
}

/// <summary>
/// 確認InDecreaseConnGW回傳資料
/// </summary>
/// <param name="strData">GW回傳資料</param>
void CSKLogin::ParseConnectionGWData(const string& strData)
{
    web::json::Parser parser;
    web::json::JsonObject::iterator iter1st;
    web::json::Node* __Node = NULL;

    try
    {
        /*
        {
            "Code": "000",
            "Message": "更新成功",
            "datas": "0:連線數增加成功"
        }
        */

        // Parse Json
        if (!parser.Parse(strData, &__Node))
            throw g_strEMParseJson;

        // Code
        iter1st = __Node->GetObj()->find("Code");
        if (iter1st != __Node->GetObj()->end())
        {
            if (iter1st->second->GetVlaueType() == web::json::kString)
            {
                m_strConnErrCode = *(iter1st->second->GetString());
                CUtility::TrimByRef(m_strConnErrCode);
            }
            else
                m_strConnErrCode = "";
        }

        // Message
        iter1st = __Node->GetObj()->find("Message");
        if (iter1st != __Node->GetObj()->end())
        {
            if (iter1st->second->GetVlaueType() == web::json::kString)
            {
                m_strConnErrMsg = *(iter1st->second->GetString());
                CUtility::TrimByRef(m_strConnErrMsg);
            }
            else
                m_strConnErrMsg = "";
        }

        // datas
        iter1st = __Node->GetObj()->find("datas");
        if (iter1st != __Node->GetObj()->end())
        {
            if (iter1st->second->GetVlaueType() == web::json::kString)
            {
                m_strConnErrData = *(iter1st->second->GetString());
                CUtility::TrimByRef(m_strConnErrData);
            }
            else
                m_strConnErrData = "";
        }

        // 當收到"000、999、101"帶LoginGW(第一次)回傳的訊息
        // 當從srvgw收到"403"且datas:"3:連線數達上線"時, 回傳給client 403:連線數已達上限 ，
        //如果是db出錯失敗，則會從srvgw收到"403"且datas:"4.連線數資加失敗
        if (m_strConnErrCode == "403"&& m_strConnErrData[0] == '3')
        {
            m_strErrCode = m_strConnErrCode;
            m_strErrMsg = g_strEMOverConnection;
        }
    }
    catch (string strMsg)
    {
        m_strConnErrCode = g_strECGWRet;
        m_strConnErrMsg = "Login Parse InDecreaseConn GW Data " + strMsg;
    }
    catch (...)
    {
        m_strConnErrCode = g_strECFatal;
        m_strConnErrMsg = "Login Parse InDecreaseConn GW Data " + g_strEMFatal;
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
void CSKLogin::SetReplyData()
{
    m_uncaReplyData[0] = ESCAPE_BYTE;
    m_uncaReplyData[1] = LOGIN_REP_BYTE;
    memcpy(m_uncaReplyData + 2, m_strErrCode.c_str(), m_strErrCode.size() > 4 ? 4 : m_strErrCode.size());
    memcpy(m_uncaReplyData + 6, m_strErrMsg.c_str(), m_strErrMsg.size() > 512 ? 512 : m_strErrMsg.size());
    memcpy(m_uncaReplyData + 518, m_strSessionID.c_str(), m_strSessionID.size() > 100 ? 100 : m_strSessionID.size());
}

/// <summary>
/// 發送回傳資料給Client
/// </summary>
void CSKLogin::SendClientCallback()
{
    struct TSKReplyData tskReplyData;
    memcpy(tskReplyData.uncaReplyData, m_uncaReplyData, sizeof(m_uncaReplyData));

    tskReplyData.tSKLoggerGW = m_tSKLoggerGW;
    tskReplyData.tvStart = m_tvStart;
    tskReplyData.tvSendT = m_tvSendT;

    if (m_shpClient != NULL)
        m_shpClient->SendReply("GW_LOGIN_REPLY", tskReplyData, sizeof(m_uncaReplyData));
}