#ifndef __SKCLIENT_H__
#define __SKCLIENT_H__

#include <string>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <openssl/ssl.h>
#include <condition_variable>
#include "SKInterface/ISKHeartbeatCallback.h"
#include "SKThread.h"
#include "SKHeartbeat.h"
#include "SKFormat.h"

using namespace std;

/// <summary>
/// Client 連線資料
/// </summary>
struct TSKClientAddrInfo
{
    SSL* ssl;
    int nSocket;
    char caIP[16];
    struct sockaddr_in ClientAddr;
};

/// <summary>
/// Client 狀態
/// </summary>
enum TSKClientStauts
{
    csNone,
    csStart,
    csLogining,     // 確保一條連線同時只會有一個登入
    csOnline,
    csGoingToClose,
    csOffline,
    csTerminated
};

class CSKClient: public CSKThread, public ISKHeartbeatCallback, public enable_shared_from_this<CSKClient>
{
public:
    CSKClient(struct TSKClientAddrInfo& ClientAddrInfo);
    virtual ~CSKClient();

    //bool SendReply(unsigned char uncSecondByte, const string& strReplyData);
    bool SendReply(const string& strWhat, struct TSKReplyData& replyData, int nSize);
    bool SendReConn(const string& strReConnType/*, const string& strReConnIP, const string& strReConnPort*/);
    void StartHeartbeat();
    void EnQueueReply(const string& strFuncName, struct TSKReplyData& replyData, int nSize);
    void SetUserData(const string& strSource, const string& strSessionID, const string& strLoginID);
    string GetLoginID();
    string GetIP();
    bool GetLogin();
    void SetStatus(TSKClientStauts csStatus);
    TSKClientStauts GetStatus();
    bool CheckShmIndex();
    int GetShmIndex();
    bool GetDecreaseConn();
    void SetDecreaseConn(bool bDecreaseConn);
    int GetClientSocketFd(); //取得ClientSocketFd
    void SetLoginResult(bool bResult);

    std::function<int(const string&)> get_shmidx_handler_;
    bool IsReplyQueueEmpty();

    void ShutDownFD();//20231108
protected:
    void Run();

    void OnHeartbeatRequest();
    void OnHeartbeatLost();
    void OnHeartbeatError(int nData, const char* pErrorMessage);

    bool RecvAll(const string& strWhat, unsigned char* pBuf, int nToRecv);
    bool SendAll(const string& strWhat, const unsigned char* pBuf, int nToSend);

private:
    void LoginCheck();
    void EnQueueLogger(TLogType tLogType, const string& strMsg);
    void EnQueueFunc(unsigned char uncaSecondByte, const unsigned char* pLoginData, const struct timeval& tvStart);
    bool CheckSessionID(const string& strSessionID);
    bool SendLoginReply(unsigned char uncaSecondByte, const string& strCode, const string& strMsg, const string& strSessionID);
    bool SendSessionReply(unsigned char uncaSecondByte, const string& strCode, const string& strMsg);
    bool SendOrderReply(unsigned char uncaSecondByte, const string& strCode, const string& strMsg,
        const string& strOrderNo, const string& strSeqNo, const string& strORKEY);
   // void ShutDownFD();
    void DispatchReplyData();

    struct TSKClientAddrInfo m_ClientAddrInfo;      //!< 連線資訊
    CSKHeartbeat* m_pHeartbeat;                     //!< Heartbeat類別
    TSKClientStauts m_csClientStatus;               //!< Client當前狀態
    queue< struct TSKClientReplyQ*> m_qReply;       //!< 回報 Queue
 
    mutex m_mtxClientStatusLock;                    //!< 設定Client狀態 Lock
    mutex m_mtxUserDataLock;                        //!< 設定UserData Lock
    mutex m_mtxHeartbeatLock;                       //!< 設定Heartbeat Lock
    mutex m_mtxShutDownLock;                        //!< 關閉SockFD Lock
    mutex m_mtxReply;                               //!< 回傳回報的 Lock

    string m_strSessionID;                          //!< 該客戶SessionID(驗證用)
    string m_strLoginID;                            //!< 該客戶LoginID(紀錄LOG用)
    string m_strSource;                             //!< 該客戶來源別
    string m_strIP;                                 //!< 該客戶IP
    int m_nLoginTimes;                              //!< 登入(SessionID驗證)次數, 最多3次
    int m_nShmIndex;                                //!< 該客戶在客戶Shm 的 Index
    int m_nSocketID;                                //!< 客戶 SocketID (不可變動)
    bool m_bLogin;                                  //!< 是否有登入成功過了
    bool m_bHeartbeatStart;                         //!< 是否啟用Heartbeat
    bool m_bDecreaseConn;                           //!< 是否減少過連線數了
    unsigned char m_uncaReplyBuf[1024];             //!< 回傳用buffer, 注意: 請確保同一時間只會有一個回傳
    
    condition_variable m_cvReply;                   //!< 回傳回報通知
    condition_variable m_cvCheckLogin;              //!< 連線後30秒內就斷線, 喚醒thread用
};
#endif
