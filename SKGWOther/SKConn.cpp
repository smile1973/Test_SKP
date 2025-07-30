#include "SKConn.h"
#include "../json.h"
#include "../Utility.h"

CSKConn::CSKConn(const string& strServer) : CSKBaseGW(strServer)
{
}

CSKConn::~CSKConn()
{
}

/// <summary>
/// 減少連線數
/// </summary>
/// <param name="strUserID">客戶ID/param>
/// <param name="nSocketID">客戶Socket ID</param>
void CSKConn::Run(const string& strUserID, int nSocketID)
{
    try
    {
        //////測試區遮罩/*
     /*   string temps = strUserID;
        for (int i = 1; i < 10; i++)
        {
            int temp = temps[i] - '0';
            temps[i] = temp + 65;
        }

        m_strPostJson = "{\"function\":\"DecreaseConn\",\"UserID\":\"" + temps + "\",\"SocketID\":\"" + to_string(nSocketID) +
            "\",\"ServerIP\":\"" + g_strLocalIp + "\"}";*/

        //////測試區遮罩*/

        m_strPostJson = "{\"function\":\"DecreaseConn\",\"UserID\":\"" + strUserID + "\",\"SocketID\":\"" + to_string(nSocketID) +
            "\",\"ServerIP\":\"" + g_strLocalIp + "\"}";


        m_hRequest.Post(m_strServer, m_strPostJson, m_ssResponse,"");
        ParseGWData(m_ssResponse.str());
    }
    catch (...)
    {
        m_strErrCode = g_strECFatal;
        m_strErrMsg = "DeConn Run " + g_strEMFatal;
    }

    EnQueueLogger();
}

/// <summary>
/// 確認GW回傳資料
/// </summary>
/// <param name="strData">GW回傳資料</param>
void CSKConn::ParseGWData(const string& strData)
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
            "datas": "0:連線數減少成功"
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
    }
    catch (string strMsg)
    {
        m_strErrCode = g_strECGWRet;
        m_strErrMsg = "DeConn Parse GW Data " + strMsg;
    }
    catch (...)
    {
        m_strErrCode = g_strECFatal;
        m_strErrMsg = "DeConn Parse GW Data " + g_strEMFatal;
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
void CSKConn::EnQueueLogger()
{
    try
    {
        if (m_strErrCode != "999")
            write_log_(e_ltServerData, "DecreaseConn Code:" + m_strErrCode + "; Msg:" + m_strErrMsg + "; PostJson:" + m_strPostJson, false);
        else
            write_log_(e_ltServerError, "DecreaseConn ErrorCode:" + m_strErrCode + "; ErrorMsg:" + m_strErrMsg + "; PostJson:" + m_strPostJson, false);
    }
    catch (...)
    {
    }
}

void CSKConn::SetReplyData()
{}

void CSKConn::SendClientCallback()
{}