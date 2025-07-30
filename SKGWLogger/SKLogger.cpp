#include "SKLogger.h"
#include "../json.h"
#include "../Utility.h"

CSKLogger::CSKLogger(const string& strServer) : CSKBaseGW(strServer)
{
}

CSKLogger::~CSKLogger()
{
}

void CSKLogger::Run(const TSKLoggerGW& tSKLoggerGW,const string& strThreadID)
{
    try
    {
        bool bSuccess = SetPostData(tSKLoggerGW);
        if (bSuccess)
        {
            m_hRequest.Post(m_strServer, m_strPostJson, m_ssResponse,strThreadID);
            ParseGWData(m_ssResponse.str());
            if (m_strErrCode != "000") //如果將log送到GW失敗則記錄在本地端
            {
                EnQueueLogger();
            }
        }
        else
        {
            EnQueueLogger();
        }
    }
    catch (...)
    {
        m_strErrCode = g_strECFatal;
        m_strErrMsg = "CSKLogger Run " + g_strEMFatal;
        EnQueueLogger();
    }
}

/// <summary>
/// 確認log資料
/// </summary>
/// <param name="tSKLoggerGW">log資料</param>
/// <returns>true/false</returns>
bool CSKLogger::SetPostData(const TSKLoggerGW& tSKLoggerGW)
{
    try
    {
        //測試區遮罩/*
        /*string strGW = tSKLoggerGW.strUserID;
       
        for (int i = 1; i < 10; i++)
        {
            int temp = tSKLoggerGW.strUserID[i] - '0';
            strGW[i] = temp + 65;
        }*/
        //測試區遮罩*/

        if (tSKLoggerGW.strFunction == "AddExceptionLog")
        {
            m_strPostJson = "{\"function\":\"" + tSKLoggerGW.strFunction + "\",\"UserID\":\"" + tSKLoggerGW.strUserID + "\",\"Func\":\"" + tSKLoggerGW.strFunc +
                "\",\"ExceptionMsg\":\"" + tSKLoggerGW.strExceptionMsg + "\",\"ClientIP\":\"" + tSKLoggerGW.strClientIP + "\",\"ServerIP\":\"" + tSKLoggerGW.strServerIP + "\"}";
        }
        else if (tSKLoggerGW.strFunction == "AddFuncLog")
        {
            m_strPostJson = "{\"function\":\"" + tSKLoggerGW.strFunction + "\",\"UserID\":\"" + tSKLoggerGW.strUserID + "\",\"Func\":\"" + tSKLoggerGW.strFunc +
                "\",\"Type\":\"" + tSKLoggerGW.strType + "\",\"UserData\":\"" + TrimUserData(tSKLoggerGW.strUserData) + "\",\"Code\":\"" + tSKLoggerGW.strCode +
                "\",\"Message\":\"" + tSKLoggerGW.strMessage + "\",\"ReplyData\":\"" + tSKLoggerGW.strReplyData + "\",\"UseTime\":\"" + tSKLoggerGW.strUseTime +
                "\",\"ClientIP\":\"" + tSKLoggerGW.strClientIP + "\",\"ServerIP\":\"" + tSKLoggerGW.strServerIP + "\",\"Info\":\"" + tSKLoggerGW.strInfo + "\",\"startTime\":\"" + tSKLoggerGW.strStartTime + "\",\"endTime\":\"" + tSKLoggerGW.strEndTime + "\",\"TimeDiff\":\"" + tSKLoggerGW.strTotalTime + "\"}";
        
           /* m_strPostJson = "{\"function\":\"" + tSKLoggerGW.strFunction + "\",\"UserID\":\"" + strGW + "\",\"Func\":\"" + tSKLoggerGW.strFunc +
                "\",\"Type\":\"" + tSKLoggerGW.strType + "\",\"UserData\":\"" + TrimUserData(tSKLoggerGW.strUserData) + "\",\"Code\":\"" + tSKLoggerGW.strCode +
                "\",\"Message\":\"" + tSKLoggerGW.strMessage + "\",\"ReplyData\":\"" + tSKLoggerGW.strReplyData + "\",\"UseTime\":\"" + tSKLoggerGW.strUseTime +
                "\",\"ClientIP\":\"" + tSKLoggerGW.strClientIP + "\",\"ServerIP\":\"" + tSKLoggerGW.strServerIP + "\",\"Info\":\"" + tSKLoggerGW.strInfo + "\",\"startTime\":\"" + tSKLoggerGW.strStartTime + "\",\"endTime\":\"" + tSKLoggerGW.strEndTime + "\",\"TimeDiff\":\"" + tSKLoggerGW.strTotalTime + "\"}";
        */
        }
        else if (tSKLoggerGW.strFunction == "AddOrderLog")
        {
            m_strPostJson = "{\"function\":\"" + tSKLoggerGW.strFunction + "\",\"UserID\":\"" + tSKLoggerGW.strUserID + "\",\"Func\":\"" + tSKLoggerGW.strFunc +
                "\",\"Type\":\"" + tSKLoggerGW.strType + "\",\"UserData\":\"" + TrimUserData(tSKLoggerGW.strUserData) + "\",\"Code\":\"" + tSKLoggerGW.strCode +
                "\",\"Message\":\"" + tSKLoggerGW.strMessage + "\",\"ORKEY\":\"" + tSKLoggerGW.strORKEY + "\",\"SeqNo\":\"" + tSKLoggerGW.strSeqNo +
                "\",\"OrderNo\":\"" + tSKLoggerGW.strOrderNo + "\",\"UseTime\":\"" + tSKLoggerGW.strUseTime + "\",\"ClientIP\":\"" + tSKLoggerGW.strClientIP +
                "\",\"ServerIP\":\"" + tSKLoggerGW.strServerIP + "\",\"Info\":\"" + tSKLoggerGW.strInfo + "\",\"startTime\":\"" + tSKLoggerGW.strStartTime + "\",\"endTime\":\"" + tSKLoggerGW.strEndTime + "\",\"TimeDiff\":\"" + tSKLoggerGW.strTotalTime + "\"}";
        
          /*  m_strPostJson = "{\"function\":\"" + tSKLoggerGW.strFunction + "\",\"UserID\":\"" + strGW + "\",\"Func\":\"" + tSKLoggerGW.strFunc +
                "\",\"Type\":\"" + tSKLoggerGW.strType + "\",\"UserData\":\"" + TrimUserData(tSKLoggerGW.strUserData) + "\",\"Code\":\"" + tSKLoggerGW.strCode +
                "\",\"Message\":\"" + tSKLoggerGW.strMessage + "\",\"ORKEY\":\"" + tSKLoggerGW.strORKEY + "\",\"SeqNo\":\"" + tSKLoggerGW.strSeqNo +
                "\",\"OrderNo\":\"" + tSKLoggerGW.strOrderNo + "\",\"UseTime\":\"" + tSKLoggerGW.strUseTime + "\",\"ClientIP\":\"" + tSKLoggerGW.strClientIP +
                "\",\"ServerIP\":\"" + tSKLoggerGW.strServerIP + "\",\"Info\":\"" + tSKLoggerGW.strInfo + "\",\"startTime\":\"" + tSKLoggerGW.strStartTime + "\",\"endTime\":\"" + tSKLoggerGW.strEndTime + "\",\"TimeDiff\":\"" + tSKLoggerGW.strTotalTime + "\"}";*/

        
        }
        else
        {
            throw string("strFunction not correct");
        }

        // 移除可能造成 SQL Injection 的字元
        TrimPostJson();

        return true;
    }
    catch (string strMsg)
    {
        m_strErrCode = g_strECUserData;
        m_strErrMsg = "CSKLogger SetPostData " + strMsg;
    }
    catch (...)
    {
        m_strErrCode = g_strECFatal;
        m_strErrMsg = "CSKLogger SetPostData " + g_strEMFatal;
    }

    return false;
}

/// <summary>
/// User敏感資料不記錄 (密碼, 簽章)
/// </summary>
/// <param name="strUserData">Json資料</param>
string CSKLogger::TrimUserData(const string& strUserData)
{
    try
    {
        if (strUserData.empty())
            return strUserData;

        string strData = strUserData;

        // 移除密碼
        size_t nStart = strData.find("\"Password\":");
        if (nStart != string::npos)
        {
            size_t nEnd = strData.find('"', nStart + 12);
            if (nEnd != string::npos)
            {
                strData.erase(nStart, nEnd - nStart + 1);
            }
        }

        // 移除簽章密文(太大)
        nStart = strData.find("\"Sign\":");
        if (nStart != string::npos)
        {
            size_t nEnd = strData.find('"', nStart + 8);
            if (nEnd != string::npos)
            {
                strData.erase(nStart, nEnd - nStart + 1);
            }
        }

        // 移除所有雙引號
        string::iterator iter;
        for (iter = strData.begin(); iter < strData.end(); iter++)
        {
            if (*iter == '"')
            {
                /*strData.erase(iter);
                iter--;*/
                iter = strData.insert(iter, '\\');
                iter++;
            }
        }

        return strData;
    }
    catch (...)
    {
    }
    return strUserData;
}

/// <summary>
/// 移除可能會造成 SQL Injection 的字元
/// </summary>
void CSKLogger::TrimPostJson()
{
    try
    {
        if (m_strPostJson.empty())
            return;

        // 移除所有單引號和減號
        string::iterator iter;
        for (iter = m_strPostJson.begin(); iter < m_strPostJson.end(); iter++)
        {
            if (*iter == '\'' || *iter == '-')
            {
                m_strPostJson.erase(iter);
                iter--;
            }
        }
    }
    catch (...)
    {
    }
}

/// <summary>
/// 確認回傳資料
/// </summary>
/// <param name="strData">回傳資料</param>
void CSKLogger::ParseGWData(const string& strData)
{
    web::json::Parser parser;
    web::json::JsonObject::iterator iter1st;
    web::json::Node* __Node = NULL;

    try
    {
        /*
        {
            "Code": "000",
            "Message": "新增成功",
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
        m_strErrMsg = "CSKLogger Parse GW Data " + strMsg;
    }
    catch (...)
    {
        m_strErrCode = g_strECFatal;
        m_strErrMsg = "CSKLogger Parse GW Data " + g_strEMFatal;
    }

    if (__Node)
    {
        delete __Node;
        __Node = NULL;
    }
}

/// <summary>
/// 紀錄LOG
/// </summary>
void CSKLogger::EnQueueLogger()
{
    try
    {
         m_strLogMsg = "Logger ErrorCode:" + m_strErrCode + "; ErrorMsg:" + m_strErrMsg + "; PostJson:" + m_strPostJson;

         if (write_local_log_ != NULL)
             write_local_log_(e_ltServerError, m_strLogMsg);
    }
    catch (...)
    {
    }
}

void CSKLogger::SetReplyData(){}
void CSKLogger::SendClientCallback() {}