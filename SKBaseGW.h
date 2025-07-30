#ifndef __SKBASEGW_H__
#define __SKBASEGW_H__

#include "HttpRequest.h"
#include "SKClient.h"
#include "SKGlobal.h"
#include <string>
#include <sstream>
using namespace std;

class CSKBaseGW
{
public:
    CSKBaseGW(const string& strServer)
    {
        m_strServer = strServer;
        m_strIP = "";
        m_strErrCode = "";
        m_strErrMsg = "";
        m_strPostJson = "";
        m_strLogMsg = "";
        m_strLogTime = "";
        m_ssResponse.clear();
        m_ssResponse.str("");
        m_shpClient = NULL;
    };
    
    virtual ~CSKBaseGW()
    {
        m_shpClient = NULL;
    };

    /// <summary>
    /// 設定回傳Client
    /// </summary>
    /// <param name="shpClient">回傳Client</param>
    virtual void SetCallback(shared_ptr<CSKClient>& shpClient)
    {
        m_shpClient = shpClient;
    }

    /// <summary>
    /// 設定外部IP
    /// </summary>
    /// <param name="strIP">外部IP</param>
    virtual void SetClientIP(const string& strIP)
    {
        m_strIP = strIP;
    }

    /// <summary>
    /// 清除資料
    /// </summary>
    virtual void CleanData()
    {
        m_strIP = "";
        m_strErrCode = "";
        m_strErrMsg = "";
        m_strPostJson = "";
        m_strLogMsg = "";
        m_strLogTime = "";
        m_ssResponse.clear();
        m_ssResponse.str("");
        m_shpClient = NULL;
    }
    std::function<bool(TLogType, const string&, bool)> write_log_;
    std::function<bool(const TSKLoggerGW&)> write_log_gw_;

protected:
    virtual void ParseGWData(const string& strData) = 0;
    virtual void SetReplyData() = 0;
    //virtual void EnQueueLogger() = 0;
    virtual void SendClientCallback() = 0;

    // Non-const Variable
    shared_ptr<CSKClient> m_shpClient;  //!< Client pointer (CSKClient自帶)
    string m_strIP;                     //!< Client 外部IP (CSKClient自帶)
    string m_strErrCode;                //!< 錯誤代碼
    string m_strErrMsg;                 //!< 錯誤訊息
    string m_strPostJson;               //!< 發送GW訊息 (Json格式)
    string m_strLogMsg;                 //!< LOG訊息
    string m_strLogTime;                //!< LOG紀錄時間
    stringstream m_ssResponse;          //!< 回傳值

    // Const Variable
    CHttpRequest m_hRequest;            //!< HTTP類別
    string m_strServer;                 //!< Server URL
};
#endif