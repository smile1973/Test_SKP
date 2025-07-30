#include "SKClient.h" 
#include "SKGlobal.h"
#include "SKServers.h"
#include "Utility.h"
#include <cstring>
#include <unistd.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <fstream>
#include <sstream>
#include <iomanip>

// SSL Docs
// SSL_get_error https://www.openssl.org/docs/man1.0.2/man3/SSL_get_error.html
// SSL_read https://www.openssl.org/docs/man1.0.2/man3/SSL_read.html
// SSL_write https://www.openssl.org/docs/man1.0.2/man3/SSL_write.html
// SSL_shutdown https://www.openssl.org/docs/man1.0.2/man3/SSL_shutdown.html
// SSL_free https://www.openssl.org/docs/man1.0.2/man3/SSL_free.html

extern void FprintfStderrLog(const string& strMsg);
extern void FprintfStderrLog(const timeval& tvStart, const timeval& tvEnd);
extern void FprintfStderrLog(const char* pCause, int nError, int nData, const char* pFile = NULL, int nLine = 0,
    unsigned char* pMessage1 = NULL, int nMessage1Length = 0, 
    unsigned char* pMessage2 = NULL, int nMessage2Length = 0);
extern int Atoi(char* ptr, int n);

extern CSKServers* g_pServers;

CSKClient::CSKClient(struct TSKClientAddrInfo &ClientAddrInfo)
{
    memset(&m_ClientAddrInfo, 0, sizeof(struct TSKClientAddrInfo));
    memcpy(&m_ClientAddrInfo, &ClientAddrInfo, sizeof(struct TSKClientAddrInfo));

    m_pHeartbeat = NULL;
    m_csClientStatus = csNone;
    m_strLoginID = "";
    m_strIP = string(m_ClientAddrInfo.caIP);
    m_nSocketID = m_ClientAddrInfo.nSocket;
    m_nLoginTimes = 0;
    m_bLogin = false;
    m_bHeartbeatStart = false;
    m_bDecreaseConn = false;
    m_nShmIndex = -1;

    //Start();
}

CSKClient::~CSKClient() 
{
    if (m_pHeartbeat != NULL)
    {
        delete m_pHeartbeat;
        m_pHeartbeat = NULL;
    }

    ShutDownFD();

    if (m_ClientAddrInfo.ssl != NULL)
    {
        //if (m_bIsNeedSSLShutdown)
        //SSL_shutdown(m_ClientAddrInfo.ssl);
        SSL_free(m_ClientAddrInfo.ssl);
        m_ClientAddrInfo.ssl = NULL;
    }
}

void CSKClient::Run()
{
    try
    {
        string strSessionID = "";
        string strORKEY = "";
        unsigned char uncaEscapeBuf[2];
        unsigned char uncaReplyTransNo;
        unsigned char uncaRecvBuf[4096];
        int nLength = 0;
        struct timeval tvStart;
        SetStatus(csStart);

        if (g_pServers == NULL)
        {
            SetStatus(csTerminated);
            return;
        }

        if (get_shmidx_handler_ == NULL)
        {
            SetStatus(csTerminated);
            return;
        }

        try
        {
            m_pHeartbeat = new CSKHeartbeat(this);
            m_pHeartbeat->SetTimeInterval(HEARTBEAT_TIME_INTERVAL);
            //m_pHeartbeat->Start();
        }
        catch (...)
        {
            SetStatus(csTerminated);
            return;
        }

        thread tCheckLogin(&CSKClient::LoginCheck, this);
        tCheckLogin.detach();

        thread tReply(&CSKClient::DispatchReplyData, this);
        tReply.detach();

        try
        {
            while (m_csClientStatus != csOffline && m_csClientStatus != csTerminated)
            {
                // Clean Data
                strSessionID = "";
                strORKEY = "";

                memset(uncaEscapeBuf, 0, sizeof(uncaEscapeBuf));
                bool bRecvAll = RecvAll("RECV_ESC", uncaEscapeBuf, 2); //esc(1) + trans_no(1)
                if (bRecvAll == false)
                {
                    EnQueueLogger(e_ltClientData, "RECV_ESC_ERROR");
                    continue;
                }

                // Recv Block 打開時判斷狀態
                if (m_csClientStatus == csOffline)
                    break;

                // Record now time
                gettimeofday(&tvStart, NULL);

                // 收到錯誤封包頭直接斷線, 避免被攻擊佔用資源
                if (uncaEscapeBuf[0] != ESCAPE_BYTE)
                {
                    EnQueueLogger(e_ltClientError, "ESCAPE_BYTE_ERROR");
                    SetStatus(csOffline);
                    continue;
                }
                else
                {
                    uncaReplyTransNo = '\0';
                    nLength = 0;
                    string strFuncName = "";

                    switch (uncaEscapeBuf[1])
                    {
                    case LOGIN_BYTE:
                        strFuncName = "RECV_LOGIN";
                        uncaReplyTransNo = LOGIN_REP_BYTE;
                        nLength = sizeof(struct TSKLogin);
                        break;
                    case SESSION_BYTE:
                        strFuncName = "RECV_SESSION";
                        uncaReplyTransNo = SESSION_REP_BYTE;
                        nLength = sizeof(struct TSKSession);
                        break;
                    case HEARTBEAT_REQ_BYTE:
                        strFuncName = "RECV_REQ_HEARTBEAT";
                        nLength = strlen(g_pHeartbeatRequestMessage);
                        break;
                    case HEARTBEAT_REP_BYTE:
                        strFuncName = "RECV_REP_HEARTBEAT";
                        nLength = strlen(g_pHeartbeatReplyMessage);
                        break;
                    case TSORDER_BYTE:
                        strFuncName = "RECV_TSORDER";
                        uncaReplyTransNo = TSORDER_REP_BYTE;
                        nLength = sizeof(struct TSKTSOrder);
                        break;
                    case TSALTER_BYTE:
                    case PRE_TSALTER_BYTE:
                        strFuncName = "RECV_TSALTER";
                        uncaReplyTransNo = TSALTER_REP_BYTE;
                        nLength = sizeof(struct TSKTSOrder);
                        break;
                    case TFORDER_BYTE:
                        strFuncName = "RECV_TFORDER";
                        uncaReplyTransNo = TFORDER_REP_BYTE;
                        nLength = sizeof(struct TSKTFOrder);
                        break;
                    case TFALTER_BYTE:
                    case PRE_TFALTER_BYTE:
                        strFuncName = "RECV_TFALTER";
                        uncaReplyTransNo = TFALTER_REP_BYTE;
                        nLength = sizeof(struct TSKTFOrder);
                        break;
                    case OFORDER_BYTE:
                    case PRE_OFORDER_BYTE:
                        strFuncName = "RECV_OFORDER";
                        uncaReplyTransNo = OFORDER_REP_BYTE;
                        nLength = sizeof(struct TSKOFOrder);
                        break;
                    case OSORDER_BYTE:
                    case PRE_OSORDER_BYTE:
                        strFuncName = "RECV_OSORDER";
                        uncaReplyTransNo = OSORDER_REP_BYTE;
                        nLength = sizeof(struct TSKOSOrder);
                        break;
                    case OSBORDER_BYTE:
                        strFuncName = "RECV_OSBORDER";
                        uncaReplyTransNo = OSBORDER_REP_BYTE;
                        nLength = sizeof(struct TSKOSOrder);
                        break;
                    default:
                        EnQueueLogger(e_ltClientError, "TRANSNO_BYTE_ERROR");
                        continue;
                    }

                    memset(uncaRecvBuf, 0, sizeof(uncaRecvBuf));
                    bRecvAll = RecvAll(strFuncName, uncaRecvBuf, nLength);
                    if (bRecvAll == false)
                    {
                        EnQueueLogger(e_ltClientError, strFuncName + "_ERROR");
                        continue;
                    }
                }

                switch (uncaEscapeBuf[1])
                {
                case LOGIN_BYTE:
                    if (m_nLoginTimes > 2)
                    {
                        SendLoginReply(uncaReplyTransNo, g_strECLogin, g_strEMLoginMax, "");
                    }
                    else if (m_csClientStatus == csOnline)
                    {
                        SendLoginReply(uncaReplyTransNo, g_strECLogin, g_strEMLoginMore, "");
                    }
                    else if (m_csClientStatus == csLogining)
                    {
                        SendLoginReply(uncaReplyTransNo, g_strECLogin, g_strEMLoginIng, "");
                    }
                    else if (m_csClientStatus == csGoingToClose)
                    {
                        SendLoginReply(uncaReplyTransNo, g_strECGoingToClose, g_strEMGoingToClose, "");
                    }
                    else
                    {
                        SetStatus(csLogining);
                        m_nLoginTimes += 1;
                        EnQueueFunc(uncaEscapeBuf[1], uncaRecvBuf, tvStart);
                    }
                    break;
                case SESSION_BYTE:
                    if (m_nLoginTimes > 2)
                    {
                        SendSessionReply(uncaReplyTransNo, g_strECLogin, g_strEMLoginMax);
                    }
                    else if (m_csClientStatus == csOnline)
                    {
                        SendSessionReply(uncaReplyTransNo, g_strECLogin, g_strEMLoginMore);
                    }
                    else if (m_csClientStatus == csLogining)
                    {
                        SendSessionReply(uncaReplyTransNo, g_strECLogin, g_strEMLoginIng);
                    }
                    else if (m_csClientStatus == csGoingToClose)
                    {
                        SendLoginReply(uncaReplyTransNo, g_strECGoingToClose, g_strEMGoingToClose, "");
                    }
                    else
                    {
                        SetStatus(csLogining);
                        m_nLoginTimes += 1;
                        EnQueueFunc(uncaEscapeBuf[1], uncaRecvBuf, tvStart);
                    }
                    break;
                case TSORDER_BYTE:
                case TSALTER_BYTE:
                case TFORDER_BYTE:
                case TFALTER_BYTE:
                case OFORDER_BYTE:
                case OSORDER_BYTE:
                case OSBORDER_BYTE:
                case PRE_TSALTER_BYTE:
                case PRE_TFALTER_BYTE:
                case PRE_OFORDER_BYTE:
                case PRE_OSORDER_BYTE:
                    strSessionID = CUtility::TrimCASessionID((const char*)uncaRecvBuf, 100);
                    strORKEY = CUtility::TrimCA((const char*)uncaRecvBuf + 100, 100);
                    if (m_csClientStatus != csOnline)
                    {
                        if (m_csClientStatus == csGoingToClose)
                        {
                            SendOrderReply(uncaReplyTransNo, g_strECGoingToClose, g_strEMGoingToClose, "", "", strORKEY);
                            EnQueueLogger(e_ltServerData, "Server GoingToClose, return fail: LoginID= "+ m_strLoginID +" ,ORKEY= " +strORKEY);
                        }
                        else
                            SendOrderReply(uncaReplyTransNo, g_strECLogin, g_strEMLoginFirst, "", "", strORKEY);
                    }
                    else
                    {
                        if (CheckSessionID(strSessionID))
                        {
                            SendOrderReply(uncaReplyTransNo, g_strECPending, g_strEMPending, "", "", strORKEY);

                            EnQueueFunc(uncaEscapeBuf[1], uncaRecvBuf, tvStart);
                        }
                        else
                        {
                            SendOrderReply(uncaReplyTransNo, g_strECSessionID, g_strEMSessionID, "", "", strORKEY);
                        }
                    }
                    break;
                case HEARTBEAT_REQ_BYTE:
                    if (memcmp(uncaRecvBuf, g_pHeartbeatRequestMessage, 4) == 0)
                    {
                        struct TSKReplyData tReplyData;

                        memset(tReplyData.uncaReplyData, 0, sizeof(tReplyData.uncaReplyData));
                        memcpy(tReplyData.uncaReplyData, g_uncaHeaetbeatReplyBuf, sizeof(g_uncaHeaetbeatReplyBuf));

                        EnQueueReply("HEARTBEAT_REPLY", tReplyData, sizeof(g_uncaHeaetbeatReplyBuf));

                        //FprintfStderrLog("GET_CLIENT_HTBT", 0, 0, NULL, 0, m_uncaLoginID, sizeof(m_uncaLoginID));
                        /*bool bSendAll = SendAll("HEARTBEAT_REPLY", g_uncaHeaetbeatReplyBuf, sizeof(g_uncaHeaetbeatReplyBuf));
                        if (bSendAll == false)
                        {
                            EnQueueLogger(e_ltClientError, "HEARTBEAT_REPLY_ERROR");
                        }*/
                    }
                    else
                    {
                        EnQueueLogger(e_ltClientError, "HEARTBEAT_REQ_DATA_WRONG_ERROR");
                    }
                    break;
                case HEARTBEAT_REP_BYTE:
                    if (memcmp(uncaRecvBuf, g_pHeartbeatReplyMessage, 4) == 0)
                    {
                        //FprintfStderrLog("GET_CLIENT_HTBR", 0, 0, NULL, 0, m_uncaLoginID, sizeof(m_uncaLoginID));
                    }
                    else
                    {
                        EnQueueLogger(e_ltClientError, "HEARTBEAT_REP_DATA_WRONG_ERROR");
                    }
                    break;
                }
            }
        }
        catch (exception& e)
        {
            EnQueueLogger(e_ltClientError, "SKClient_Run while Error, exception: " + string(e.what()));
        }
        catch (...)
        {
            EnQueueLogger(e_ltClientError, "SKClient_Run while Error.");
        }
        // 砍掉Heartbeat物件
        if (m_pHeartbeat != NULL && m_bHeartbeatStart)
            m_pHeartbeat->TriggerTerminateEvent();

        // 如果是30秒內斷線, 這個喚醒才有作用
        m_cvCheckLogin.notify_all();
        
        // 停止送回報
        m_cvReply.notify_all();

        // 減少連線數: 判斷是否有成功登入過, m_bDecreaseConn 設置以避免在登入中斷線沒減少到連線數, 每分鐘移除SKClient時還能補減
        if (m_bLogin == true && m_bDecreaseConn == false)
        {
            m_bDecreaseConn = true;
            g_pServers->EnQueueDeConn(m_strLoginID, m_nSocketID);
        }
        //this_thread::sleep_for(chrono::seconds(3));  //20231108
        this_thread::sleep_for(chrono::seconds(1));
        SetStatus(csTerminated);
    }
    catch (exception& e)
    {
        EnQueueLogger(e_ltClientError, "SKClient_Run Error, exception: " + string(e.what()));
    }
    catch (...)
    {
        EnQueueLogger(e_ltClientError, "SKClient_Run Error.");
    }
}

/// <summary>
/// SKHeartbeat Callback
/// </summary>
void CSKClient::OnHeartbeatLost()
{
    EnQueueLogger(e_ltClientError, "HEARTBEAT_LOST");

    SetStatus(csOffline);
    ShutDownFD();
}

/// <summary>
/// SKHeartbeat Callback
/// </summary>
void CSKClient::OnHeartbeatRequest() //20231018
{
    struct TSKReplyData tReplyData;
    
    memset(tReplyData.uncaReplyData, 0, sizeof(tReplyData.uncaReplyData));
    memcpy(tReplyData.uncaReplyData, g_uncaHeaetbeatRequestBuf, sizeof(g_uncaHeaetbeatRequestBuf));

    EnQueueReply("HEARTBEAT_REQUEST", tReplyData, sizeof(g_uncaHeaetbeatRequestBuf));

    /*bool bSendAll = SendAll("HEARTBEAT_REQUEST", g_uncaHeaetbeatRequestBuf, sizeof(g_uncaHeaetbeatRequestBuf));
    if(bSendAll == false)
    {
        EnQueueLogger(e_ltClientError, "HEARTBEAT_REQUEST_ERROR");
    }*/
}

/// <summary>
/// SKHeartbeat Callback
/// </summary>
void CSKClient::OnHeartbeatError(int nData, const char* pErrorMessage)
{
    if(string(pErrorMessage)== "HEARTBEAT_TERMINATE")
        EnQueueLogger(e_ltClientData, "HEARTBEAT_ERROR " + string(pErrorMessage));
    else
    EnQueueLogger(e_ltClientError, "HEARTBEAT_ERROR " + string(pErrorMessage));
}

/// <summary>
/// Socket 接收資料
/// </summary>
/// <param name="strWhat">接收中的功能</param>
/// <param name="pBuf">接收用的buffer</param>
/// <param name="nToRecv">需要接收的長度</param>
/// <returns>接收結果</returns>
bool CSKClient::RecvAll(const string& strWhat, unsigned char* pBuf, int nToRecv)
{
    if (m_ClientAddrInfo.ssl == NULL)
    {
        SetStatus(csOffline);
        EnQueueLogger(e_ltClientError, "[" + strWhat + "] RECV_SK_ERROR: SSL NULL");
        return false;
    }

    int nRecv = 0;
    int nRecved = 0;

    do
    {
        nToRecv -= nRecv;
        
        if (m_csClientStatus != csOffline && m_csClientStatus != csTerminated)
        {
            nRecv = SSL_read(m_ClientAddrInfo.ssl, pBuf + nRecved, nToRecv);
        }
        else
        {
            EnQueueLogger(e_ltClientData, "[" + strWhat + "] RECV_OFFLINE_ERROR");
            break;
        }

        if(nRecv > 0)
        {
            if(m_pHeartbeat)
                m_pHeartbeat->TriggerGetReplyEvent();
            else
            {
                EnQueueLogger(e_ltClientError, "[" + strWhat + "] RECV_HEARTBEAT_NULL_ERROR");
            }

            //FprintfStderrLog(strWhat.c_str(), 0, 0, __FILE__, 0, m_uncaLoginID, sizeof(m_uncaLoginID), pBuf + nRecved, nRecv);
            nRecved += nRecv;
        }
        else if(nRecv <= 0)
        {
            SetStatus(csOffline);
            // if SSL_get_error returns these errors, SSL should not use SSL_shutdown.
            /*int nSSLError = SSL_get_error(m_ClientAddrInfo.ssl, nRecv);
            if (nSSLError == SSL_ERROR_SYSCALL || nSSLError == SSL_ERROR_SSL) 
            {
                m_bIsNeedSSLShutdown = false;
            }*/

            EnQueueLogger(e_ltClientData, "[" + strWhat + "] RECV_SK_ERROR : " + strerror(errno) + " " + to_string(nRecv));
            break;
        }
        else
        {
            SetStatus(csOffline);

            EnQueueLogger(e_ltClientError, "[" + strWhat + "] RECV_SK_ELSE_ERROR : " + strerror(errno));
            break;
        }
    }
    while(nRecv != nToRecv);

    return nRecv == nToRecv ? true : false;
}

/// <summary>
/// Socket 發送資料
/// </summary>
/// <param name="strWhat">發送中的功能</param>
/// <param name="pBuf">發送用的buffer</param>
/// <param name="nToSend">需要發送的長度</param>
/// <returns>發送結果</returns>
bool CSKClient::SendAll(const string& strWhat, const unsigned char* pBuf, int nToSend)
{
    if (pBuf == NULL)
        return false;
    if (m_ClientAddrInfo.ssl == NULL)
    {
        SetStatus(csOffline);
        EnQueueLogger(e_ltClientError, "[" + strWhat + "] SEND_SK_ERROR: SSL NULL");
        return false;
    }

    int nSend = 0;
    int nSended = 0;

    do
    {
        nToSend -= nSend;

        if(m_csClientStatus != csOffline && m_csClientStatus != csTerminated)
        {
            nSend = SSL_write(m_ClientAddrInfo.ssl, pBuf + nSended , nToSend);
        }
        else
        {
            EnQueueLogger(e_ltClientData, "[" + strWhat + "] SEND_OFFLINE_ERROR");
            break;
        }
        
        
        if (nSend <= 0)
        {
            SetStatus(csOffline);
            // if SSL_get_error returns these errors, SSL should not use SSL_shutdown.
            /*int nSSLError = SSL_get_error(m_ClientAddrInfo.ssl, nSend);
            if (nSSLError == SSL_ERROR_SYSCALL || nSSLError == SSL_ERROR_SSL)
            {
                m_bIsNeedSSLShutdown = false;
            }*/

            EnQueueLogger(e_ltClientError, "[" + strWhat + "] SEND_SK_ERROR: " + strerror(errno) + " " + to_string(nSend));
            break;
        }
        if(nSend == nToSend)
        {
            //FprintfStderrLog(strWhat.c_str(), 0, 0, __FILE__, __LINE__, m_uncaLoginID, sizeof(m_uncaLoginID), (unsigned char*)pBuf + nSended, nSend);
            break;
        }
        else if(nSend < nToSend)
        {
            //FprintfStderrLog(strWhat.c_str(), -1, 0, __FILE__, __LINE__, m_uncaLoginID, sizeof(m_uncaLoginID), (unsigned char*)pBuf + nSended, nSend);
            nSended += nSend;
        }
        else
        {
            EnQueueLogger(e_ltClientError, "[" + strWhat + "] SEND_SK_ELSE_ERROR : " + strerror(errno));
            break;
        }
    }
    while(nSend != nToSend);

    return nSend == nToSend ? true : false;
}

/// <summary>
/// 外部發送回傳資料
/// </summary>
/// <param name="strWhat">功能名稱</param>
/// <param name="unpcReplyData">發送資料</param>
/// <param name="nSize">資料長度</param>
/// <returns>發送結果</returns>
bool CSKClient::SendReply(const string& strWhat, struct TSKReplyData& replydata, int nSize)
{
    try
    {
        // 斷線就不用寫進來啦~ (外部打過來的, 有可能已經斷線了)
        if (m_csClientStatus == csOffline || m_csClientStatus == csTerminated)
        {
            EnQueueLogger(e_ltClientError, strWhat + "_ERROR: SendReply Offline");
            return false;
        }

        if (replydata.uncaReplyData == NULL)
        {
            EnQueueLogger(e_ltClientError, strWhat + "_ERROR: DATA NULL");
            return false;
        }

        EnQueueReply(strWhat, replydata, nSize);

        /*// 以防萬一, 還是複製一下好了
        unsigned char uncpSendBuf[nSize];
        memset(uncpSendBuf, 0, sizeof(uncpSendBuf));
        memcpy(uncpSendBuf, unpcReplyData, sizeof(uncpSendBuf));

        bool bSendAll = SendAll(strWhat, uncpSendBuf, nSize);
        if (!bSendAll)
        {
            EnQueueLogger(e_ltClientError, strWhat + "_ERROR");
        }*/
        return true;
    }
    catch (...)
    {
    }
    return false;
}

/// <summary>
/// 發送重新連線
/// </summary>
/// <param name="strReConnType">重新連線原因</param>
/// <param name="strReConnIP">重新連線IP/HostName</param>
/// <param name="strReConnPort">重新連線Port</param>
/// <returns>發送結果</returns>
bool CSKClient::SendReConn(const string& strReConnType/*, const string& strReConnIP, const string& strReConnPort*/)
{
    try
    {
        unsigned char uncpSendBuf[PACKET_HEADER_LENGTH + RECONN_LENGTH];
        memset(uncpSendBuf, 0, sizeof(uncpSendBuf));
        uncpSendBuf[0] = ESCAPE_BYTE;
        uncpSendBuf[1] = RECONN_BYTE;
        memcpy(uncpSendBuf + 2, strReConnType.c_str(), strReConnType.size() > 1 ? 1 : strReConnType.size());
        //memcpy(uncpSendBuf + 3, strReConnIP.c_str(), strReConnIP.size() > 50 ? 50 : strReConnIP.size());
        //memcpy(uncpSendBuf + 53, strReConnPort.c_str(), strReConnPort.size() > 5 ? 5 : strReConnPort.size());

        struct TSKReplyData tReplyData;

        memset(tReplyData.uncaReplyData, 0, sizeof(tReplyData.uncaReplyData));
        memcpy(tReplyData.uncaReplyData, uncpSendBuf, sizeof(uncpSendBuf));

        EnQueueReply("SEND_RECONN", tReplyData, sizeof(uncpSendBuf));

        /*bool bSendAll = SendAll("SEND_RECONN", uncpSendBuf, sizeof(uncpSendBuf));
        if (!bSendAll)
        {
            EnQueueLogger(e_ltClientError, "SEND_RECONN_ERROR");
        }
        return bSendAll;*/
            }
    catch (...)
    {
    }
    return false;
}

/// <summary>
/// SKClient內部回傳Login結果
/// </summary>
/// <param name="uncaSecondByte">TransNo</param>
/// <param name="strCode">代碼</param>
/// <param name="strMsg">訊息</param>
/// <param name="strSessionID">SessionID</param>
/// <returns>發送結果</returns>
bool CSKClient::SendLoginReply(unsigned char uncaSecondByte, const string& strCode, const string& strMsg, const string& strSessionID)
{
    try
    {
        memset(m_uncaReplyBuf, 0, sizeof(m_uncaReplyBuf));
        m_uncaReplyBuf[0] = ESCAPE_BYTE;
        m_uncaReplyBuf[1] = uncaSecondByte;
        memcpy(m_uncaReplyBuf + 2, strCode.c_str(), strCode.size() > 4 ? 4 : strCode.size());
        memcpy(m_uncaReplyBuf + 6, strMsg.c_str(), strMsg.size() > 512 ? 512 : strMsg.size());
        memcpy(m_uncaReplyBuf + 518, strSessionID.c_str(), strSessionID.size() > 100 ? 100 : strSessionID.size());

        struct TSKReplyData tReplyData;

        memset(tReplyData.uncaReplyData, 0, sizeof(tReplyData.uncaReplyData));
        memcpy(tReplyData.uncaReplyData, m_uncaReplyBuf, PACKET_HEADER_LENGTH + LOGIN_REP_LENGTH);

        EnQueueReply("M_LOGIN_REPLY", tReplyData, PACKET_HEADER_LENGTH + LOGIN_REP_LENGTH);

        /*bool bSendAll = SendAll("M_LOGIN_REPLY", m_uncaReplyBuf, PACKET_HEADER_LENGTH + LOGIN_REP_LENGTH);
        if (!bSendAll)
        {
            EnQueueLogger(e_ltClientError, "M_LOGIN_REPLY_ERROR");
        }
        return bSendAll;*/
    }
    catch (...)
    {
    }
    return false;
}


/// <summary>
/// SKClient內部回傳SessionID驗證結果
/// </summary>
/// <param name="uncaSecondByte">TransNo</param>
/// <param name="strCode">代碼</param>
/// <param name="strMsg">訊息</param>
/// <returns>發送結果</returns>
bool CSKClient::SendSessionReply(unsigned char uncaSecondByte, const string& strCode, const string& strMsg)
{
    try
    {
        memset(m_uncaReplyBuf, 0, sizeof(m_uncaReplyBuf));
        m_uncaReplyBuf[0] = ESCAPE_BYTE;
        m_uncaReplyBuf[1] = uncaSecondByte;
        memcpy(m_uncaReplyBuf + 2, strCode.c_str(), strCode.size() > 4 ? 4 : strCode.size());
        memcpy(m_uncaReplyBuf + 6, strMsg.c_str(), strMsg.size() > 512 ? 512 : strMsg.size());

        struct TSKReplyData tReplyData;

        memset(tReplyData.uncaReplyData, 0, sizeof(tReplyData.uncaReplyData));
        memcpy(tReplyData.uncaReplyData, m_uncaReplyBuf, PACKET_HEADER_LENGTH + SESSION_REP_LENGTH);

        EnQueueReply("M_SESSION_REPLY", tReplyData, PACKET_HEADER_LENGTH + SESSION_REP_LENGTH);

        /*bool bSendAll = SendAll("M_SESSION_REPLY", m_uncaReplyBuf, PACKET_HEADER_LENGTH + SESSION_REP_LENGTH);
        if (!bSendAll)
        {
            EnQueueLogger(e_ltClientError, "M_SESSION_REPLY_ERROR");
        }
        return bSendAll;*/
    }
    catch (...)
    {
    }
    return false;
}

/// <summary>
/// SKClient內部回傳送單結果
/// </summary>
/// <param name="uncaSecondByte">TransNo</param>
/// <param name="strCode">代碼</param>
/// <param name="strMsg">訊息</param>
/// <param name="strOrderNo">書號</param>
/// <param name="strSeqNo">序號</param>
/// <param name="strORKEY">自訂碼</param>
/// <returns>發送結果</returns>
bool CSKClient::SendOrderReply(unsigned char uncaSecondByte, const string& strCode, const string& strMsg,
    const string& strOrderNo, const string& strSeqNo, const string& strORKEY)
{
    try
    {
        memset(m_uncaReplyBuf, 0, sizeof(m_uncaReplyBuf));
        m_uncaReplyBuf[0] = ESCAPE_BYTE;
        m_uncaReplyBuf[1] = uncaSecondByte;
        memcpy(m_uncaReplyBuf + 2, strCode.c_str(), strCode.size() > 4 ? 4 : strCode.size());
        memcpy(m_uncaReplyBuf + 6, strMsg.c_str(), strMsg.size() > 512 ? 512 : strMsg.size());
        memcpy(m_uncaReplyBuf + 518, strOrderNo.c_str(), strOrderNo.size() > 20 ? 20 : strOrderNo.size());
        memcpy(m_uncaReplyBuf + 538, strSeqNo.c_str(), strSeqNo.size() > 20 ? 20 : strSeqNo.size());
        memcpy(m_uncaReplyBuf + 558, strORKEY.c_str(), strORKEY.size() > 100 ? 100 : strORKEY.size());

        struct TSKReplyData tReplyData;

        memset(tReplyData.uncaReplyData, 0, sizeof(tReplyData.uncaReplyData));
        memcpy(tReplyData.uncaReplyData, m_uncaReplyBuf, PACKET_HEADER_LENGTH + ORDER_REP_LENGTH);

        EnQueueReply("M_ORDER_REPLY", tReplyData, PACKET_HEADER_LENGTH + ORDER_REP_LENGTH);

        /*bool bSendAll = SendAll("M_ORDER_REPLY", m_uncaReplyBuf, PACKET_HEADER_LENGTH + ORDER_REP_LENGTH);
        if (!bSendAll)
        {
            EnQueueLogger(e_ltClientError, "M_ORDER_REPLY_ERROR");
        }
        return bSendAll;*/
    }
    catch (...)
    {
    }
    return false;
}

/// <summary>
/// 確認連線後n秒內是否有連線成功
/// </summary>
void CSKClient::LoginCheck()
{
    try
    {
        // 1. 只會有一條thread, mutex不用共用啦~
        // 2. 不用管是被喚醒還是時間到被叫起來, 這樣寫是確保thread能在被解構前執行
        //    雖然解構後執行, 也不會閃退或有問題啥的, 但...總是會怕啦XD
        // 3. 連線後30秒內就斷線了, 那就喚醒thread釋放

        mutex mtx;
        std::unique_lock<std::mutex> lock(mtx);
        m_cvCheckLogin.wait_for(lock, chrono::seconds(30));

        // 開始or登入中: 設成斷線
        // 其它: 不設定
        // 避免在登入中就先斷線了, 這樣狀態會有問題
        if (m_csClientStatus == csStart || m_csClientStatus == csLogining)
        {
            SetStatus(csOffline);
            ShutDownFD();
            EnQueueLogger(e_ltClientError, "NOT_LOGIN_IN_30_SECONDS");
        }
    }
    catch (exception& e)
    {
        EnQueueLogger(e_ltClientError, "SKClient_LoginCheck Error, exception: " + string(e.what()));
    }
    catch (...)
    {
        EnQueueLogger(e_ltClientError, "SKClient_LoginCheck Error.");
    }
}

/// <summary>
/// 送進Log Queue 
/// </summary>
/// <param name="tLogType">Log類型</param>
/// <param name="strMsg">文字</param>
void CSKClient::EnQueueLogger(TLogType tLogType, const string& strMsg)
{
    // Add IP & ID
    string strLogMsg = "[" + m_strIP + "][" + m_strLoginID + "][" + to_string(m_nSocketID) + "]" + strMsg;

    if (g_pServers != NULL)
        g_pServers->EnQueueLogger(tLogType, strLogMsg, false);
}

/// <summary>
/// 送進委託Queue
/// </summary>
/// <param name="uncaSecondByte">TransNo</param>
/// <param name="pFuncData">委託資料</param>
/// <param name="tvStart">紀錄收到時間</param>
void CSKClient::EnQueueFunc(unsigned char uncaSecondByte, const unsigned char* pFuncData, const struct timeval& tvStart)
{
    string strQueueID = "";
    switch (uncaSecondByte)
    {
    case LOGIN_BYTE:   strQueueID = "1"; break;
    case SESSION_BYTE: strQueueID = "2"; break;
    case TSORDER_BYTE: strQueueID = string((g_tSKClientsShm + m_nShmIndex)->caTSOrderQID); break;
    case TSALTER_BYTE: strQueueID = string((g_tSKClientsShm + m_nShmIndex)->caTSAlterQID); break;
    case TFORDER_BYTE: strQueueID = string((g_tSKClientsShm + m_nShmIndex)->caTFOrderQID); break;
    case TFALTER_BYTE: strQueueID = string((g_tSKClientsShm + m_nShmIndex)->caTFAlterQID); break;
    case OFORDER_BYTE: strQueueID = string((g_tSKClientsShm + m_nShmIndex)->caOFOrderQID); break;
    case OSORDER_BYTE: strQueueID = string((g_tSKClientsShm + m_nShmIndex)->caOSOrderQID); break;
    case OSBORDER_BYTE: strQueueID = string((g_tSKClientsShm + m_nShmIndex)->caOSBOrderQID); break;
    case PRE_TSALTER_BYTE: strQueueID = string((g_tSKClientsShm + m_nShmIndex)->caPreTSAlterQID); break;
    case PRE_TFALTER_BYTE: strQueueID = string((g_tSKClientsShm + m_nShmIndex)->caPreTFAlterQID); break;
    case PRE_OFORDER_BYTE: strQueueID = string((g_tSKClientsShm + m_nShmIndex)->caPreOFOrderQID); break;
    case PRE_OSORDER_BYTE: strQueueID = string((g_tSKClientsShm + m_nShmIndex)->caPreOSOrderQID); break;
    default:
        return;
    }
    shared_ptr<CSKClient> shpClient{ shared_from_this() };

    if (strQueueID == "1")
        g_pServers->EnQueueLogin(shpClient, pFuncData, tvStart);
    else if (strQueueID == "2")
        g_pServers->EnQueueSession(shpClient, pFuncData, tvStart);
    else if (strQueueID == "3")
        g_pServers->EnQueueTSOrder(shpClient, pFuncData, tvStart);
    else if (strQueueID == "4")
        g_pServers->EnQueueTSAlter(shpClient, pFuncData, tvStart);
    else if (strQueueID == "5")
        g_pServers->EnQueueTFOrder(shpClient, pFuncData, tvStart);
    else if (strQueueID == "6")
        g_pServers->EnQueueTFAlter(shpClient, pFuncData, tvStart);
    else if (strQueueID == "7")
        g_pServers->EnQueueOFOrder(shpClient, pFuncData, tvStart);
    //else if (strQueueID == "8")
    // ; 
    //else if (strQueueID == "9")
    // ;
    else if (strQueueID == "10")
        g_pServers->EnQueueOSOrder(shpClient, pFuncData, tvStart);
    else if (strQueueID == "11")
        g_pServers->EnQueueOSBOrder(shpClient, pFuncData, tvStart);
    else if (strQueueID == "12")
        g_pServers->EnQueuePreTSAlter(shpClient, pFuncData, tvStart);
    else if (strQueueID == "13")
        g_pServers->EnQueuePreTFAlter(shpClient, pFuncData, tvStart);
    else if (strQueueID == "14")
        g_pServers->EnQueuePreOFOrder(shpClient, pFuncData, tvStart);
    else if (strQueueID == "15")
        g_pServers->EnQueuePreOSOrder(shpClient, pFuncData, tvStart);
    else
        return;

    //auto iter = m_mapTransToFunc.find(uncaSecondByte);
    //if (iter == m_mapTransToFunc.end())
    //    return;
    //else
    //{
    //    shared_ptr<CSKClient> shpClient{ shared_from_this() };
    //    switch (iter->second)
    //    {
    //    //case e_ftLogger: break; // Log 格式不同
    //    case e_ftLogin:   g_pServers->EnQueueLogin(shpClient, pFuncData, m_strIP, tvStart); break;
    //    case e_ftSession: g_pServers->EnQueueSession(shpClient, pFuncData, m_strIP, tvStart); break;
    //    case e_ftTSOrder: g_pServers->EnQueueTSOrder(shpClient, pFuncData, m_strIP, tvStart); break;
    //    case e_ftTSAlter: g_pServers->EnQueueTSAlter(shpClient, pFuncData, m_strIP, tvStart); break;
    //    case e_ftTFOrder: g_pServers->EnQueueTFOrder(shpClient, pFuncData, m_strIP, tvStart); break;
    //    case e_ftTFAlter: g_pServers->EnQueueTFAlter(shpClient, pFuncData, m_strIP, tvStart); break;
    //    case e_ftOFOrder: g_pServers->EnQueueOFOrder(shpClient, pFuncData, m_strIP, tvStart); break;
    //    default:
    //        break;
    //    }
    //}

    //if (g_pServers != NULL)
    //    g_pServers->EnQueueFunc(uncaSecondByte, shpClient, pFuncData, m_strIP, tvStart);

    //switch (uncaSecondByte)
    //{
    //case LOGIN_BYTE:
    //    if (g_pServers != NULL)
    //        g_pServers->EnQueueFunc(ftLogin, shpClient, pFuncData, m_strIP, tvStart);
    //        //g_pServers->EnQueueLogin(shpClient, pFuncData, m_strIP, tvStart);
    //    break;
    //case SESSION_BYTE:
    //    if (g_pServers != NULL)
    //        g_pServers->EnQueueFunc(ftSession, shpClient, pFuncData, m_strIP, tvStart);
    //        //g_pServers->EnQueueSession(shpClient, pFuncData, m_strIP, tvStart);
    //    break;
    //case TSORDER_BYTE:
    //    if (g_pServers != NULL)
    //        g_pServers->EnQueueFunc(ftTSOrder, shpClient, pFuncData, m_strIP, tvStart);
    //        //g_pServers->EnQueueTSOrder(shpClient, pFuncData, m_strIP, tvStart);
    //    break;
    //case TSALTER_BYTE:
    //    if (g_pServers != NULL)
    //        g_pServers->EnQueueFunc(ftTSAlter, shpClient, pFuncData, m_strIP, tvStart);
    //        //g_pServers->EnQueueTSAlter(shpClient, pFuncData, m_strIP, tvStart);
    //    break;
    //case TFORDER_BYTE:
    //    if (g_pServers != NULL)
    //        g_pServers->EnQueueFunc(ftTFOrder, shpClient, pFuncData, m_strIP, tvStart);
    //        //g_pServers->EnQueueTFOrder(shpClient, pFuncData, m_strIP, tvStart);
    //    break;
    //case TFALTER_BYTE:
    //    if (g_pServers != NULL)
    //        g_pServers->EnQueueFunc(ftTFAlter, shpClient, pFuncData, m_strIP, tvStart);
    //        //g_pServers->EnQueueTFAlter(shpClient, pFuncData, m_strIP, tvStart);
    //    break;
    //case OFORDER_BYTE:
    //    if (g_pServers != NULL)
    //        g_pServers->EnQueueFunc(ftOFOrder, shpClient, pFuncData, m_strIP, tvStart);
    //        //g_pServers->EnQueueOFOrder(shpClient, pFuncData, m_strIP, tvStart);
    //    break;
    //default:
    //    break;
    //}
}

/// <summary>
/// 寫回傳資料進Queue
/// </summary>
/// <param name="strFuncName">功能名稱</param>
/// <param name="unpcReplyData">發送資料</param>
/// <param name="nSize">資料長度</param>
void CSKClient::EnQueueReply(const string& strFuncName, struct TSKReplyData& replyData, int nSize)
{
    try
    {
        TSKClientReplyQ* pTSKClientReplyQ = new TSKClientReplyQ();
        pTSKClientReplyQ->nLength = nSize;
        pTSKClientReplyQ->strFuncName.assign(strFuncName);
        pTSKClientReplyQ->tSKLoggerGW = replyData.tSKLoggerGW;
        pTSKClientReplyQ->tvStart = replyData.tvStart;
        pTSKClientReplyQ->tvSendT = replyData.tvSendT;
        memset(pTSKClientReplyQ->uncaReplyBuf, 0, sizeof(pTSKClientReplyQ->uncaReplyBuf));
        memcpy(pTSKClientReplyQ->uncaReplyBuf, replyData.uncaReplyData, nSize);

        if (true)
        {
            std::unique_lock<std::mutex> lock(m_mtxReply);
            m_qReply.push(pTSKClientReplyQ);
        }
        m_cvReply.notify_one();
    }
    catch (exception& e)
    {
        EnQueueLogger(e_ltClientError, "EnQueueReply Error, exception: " + string(e.what()));
    }
    catch (...)
    {
        EnQueueLogger(e_ltClientError, "EnQueueReply Error.");
    }
}

/// <summary>
/// Thread 處理回報 
/// </summary>
void CSKClient::DispatchReplyData()
{
    try
    {
        TSKClientReplyQ* pTSKClientReplyQ = NULL;
        while (true)
        {
            // Block 打開時判斷狀態
            if (m_csClientStatus == csOffline || m_csClientStatus == csTerminated)
                break;

            if (true)
            {
                std::unique_lock<std::mutex> lock(m_mtxReply);
                if (m_qReply.empty())
                {
                    m_cvReply.wait(lock);
                    continue;
                }
                pTSKClientReplyQ = m_qReply.front();
                m_qReply.pop();
            }

            if (pTSKClientReplyQ != NULL)
            {
                // EnQueueLogger(e_ltClientError, "temp tSKLoggerGW.strUseTime: " + pTSKClientReplyQ->tSKLoggerGW.strUseTime);
                bool bSendAll = SendAll(pTSKClientReplyQ->strFuncName.c_str(), pTSKClientReplyQ->uncaReplyBuf, pTSKClientReplyQ->nLength);
                if (!bSendAll)
                {
                    EnQueueLogger(e_ltClientError, pTSKClientReplyQ->strFuncName + "_ERROR");
                }
                if (pTSKClientReplyQ->tSKLoggerGW.strFunction == "AddFuncLog" || pTSKClientReplyQ->tSKLoggerGW.strFunction == "AddOrderLog")
                {
                    struct timeval tvStart2;
                    gettimeofday(&tvStart2, NULL);
                    time_t nowtime = tvStart2.tv_sec;
                    struct tm* nowtm = localtime(&nowtime);
                    char tmbuf[64], buf[64];
                    strftime(tmbuf, sizeof tmbuf, "%Y %m %d %H:%M:%S", nowtm);
                    snprintf(buf, sizeof buf, "%s.%06ld", tmbuf, tvStart2.tv_usec);
                    string strEndTime = buf;

                    long totalT = 1000000 * (tvStart2.tv_sec - pTSKClientReplyQ->tvStart.tv_sec) + (tvStart2.tv_usec - pTSKClientReplyQ->tvStart.tv_usec);
                    long enqueuSend = 1000000 * (tvStart2.tv_sec - pTSKClientReplyQ->tvSendT.tv_sec) + (tvStart2.tv_usec - pTSKClientReplyQ->tvSendT.tv_usec);

                    size_t nIndex = pTSKClientReplyQ->tSKLoggerGW.strUseTime.find("#");
                    if (nIndex != string::npos)
                    {
                        string tempStr = pTSKClientReplyQ->tSKLoggerGW.strUseTime.substr(0, nIndex);
                        tempStr.append(",").append(to_string(enqueuSend)).append(",").append(pTSKClientReplyQ->tSKLoggerGW.strUseTime.substr(nIndex + 1));
                        pTSKClientReplyQ->tSKLoggerGW.strUseTime = tempStr;
                    }
                    pTSKClientReplyQ->tSKLoggerGW.strUseTime = pTSKClientReplyQ->tSKLoggerGW.strUseTime.append(",total=").append(to_string(totalT));

                    pTSKClientReplyQ->tSKLoggerGW.strEndTime = strEndTime;

                    pTSKClientReplyQ->tSKLoggerGW.strTotalTime = to_string(totalT);

                    g_pServers->EnQueueLoggerToGW(pTSKClientReplyQ->tSKLoggerGW);

                }
                delete pTSKClientReplyQ;
                pTSKClientReplyQ = NULL;
            }
        }
    }
    catch (...)
    {
        EnQueueLogger(e_ltClientError, "DispatchReplyData Error.");
    }
}

/// <summary>
/// 若偵測到SessionID修改, 重新驗證SessionID
/// </summary>
/// <param name="strSessionID">SessionID</param>
/// <returns>驗證結果</returns>
bool CSKClient::CheckSessionID(const string& strSessionID)
{
    if (m_strSessionID == strSessionID)
        return true;
    else
    {
        // 同步驗證 SessionID 是否有效
        bool bSuccess = g_pServers->CheckSessionID(m_strLoginID, strSessionID, m_strSource, m_strIP);
        if (bSuccess)
        {
            m_strSessionID = strSessionID; // 有效, 覆蓋舊 SessionID
            EnQueueLogger(e_ltClientData, "New SessionID Check Success! SessionID=" + strSessionID);
        }
        else
            EnQueueLogger(e_ltClientData, "New SessionID Check Fail! SessionID=" + strSessionID);

        return bSuccess;
    }
}

/// <summary>
/// 啟動Heartbeat
/// </summary>
void CSKClient::StartHeartbeat()
{
    lock_guard<std::mutex> lock(m_mtxHeartbeatLock);
    if (m_bHeartbeatStart == false)
    {
        m_pHeartbeat->Start();
        m_bHeartbeatStart = true;
    }
}

/// <summary>
/// 設定Client當前狀態
/// </summary>
/// <param name="csStatus">設定狀態</param>
void CSKClient::SetStatus(TSKClientStauts csStatus)
{
    lock_guard<std::mutex> lock(m_mtxClientStatusLock);
    m_csClientStatus = csStatus;
}

/// <summary>
/// 取得Client當前狀態
/// </summary>
/// <returns>當前狀態</returns>
TSKClientStauts CSKClient::GetStatus()
{
    return m_csClientStatus;
}

/// <summary>
/// 設定Client資訊
/// </summary>
/// <param name="strSource">來源別</param>
/// <param name="strSessionID">SessionID</param>
/// <param name="strLoginID">LoginID</param>
void CSKClient::SetUserData(const string& strSource, const string& strSessionID, const string& strLoginID)
{
    lock_guard<std::mutex> lock(m_mtxUserDataLock);
    m_strSessionID = strSessionID;
    m_strLoginID = strLoginID;
    m_strSource = strSource;
}

void CSKClient::SetLoginResult(bool bResult)
{
    if (bResult)
    {
        // 紀錄曾經登入成功過, 減連線數用的
        m_bLogin = true;

        // 登入中才改狀態, 避免在登入中就斷線了, 這樣狀態會有問題
        if (m_csClientStatus == csLogining)
        {
            SetStatus(csOnline);
            StartHeartbeat();
        }
    }
    else
    {
        // 登入中才改狀態, 避免在登入中就斷線了, 這樣狀態會有問題
        if (m_csClientStatus == csLogining)
            SetStatus(csStart);
    }
}

string CSKClient::GetLoginID()
{
    return m_strLoginID;
}

string CSKClient::GetIP()
{
    return m_strIP;
}

bool CSKClient::GetLogin()
{
    return m_bLogin;
}

/// <summary>
/// 確認該客戶ShmIndex是否正確
/// </summary>
/// <returns>-1:異常/-2:欄位不夠/>=0:正確Index</returns>
bool CSKClient::CheckShmIndex()
{
    // 取得該客戶在Shm的Index
    m_nShmIndex = get_shmidx_handler_(m_strLoginID);
    
    if (m_nShmIndex >= 0)
    {
        EnQueueLogger(e_ltClientData, "CheckShmIndex Get " + to_string(m_nShmIndex));
        return true;
    }
    else
    {
        if (m_nShmIndex == -1)
            EnQueueLogger(e_ltClientData, "CheckShmIndex Get -1, GetIdxFromShmMap exception.");
        else if (m_nShmIndex == -2)
            EnQueueLogger(e_ltClientData, "CheckShmIndex Get -2, GetIdxFromShmMap cannot find or get new index.");
        
        return false;
    }
}

int CSKClient::GetShmIndex()
{
    return m_nShmIndex;
}

int CSKClient::GetClientSocketFd()
{
    return m_nSocketID;
}

bool CSKClient::GetDecreaseConn()
{
    return m_bDecreaseConn;
}

void CSKClient::SetDecreaseConn(bool bDecreaseConn)
{
    m_bDecreaseConn = bDecreaseConn;
}

void CSKClient::ShutDownFD()
{
    lock_guard<std::mutex> lock(m_mtxShutDownLock);
    if (m_ClientAddrInfo.nSocket != -1)
    {
        shutdown(m_ClientAddrInfo.nSocket, SHUT_RDWR);
        close(m_ClientAddrInfo.nSocket);
        m_ClientAddrInfo.nSocket = -1;
    }
}

bool CSKClient::IsReplyQueueEmpty()
{
    if (m_qReply.size() > 0)
        return false;
    else
        return true;
}
