#ifndef __SKSESSION_H__
#define __SKSESSION_H__

#include "../SKBaseGW.h"

class CSKSession : public CSKBaseGW
{
public:
    CSKSession(const string& strServer, const string& strConnURL, const string& strLoginSalesURL);
    virtual ~CSKSession();
    void Run(const TSKSession& tSKSession, const struct timeval& tvStart);
    static bool RunSync(const string& strServer, const string& strLoginID, const string& strSessionID, const string& strSource, const string& strIP);
    virtual void CleanData();
    std::function<bool(TLogType, const string&, bool)> write_log_;
    void SetLogger();
private:
    bool SetPostData(const TSKSession& tSKSession);
    void ParseGWData(const string& strData) override;
    bool SetPostConnectionData();
    void ParseConnectionGWData(const string& strData);
    void SetShmAndConn();
    void SetReplyData() override;
    void SendClientCallback() override;

    // no need clean
    string m_strServerConnectionURL;    //!< InDecreaseConnURL
    CHttpRequest m_hConnRequest;        //!< 連線數專用Http (因為Hostname和登入的不一樣, 所以Curl不會複用長連線, 那不如建一個專門的長連線)

    // need clean
    string m_strLoginID;                //!< Client LoginID (User自帶)
    string m_strSessionID;              //!< Client SessionID (User自帶)
    string m_strSource;                 //!< Client 來源別 (User自帶)
    string m_strConnPostJson;           //!< 新增連線數 Json Data
    string m_strConnErrCode;            //!< 新增連線數 GW 代碼
    string m_strConnErrMsg;             //!< 新增連線數 GW 訊息
    string m_strConnErrData;             //!< 新增連線數 GW datas欄位
    unsigned char m_uncaReplyData[PACKET_HEADER_LENGTH + SESSION_REP_LENGTH]; //!< 回傳Client訊息

    TSKLoggerGW m_tSKLoggerGW;
    struct timeval m_tvStart;
    struct timeval m_tvSendT;
    string m_strStartT;
};

#endif