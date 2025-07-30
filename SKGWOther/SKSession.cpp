#include "SKSession.h"
#include "../json.h"
#include "../Utility.h"
//#include "../SKGlobal.h"

CSKSession::CSKSession(const string& strServer, const string& strConnURL, const string& strLoginSalesURL) : CSKBaseGW(strServer)
{
    m_strLoginID = "";
    m_strSessionID = "";
    m_strSource = "";
    m_strConnPostJson = "";
    m_strConnErrCode = "";
    m_strConnErrMsg = "";
    m_strConnErrData = "";
    m_strServerConnectionURL = strConnURL;
    memset(m_uncaReplyData, 0, sizeof(m_uncaReplyData));
    m_tSKLoggerGW = {};
}

CSKSession::~CSKSession()
{
}

void CSKSession::SetLogger()
{
    m_tSKLoggerGW.strFunction.assign("AddFuncLog");
    m_tSKLoggerGW.strUserID.assign(m_strLoginID);
    m_tSKLoggerGW.strFunc.assign("Session");
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
/// Session驗證
/// </summary>
/// <param name="tSKSession">Session驗證資料</param>
/// <param name="tvStart">開始時間</param>
void CSKSession::Run(const TSKSession& tSKSession, const struct timeval& tvStart)
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
       /* if (nLogModel == 0)
        {*/
            struct timeval tvEnd[6];
            long nTime[6] = { 0 };

            gettimeofday(&tvEnd[0], NULL); // EnQ + DeQ

            bool bSuccess = SetPostData(tSKSession);
            if (bSuccess == false)
            {
                m_shpClient->SetLoginResult(false);
                SetReplyData();

                gettimeofday(&tvEnd[1], NULL); // SetPostData(false)
                nTime[0] = 1000000 * (tvEnd[0].tv_sec - tvStart.tv_sec) + (tvEnd[0].tv_usec - tvStart.tv_usec);
                nTime[1] = 1000000 * (tvEnd[1].tv_sec - tvEnd[0].tv_sec) + (tvEnd[1].tv_usec - tvEnd[0].tv_usec);
                m_strLogTime += to_string(nTime[0]) + "," + to_string(nTime[1])+",,,,,,";

                SetLogger();

                SendClientCallback();
            }
            else
            {
                gettimeofday(&tvEnd[1], NULL); // SetPostData(true)

                m_hRequest.Post(m_strServer, m_strPostJson, m_ssResponse,"Session");

                gettimeofday(&tvEnd[2], NULL); // Post GW

                nowtime = tvEnd[2].tv_sec;
                nowtm = localtime(&nowtime);
                strftime(tmbuf, sizeof tmbuf, "%Y-%m-%d %H:%M:%S", nowtm);
                snprintf(buf, sizeof buf, "%s.%06ld", tmbuf, tvStart.tv_usec);
                string strAfterGWTime = buf;

                ParseGWData(m_ssResponse.str());

                gettimeofday(&tvEnd[3], NULL); // Parse GW Data

                if (m_strErrCode == "000") // Session Verify Success
                {
                    m_shpClient->SetUserData(m_strSource, m_strSessionID, m_strLoginID);
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

        //    bool bSuccess = SetPostData(tSKSession);
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
        //        m_hRequest.Post(m_strServer, m_strPostJson, m_ssResponse);
        //        ParseGWData(m_ssResponse.str());

        //        if (m_strErrCode == "000") // Session Verify Success
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
        m_strErrMsg = "Session Run " + g_strEMFatal;
        SetReplyData();
        SetLogger();
        SendClientCallback();
    }
}

/// <summary>Session同步驗證</summary>
/// <param name="strLoginID">登入ID</param>
/// <param name="strSessionID">SessionID</param>
/// <param name="strSource">來源別</param>
/// <param name="strIP">外部IP</param>
/// <returns>驗證成功/失敗</returns>
bool CSKSession::RunSync(const string& strServer, const string& strLoginID, const string& strSessionID, const string& strSource, const string& strIP)
{
    try
    {
        stringstream ssResponse;
        ssResponse.clear();
        ssResponse.str("");
        string strPostJson = "";    //送GW訊息(Json格式)
        string strErrCode = "";     //GW回傳代碼

        // Json Start
        strPostJson = "{";
        strPostJson += "\"IDNO\":\"" + strLoginID + "\",";
        strPostJson += "\"TokenType\":\"2\",";
        strPostJson += "\"SessionID\":\"" + strSessionID + "\",";
        strPostJson += "\"ClientIP\":\"" + strIP + "\",";
        strPostJson += "\"Source\":\"" + strSource + "\"";
        strPostJson += "}";

        // Post
        CHttpRequest m_hRequestSync;
        m_hRequestSync.Post(strServer, strPostJson, ssResponse,"session");

        // ParseData
        web::json::Parser parser;
        web::json::JsonObject::iterator iter1st;
        web::json::Node* __Node = NULL;
        try
        {
            /*
            {
                "Code": "000",
                "Message": "權限驗證成功",
                "datas": null
            }
            */

            // Parse Json
            if (!parser.Parse(ssResponse.str(), &__Node))
                throw g_strEMParseJson;

            // Code
            iter1st = __Node->GetObj()->find("Code");
            if (iter1st != __Node->GetObj()->end())
            {
                if (iter1st->second->GetVlaueType() == web::json::kString)
                {
                    strErrCode = *(iter1st->second->GetString());
                    CUtility::TrimByRef(strErrCode);
                }
                else
                    strErrCode = "";
            }
        }
        catch (string strMsg) {}
        catch (...) {}

        ssResponse.clear();
        ssResponse.str("");

        if (__Node)
        {
            delete __Node;
            __Node = NULL;
        }

        // Session Verify Success
        if (strErrCode == "000")
            return true;
    }
    catch (...) {}
    return false;
}

/// <summary> 
/// 設定客戶 Shm Index 和 連線數增加確認
/// </summary>
void CSKSession::SetShmAndConn()
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
                m_hConnRequest.Post(m_strServerConnectionURL, m_strConnPostJson, m_ssResponse,"increase conn");
                ParseConnectionGWData(m_ssResponse.str());
            }

            // 紀錄LOG在本地端
            if (m_strConnErrCode != "999" )
                write_log_(e_ltServerData, "Session IncreaseConn Code:" + m_strConnErrCode +
                    "; Msg:" + m_strConnErrMsg + "; PostJson:" + m_strConnPostJson, false);
            else
                write_log_(e_ltServerError, "Session IncreaseConn ErrorCode:" + m_strConnErrCode +
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
void CSKSession::CleanData()
{
    CSKBaseGW::CleanData();

    m_strLoginID = "";
    m_strSessionID = "";
    m_strSource = "";
    m_strConnErrCode = "";
    m_strConnErrMsg = "";
    m_strConnErrData = "";
    memset(m_uncaReplyData, 0, sizeof(m_uncaReplyData));
}

/// <summary>
/// 確認User Session驗證資料
/// </summary>
/// <param name="tSKSession">Session驗證資料</param>
/// <returns>true/false</returns>
bool CSKSession::SetPostData(const TSKSession& tSKSession)
{
    try
    {
        m_strLoginID = CUtility::TrimCA(tSKSession.caIDNO, sizeof(tSKSession.caIDNO));
        if (m_strLoginID.empty())
            throw string("No IDNO");

        string strTokenType = CUtility::TrimCA(tSKSession.caTokenType, sizeof(tSKSession.caTokenType));
        if (strTokenType.empty())
            throw string("No TokenType");

        m_strSessionID = CUtility::TrimCASessionID(tSKSession.caSessionID, sizeof(tSKSession.caSessionID));
        if (m_strSessionID.empty())
            throw string("No SessionID");

        m_strSource = CUtility::TrimCA(tSKSession.caSource, sizeof(tSKSession.caSource));
        if (m_strSource.empty())
            throw string("No Source");

        // VersionNum
        //string strVersionNum(tSKSession.caVersionNum, sizeof(tSKSession.caVersionNum));
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
        m_strPostJson += "\",\"TokenType\":\"";
        m_strPostJson += strTokenType;
        m_strPostJson += "\",\"SessionID\":\"";
        m_strPostJson += m_strSessionID;
        m_strPostJson += "\",\"Source\":\"";
        m_strPostJson += m_strSource;
        m_strPostJson += "\",\"ClientIP\":\"";
        m_strPostJson += m_strIP;
        m_strPostJson += "\"}";

        /*m_strLogMsg += "{IDNO:";
        m_strLogMsg += m_strLoginID;
        m_strLogMsg += ",TokenType:";
        m_strLogMsg += strTokenType;
        m_strLogMsg += ",SessionID:";
        m_strLogMsg += m_strSessionID;
        m_strLogMsg += ",Source:";
        m_strLogMsg += m_strSource;
        m_strLogMsg += ",ClientIP:";
        m_strLogMsg += m_strIP;
        m_strLogMsg += "}";*/

        /*m_strLoginID.append(tSKSession.caIDNO, 10);
        CUtility::TrimByRef(m_strLoginID);
        if (m_strLoginID.empty())
            throw string("No IDNO");

        string strTokenType(tSKSession.caTokenType, 1);
        CUtility::TrimByRef(strTokenType);
        if (strTokenType.empty())
            throw string("No TokenType");

        m_strSessionID.append(tSKSession.caSessionID, 100);
        CUtility::TrimSessionIDByRef(m_strSessionID);
        if (m_strSessionID.empty())
            throw string("No SessionID");

        m_strSource.append(tSKSession.caSource, 2);
        CUtility::TrimByRef(m_strSource);
        if (m_strSource.empty())
            throw string("No Source");*/

        /*char caBuf[256];
        sprintf(caBuf, "{\"IDNO\":\"%s\",\"TokenType\":\"%s\",\"SessionID\":\"%s\",\"Source\":\"%s\",\"ClientIP\":\"%s\"}"
            , m_strLoginID.c_str()
            , strTokenType.c_str()
            , m_strSessionID.c_str()
            , m_strSource.c_str()
            , m_strIP.c_str());
        m_strPostJson = string(caBuf);*/

        return true;
    }
    catch (string strMsg)
    {
        m_strErrCode = g_strECUserData;
        m_strErrMsg = "Session Parse User Data " + strMsg;
    }
    catch (...)
    {
        m_strErrCode = g_strECFatal;
        m_strErrMsg = "Session Parse User Data " + g_strEMFatal;
    }
    return false;
}

/// <summary>
/// 確認GW回傳資料
/// </summary>
/// <param name="strData">回傳資料</param>
void CSKSession::ParseGWData(const string& strData)
{
    web::json::Parser parser;
    web::json::JsonObject::iterator iter1st;
    web::json::Node* __Node = NULL;

    try
    {
        /*
        {
            "Code": "000",
            "Message": "權限驗證成功",
            "datas": null
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
                m_strErrMsg = *(iter1st->second->GetString());
                CUtility::TrimByRef(m_strErrMsg);
            }
            else
                m_strErrMsg = "";
        }
    }
    catch (string strMsg)
    {
        m_strErrCode = g_strECGWRet;
        m_strErrMsg = "Session Parse GW Data " + strMsg;
    }
    catch (...)
    {
        m_strErrCode = g_strECFatal;
        m_strErrMsg = "Session Parse GW Data " + g_strEMFatal;
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
bool CSKSession::SetPostConnectionData()
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

        //////////////////////////測試區遮罩
      /*  for (int i = 1; i < 10; i++)
        {
            int temp = m_strLoginID[i] - '0';
            m_strLoginID[i] = temp + 65;
        }*/
        //////////////////////////

        m_strConnPostJson = "{\"function\":\"" + strFunc + "\",\"UserID\":\"" + m_strLoginID + "\",\"SocketID\":\"" + strSocketID +
            "\",\"ServerIP\":\"" + strServerIP + "\",\"ClientIP\":\"" + m_strIP + "\"}";
        return true;
    }
    catch (string strMsg)
    {
        m_strConnErrCode = g_strECUserData;
        m_strConnErrMsg = "Session Parse User Connection Data " + strMsg;
    }
    catch (...)
    {
        m_strConnErrCode = g_strECFatal;
        m_strConnErrMsg = "Session Parse User Connection Data " + g_strEMFatal;
    }
    return false;
}

/// <summary>
/// 確認InDecreaseConnGW回傳資料
/// </summary>
/// <param name="strData">GW回傳資料</param>
void CSKSession::ParseConnectionGWData(const string& strData)
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
        m_strConnErrMsg = "Session Parse InDecreaseConn GW Data " + strMsg;
    }
    catch (...)
    {
        m_strConnErrCode = g_strECFatal;
        m_strConnErrMsg = "Session Parse InDecreaseConn GW Data " + g_strEMFatal;
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
void CSKSession::SetReplyData()
{
    m_uncaReplyData[0] = ESCAPE_BYTE;
    m_uncaReplyData[1] = SESSION_REP_BYTE;
    memcpy(m_uncaReplyData + 2, m_strErrCode.c_str(), m_strErrCode.size() > 4 ? 4 : m_strErrCode.size());
    memcpy(m_uncaReplyData + 6, m_strErrMsg.c_str(), m_strErrMsg.size() > 512 ? 512 : m_strErrMsg.size());
}

/// <summary>
/// 發送回傳資料給Client
/// </summary>
void CSKSession::SendClientCallback()
{
    struct TSKReplyData tskReplyData;
    memcpy(tskReplyData.uncaReplyData, m_uncaReplyData, sizeof(m_uncaReplyData));

    tskReplyData.tSKLoggerGW = m_tSKLoggerGW;
    tskReplyData.tvStart = m_tvStart;
    tskReplyData.tvSendT = m_tvSendT;

    if (m_shpClient != NULL)
        m_shpClient->SendReply("GW_SESSION_REPLY", tskReplyData, sizeof(m_uncaReplyData));
}