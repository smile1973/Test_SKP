#include "SKServers.h"
#include "SKClients.h"
#include <sstream>
#include <cstring>
#include <algorithm>
#include <exception>
#include <iostream>
#include <memory>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <iomanip>

using namespace std;

extern void FprintfStderrLog(const string& strMsg);
extern int Atoi(char* ptr, int n);

extern CSKServers* g_pServers;

CSKClients* CSKClients::instance = NULL;
mutex CSKClients::m_mtxInstance;

CSKClients::CSKClients() 
{
    m_vClient = {};
    m_tvSendTimeout = { 2,0 };
}

void CSKClients::CheckBeforeDisConn(vector<shared_ptr<CSKClient> >::iterator iter)
{
    CSKClient* pClient = (*iter).get();
    while (1)
    {
        if (pClient->IsReplyQueueEmpty() == true)
        {
            pClient->SetStatus(csOffline);
            pClient->ShutDownFD();
            sleep(3);
            if (pClient->GetStatus() == csTerminated)
                break;
            else
                continue;
        }  
        else
            sleep(1);
    }
}

CSKClients::~CSKClients() 
{
    if (m_pServerSocket->GetStatus() == sssListening)
    {
        m_pServerSocket->ShutdownServer();
    }
    
    sleep(2);//20231129

    if (m_pServerSocket)
    {
        delete m_pServerSocket;
        m_pServerSocket = NULL;
    }
    int nSize=m_vClient.size();
    if (nSize > 0)
    {
        vector<shared_ptr<CSKClient> >::iterator iter = m_vClient.begin(); //20231106 update
        std::thread myThreads[nSize];
        int i = 0;
        while (iter != m_vClient.end())
        {
            CSKClient* pClient = (*iter).get();

            if (pClient->GetLogin() == true && pClient->GetDecreaseConn() == false && i< nSize)
            {
                myThreads[i] = std::thread(&CSKClients::CheckBeforeDisConn, this, iter);
            }
            iter++;
            i++;
        }

        for (int j = 0; j < nSize; j++)
        {
            myThreads[j].join();
        }

        m_vClient.clear();//to check
    }
}

void CSKClients::ShutdownServer()
{
    if (m_pServerSocket->GetStatus() == sssListening)
    {
        m_pServerSocket->ShutdownServer();
    }
}

/// <summary>
/// 監聽連線Port, 並新增SKClient
/// </summary>
void CSKClients::Run()
{
    try
    {
        while (m_pServerSocket->GetStatus() == sssListening)
        {
            
            struct TSKClientAddrInfo ClientAddrInfo;
            memset(&ClientAddrInfo, 0, sizeof(struct TSKClientAddrInfo));
            ClientAddrInfo.nSocket = m_pServerSocket->Accept(&ClientAddrInfo.ClientAddr, &ClientAddrInfo.ssl);
            // Accept錯誤, 沒有IP可以記錄
            if (ClientAddrInfo.nSocket == -2)
            {
                g_pServers->EnQueueLogger(e_ltClientError, "ACCEPT_CLIENT_ERROR: Socket accept error.", false);
                continue;
            }

            // 紀錄IP
            unsigned char* ip = (unsigned char*)(&ClientAddrInfo.ClientAddr.sin_addr.s_addr);
            if (ip != NULL)
                snprintf(ClientAddrInfo.caIP,sizeof(ClientAddrInfo.caIP), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
            else
                g_pServers->EnQueueLogger(e_ltClientError, "ACCEPT_CLIENT_ERROR: IP NULL.", false);

            // SSL錯誤
            if (ClientAddrInfo.nSocket == -1 || ClientAddrInfo.ssl == NULL)
            {
                g_pServers->EnQueueLogger(e_ltClientError, "ACCEPT_CLIENT_ERROR: SSL error. IP: " + string(ClientAddrInfo.caIP), false);
                continue;
            }
            
            g_pServers->EnQueueLogger(e_ltClientInfo, "ACCEPT_CLIENT_IP: " + string(ClientAddrInfo.caIP), false);
            setsockopt(ClientAddrInfo.nSocket, SOL_SOCKET, SO_SNDTIMEO, (char*) &m_tvSendTimeout, sizeof(struct timeval));
            
            shared_ptr<CSKClient> shpClient = make_shared<CSKClient>(ClientAddrInfo);
            shpClient->get_shmidx_handler_ = std::bind(&CSKClients::GetIdxFromShmMap, this, std::placeholders::_1);
            shpClient->Start();
            PushBackClientToVector(shpClient);

            // Reset pointer
            ClientAddrInfo.ssl = NULL;
        }
    }
    catch (exception& e)
    {
        string strMsg = "SKClients_Run catch error, exception: " + string(e.what());
        g_pServers->EnQueueLogger(e_ltServerError, strMsg, false);
    }
    catch (...)
    {
        string strMsg = "SKClients_Run catch fatal error.";
        g_pServers->EnQueueLogger(e_ltServerError, strMsg, false);
    }
}

/// <summary>
/// SKServerSocket Callback
/// </summary>
void CSKClients::OnListening()
{
    Start();
}

/// <summary>
/// SKServerSocket Callback
/// </summary>
void CSKClients::OnShutdown()
{
}

CSKClients* CSKClients::GetInstance()
{
    if(instance == NULL)
    {
        lock_guard<std::mutex> lock(m_mtxInstance);

        if(instance == NULL)
        {
            instance = new CSKClients();
            FprintfStderrLog("CLIENTS_ONE");
        }
    }

    return instance;
}


/// <summary>
/// 設定基本資訊
/// </summary>
/// <param name="strListenPort">監聽Port</param>
void CSKClients::SetConfig(string& strListenPort, int nClientShmSize)
{
    try
    {
        m_strListenPort = strListenPort;
        m_nClientShmSize = nClientShmSize;

        m_pServerSocket = new CSKServerSocket(this);
        m_pServerSocket->Listen(m_strListenPort);
    }
    catch (exception& e)
    {
        FprintfStderrLog("NEW_SERVERSOCKET_ERROR" + string(e.what()));
    }
}

/// <summary>
/// 更新Shm
/// </summary>
void CSKClients::ShmTimer()
{
    try
    {
        // 修改 g_tSKShm 當前 SKClient 連線數量
        size_t nClientsNum = m_vClient.size();
        char caClientsNum[sizeof(g_tSKServersShm->caConnCounts)];
        string strClientsNum = "";

        // 清空參數
        memset(caClientsNum, 0, sizeof(caClientsNum));

        // 數量>99999, Shm大小不足, 只顯示99999
        if (nClientsNum > 99999)
            memcpy(caClientsNum, "99999", sizeof(caClientsNum));
        else
        {
            strClientsNum = to_string(nClientsNum);
            memcpy(caClientsNum, strClientsNum.c_str(), strClientsNum.size());
        }

        memcpy(g_tSKServersShm->caConnCounts, caClientsNum, sizeof(g_tSKServersShm->caConnCounts));
    }
    catch (exception& e)
    {
        string strMsg = "SKClients_ShmTimer catch error, exception: " + string(e.what());
        g_pServers->EnQueueLogger(e_ltServerError, strMsg, false);
    }
    catch (...)
    {
        string strMsg = "SKClients_ShmTimer catch fatal error.";
        g_pServers->EnQueueLogger(e_ltServerError, strMsg, false);
    }
}

/// <summary>
/// 回傳該客戶Shm資料的Index
/// </summary>
/// <param name="strLoginID">客戶ID</param>
/// <returns>-1:異常/-2:欄位不夠/>=0:正確Index</returns>
int CSKClients::GetIdxFromShmMap(const string& strLoginID)
{
    try
    {
        int nLoginIDIdx = -1;   // Shm Index

        // Q:為何查詢也要上鎖? 不是只有新增時上鎖?
        // A:如果查詢時不上鎖, 假設某一客戶同時建立兩條連線&登入成功, 同時查詢後發現沒有資料
        //   那就會是兩條連線同時要新增資料進Shm, Shm裡會有兩筆相同資料, 這樣就會有問題
        {
            // 表鎖: 分配總體資源用
            lock_guard<std::mutex> lock(m_mtxClientShmMapLock);
            auto iter = m_mapClientShm.find(strLoginID);

            // 有找到
            if (iter != m_mapClientShm.end())
            {
                nLoginIDIdx = iter->second;
            }
            // 沒找到, 新增進map
            else
            {
                if (m_qIdleIndex.empty()) // 沒有找到空閒的index, 空間不足, 難過QQ
                    return -2;
                else
                {
                    nLoginIDIdx = m_qIdleIndex.front();
                    m_qIdleIndex.pop();

                    m_mapClientShm.insert({ strLoginID, nLoginIDIdx });

                    memcpy((g_tSKClientsShm + nLoginIDIdx)->caLoginID, strLoginID.c_str(), strLoginID.length());
                    memcpy((g_tSKClientsShm + nLoginIDIdx)->caLoggerQID, g_tSKServersShm->caLoggerQID, sizeof((g_tSKClientsShm + nLoginIDIdx)->caLoggerQID));
                    memcpy((g_tSKClientsShm + nLoginIDIdx)->caLoginQID, g_tSKServersShm->caLoginQID, sizeof((g_tSKClientsShm + nLoginIDIdx)->caLoginQID));
                    memcpy((g_tSKClientsShm + nLoginIDIdx)->caSessionQID, g_tSKServersShm->caSessionQID, sizeof((g_tSKClientsShm + nLoginIDIdx)->caSessionQID));
                    memcpy((g_tSKClientsShm + nLoginIDIdx)->caTSOrderQID, g_tSKServersShm->caTSOrderQID, sizeof((g_tSKClientsShm + nLoginIDIdx)->caTSOrderQID));
                    memcpy((g_tSKClientsShm + nLoginIDIdx)->caTSAlterQID, g_tSKServersShm->caTSAlterQID, sizeof((g_tSKClientsShm + nLoginIDIdx)->caTSAlterQID));
                    memcpy((g_tSKClientsShm + nLoginIDIdx)->caTFOrderQID, g_tSKServersShm->caTFOrderQID, sizeof((g_tSKClientsShm + nLoginIDIdx)->caTFOrderQID));
                    memcpy((g_tSKClientsShm + nLoginIDIdx)->caTFAlterQID, g_tSKServersShm->caTFAlterQID, sizeof((g_tSKClientsShm + nLoginIDIdx)->caTFAlterQID));
                    memcpy((g_tSKClientsShm + nLoginIDIdx)->caOFOrderQID, g_tSKServersShm->caOFOrderQID, sizeof((g_tSKClientsShm + nLoginIDIdx)->caOFOrderQID));
                    memcpy((g_tSKClientsShm + nLoginIDIdx)->caFTSOrderQID, g_tSKServersShm->caFTSOrderQID, sizeof((g_tSKClientsShm + nLoginIDIdx)->caFTSOrderQID));
                    memcpy((g_tSKClientsShm + nLoginIDIdx)->caFTSAlterQID, g_tSKServersShm->caFTSAlterQID, sizeof((g_tSKClientsShm + nLoginIDIdx)->caFTSAlterQID));
                    memcpy((g_tSKClientsShm + nLoginIDIdx)->caOSOrderQID, g_tSKServersShm->caOSOrderQID, sizeof((g_tSKClientsShm + nLoginIDIdx)->caOSOrderQID));
                    memcpy((g_tSKClientsShm + nLoginIDIdx)->caOSBOrderQID, g_tSKServersShm->caOSBOrderQID, sizeof((g_tSKClientsShm + nLoginIDIdx)->caOSBOrderQID));
                    memcpy((g_tSKClientsShm + nLoginIDIdx)->caPreTSAlterQID, g_tSKServersShm->caPreTSAlterQID, sizeof((g_tSKClientsShm + nLoginIDIdx)->caPreTSAlterQID));
                    memcpy((g_tSKClientsShm + nLoginIDIdx)->caPreTFAlterQID, g_tSKServersShm->caPreTFAlterQID, sizeof((g_tSKClientsShm + nLoginIDIdx)->caPreTFAlterQID));
                    memcpy((g_tSKClientsShm + nLoginIDIdx)->caPreOFOrderQID, g_tSKServersShm->caPreOFOrderQID, sizeof((g_tSKClientsShm + nLoginIDIdx)->caPreOFOrderQID));
                    memcpy((g_tSKClientsShm + nLoginIDIdx)->caPreOSOrderQID, g_tSKServersShm->caPreOSOrderQID, sizeof((g_tSKClientsShm + nLoginIDIdx)->caPreOSOrderQID));
                }
            }
        }

        return nLoginIDIdx;

        /*
        //紀錄當前時間, 這個不用進Lock
        struct timeval tv_NowTime;
        struct tm tm_NowTime;
        gettimeofday(&tv_NowTime, NULL);
        localtime_r(&tv_NowTime.tv_sec, &tm_NowTime);
        char cDate[20];
        strftime(cDate, 20, "%Y/%m/%d %H:%M:%S", &tm_NowTime); //日期時間(無毫秒)

        bool bFindLoginID = false;  // LoginID是否存在?
        int nLoginIDIdx = -1;   // Shm Index

        // Q:為何查詢也要上鎖? 不是只有新增時上鎖?
        // A:如果查詢時不上鎖, 假設某一客戶同時建立兩條連線&登入成功, 同時查詢後發現沒有資料
        //   那就會是兩條連線同時要新增資料進Shm, Shm裡會有兩筆相同資料, 這樣就會有問題
        {
            // 表鎖: 分配總體資源用
            lock_guard<std::mutex> lock(m_mtxClientShmMapLock);
            auto iter = m_mapClientShm.find(strLoginID);

            // 有找到
            if (iter != m_mapClientShm.end())
            {
                bFindLoginID = true;
                nLoginIDIdx = iter->second;
            }
            // 沒找到, 新增進map
            else
            {
                bFindLoginID = false;
                if(m_qIdleIndex.empty()) // 沒有找到空閒的index, 空間不足, 難過QQ
                    return -2;
                else
                {
                    nLoginIDIdx = m_qIdleIndex.front();
                    m_qIdleIndex.pop();
                    InsertToShmMap(strLoginID, nLoginIDIdx);
                }
            }
        }

        // 找到的Index超過最大值, 不合理
        if (nLoginIDIdx >= CLIENT_SHM_SIZE)
            return -1;

        {
            // Index鎖: 客戶各自上鎖
            lock_guard<std::mutex> lock(m_mtxaShmLock[nLoginIDIdx]);

            // 客戶ID已存在
            if (bFindLoginID)
            {
                // 判斷連線數有沒有超過?
                int nNowConn = Atoi((g_tSKClientShm + nLoginIDIdx)->caNowConnNum, sizeof((g_tSKClientShm + nLoginIDIdx)->caNowConnNum));
                int nMaxConn = Atoi((g_tSKClientShm + nLoginIDIdx)->caMaxConnNum, sizeof((g_tSKClientShm + nLoginIDIdx)->caMaxConnNum));
                // 任意一個數值<0, 回傳異常
                if (nNowConn < 0 || nMaxConn < 0)
                    return -3;
                // 連線數>=最大值, 回傳連線數超過 (=代表這條加上去會超過)
                if (nNowConn >= nMaxConn)
                    return -4;
                // 連線數<最大值, 當前連線數加1寫回去Shm&紀錄時間, 回傳Index
                else
                {
                    nNowConn += 1;
                    memcpy((g_tSKClientShm + nLoginIDIdx)->caNowConnNum, to_string(nNowConn).c_str(), to_string(nNowConn).length());
                    memcpy((g_tSKClientShm + nLoginIDIdx)->caLastConnTime, cDate, sizeof((g_tSKClientShm + nLoginIDIdx)->caLastConnTime));
                    return nLoginIDIdx;
                }
            }
            else // 客戶ID不存在, 開始新增
            {
                memcpy((g_tSKClientShm + nLoginIDIdx)->caLoginID, strLoginID.c_str(), strLoginID.length());
                memcpy((g_tSKClientShm + nLoginIDIdx)->caMaxConnNum, g_tSKServersShm->caClientMaxConn, sizeof((g_tSKClientShm + nLoginIDIdx)->caMaxConnNum));
                memcpy((g_tSKClientShm + nLoginIDIdx)->caNowConnNum, "1", sizeof("1")); //第一條連線
                memcpy((g_tSKClientShm + nLoginIDIdx)->caLastConnTime, cDate, sizeof((g_tSKClientShm + nLoginIDIdx)->caLastConnTime));
                memcpy((g_tSKClientShm + nLoginIDIdx)->caLoggerQID, g_tSKServersShm->caLoggerQID, sizeof((g_tSKClientShm + nLoginIDIdx)->caLoggerQID));
                memcpy((g_tSKClientShm + nLoginIDIdx)->caLoginQID, g_tSKServersShm->caLoginQID, sizeof((g_tSKClientShm + nLoginIDIdx)->caLoginQID));
                memcpy((g_tSKClientShm + nLoginIDIdx)->caSessionQID, g_tSKServersShm->caSessionQID, sizeof((g_tSKClientShm + nLoginIDIdx)->caSessionQID));
                memcpy((g_tSKClientShm + nLoginIDIdx)->caTSOrderQID, g_tSKServersShm->caTSOrderQID, sizeof((g_tSKClientShm + nLoginIDIdx)->caTSOrderQID));
                memcpy((g_tSKClientShm + nLoginIDIdx)->caTSAlterQID, g_tSKServersShm->caTSAlterQID, sizeof((g_tSKClientShm + nLoginIDIdx)->caTSAlterQID));
                memcpy((g_tSKClientShm + nLoginIDIdx)->caTFOrderQID, g_tSKServersShm->caTFOrderQID, sizeof((g_tSKClientShm + nLoginIDIdx)->caTFOrderQID));
                memcpy((g_tSKClientShm + nLoginIDIdx)->caTFAlterQID, g_tSKServersShm->caTFAlterQID, sizeof((g_tSKClientShm + nLoginIDIdx)->caTFAlterQID));
                memcpy((g_tSKClientShm + nLoginIDIdx)->caOFOrderQID, g_tSKServersShm->caOFOrderQID, sizeof((g_tSKClientShm + nLoginIDIdx)->caOFOrderQID));
                memcpy((g_tSKClientShm + nLoginIDIdx)->caFTSOrderQID, g_tSKServersShm->caFTSOrderQID, sizeof((g_tSKClientShm + nLoginIDIdx)->caFTSOrderQID));
                memcpy((g_tSKClientShm + nLoginIDIdx)->caFTSAlterQID, g_tSKServersShm->caFTSAlterQID, sizeof((g_tSKClientShm + nLoginIDIdx)->caFTSAlterQID));

                return nLoginIDIdx;
            }
        }
        */

        /*
        bool bFirstIdx = true;
        unsigned int nFirstIdx = 0;

        // Q:為何查詢也要上鎖? 不是只有新增時上鎖?
        // A:如果查詢時不上鎖, 假設某一客戶同時建立兩條連線&登入成功, 同時查詢後發現沒有資料
        //   那就會是兩條連線同時要新增資料進Shm, Shm裡會有兩筆相同資料, 這樣就會有問題
        lock_guard<std::mutex> lock(m_mtxClientShmMapLock);
        for (unsigned int nIdx = 0; nIdx < m_mapClientShm.size(); nIdx++)
        {
            // Shm有這個ID, 取得該ID現前Index
            if (m_mapClientShm[nIdx] == strLoginID)
            {
                // 判斷連線數有沒有超過?
                int nNowConn = Atoi((g_tSKClientShm + nIdx)->caNowConnNum, sizeof((g_tSKClientShm + nIdx)->caNowConnNum));
                int nMaxConn = Atoi((g_tSKClientShm + nIdx)->caMaxConnNum, sizeof((g_tSKClientShm + nIdx)->caMaxConnNum));
                
                // 任意一個數值<0, 回傳異常(-1)
                if (nNowConn < 0 || nMaxConn < 0)
                    return -3;
                // 連線數>=最大值, 回傳連線數超過 (=代表這條加上去會超過)
                if (nNowConn >= nMaxConn)
                    return -4;
                // 連線數<最大值, 當前連線數加1寫回去Shm&紀錄時間, 回傳Index
                else
                {
                    nNowConn += 1;
                    memcpy((g_tSKClientShm + nIdx)->caNowConnNum, to_string(nNowConn).c_str(), to_string(nNowConn).length());
                    memcpy((g_tSKClientShm + nIdx)->caLastConnTime, cDate, sizeof((g_tSKClientShm + nIdx)->caLastConnTime));
                    return nIdx;
                }
            }

            // 紀錄第一個可用Index, 如果沒找到值, 就用這個新增
            if (bFirstIdx)
            {
                if (m_mapClientShm[nIdx] == "")
                {
                    nFirstIdx = nIdx;
                    bFirstIdx = false;
                }
            }
        }
        // --------------------跳出迴圈代表沒找到--------------------

        // 沒有找到可用的index, 空間不足, 難過QQ
        if (bFirstIdx)
            return -2;
        // 有可用的index
        else
        {
            //紀錄Index對應LoginID
            m_mapClientShm[nFirstIdx] = strLoginID;

            // Shm初始化資料
            memcpy((g_tSKClientShm + nFirstIdx)->caLoginID, strLoginID.c_str(), strLoginID.length());
            memcpy((g_tSKClientShm + nFirstIdx)->caMaxConnNum, g_tSKServersShm->caClientMaxConn, sizeof((g_tSKClientShm + nFirstIdx)->caMaxConnNum));
            memcpy((g_tSKClientShm + nFirstIdx)->caNowConnNum, "1", sizeof("1")); //第一條連線
            memcpy((g_tSKClientShm + nFirstIdx)->caLastConnTime, cDate, sizeof((g_tSKClientShm + nFirstIdx)->caLastConnTime));
            memcpy((g_tSKClientShm + nFirstIdx)->caLoggerQID, g_tSKServersShm->caLoggerQID, sizeof((g_tSKClientShm + nFirstIdx)->caLoggerQID));
            memcpy((g_tSKClientShm + nFirstIdx)->caLoginQID, g_tSKServersShm->caLoginQID, sizeof((g_tSKClientShm + nFirstIdx)->caLoginQID));
            memcpy((g_tSKClientShm + nFirstIdx)->caSessionQID, g_tSKServersShm->caSessionQID, sizeof((g_tSKClientShm + nFirstIdx)->caSessionQID));
            memcpy((g_tSKClientShm + nFirstIdx)->caTSOrderQID, g_tSKServersShm->caTSOrderQID, sizeof((g_tSKClientShm + nFirstIdx)->caTSOrderQID));
            memcpy((g_tSKClientShm + nFirstIdx)->caTSAlterQID, g_tSKServersShm->caTSAlterQID, sizeof((g_tSKClientShm + nFirstIdx)->caTSAlterQID));
            memcpy((g_tSKClientShm + nFirstIdx)->caTFOrderQID, g_tSKServersShm->caTFOrderQID, sizeof((g_tSKClientShm + nFirstIdx)->caTFOrderQID));
            memcpy((g_tSKClientShm + nFirstIdx)->caTFAlterQID, g_tSKServersShm->caTFAlterQID, sizeof((g_tSKClientShm + nFirstIdx)->caTFAlterQID));
            memcpy((g_tSKClientShm + nFirstIdx)->caOFOrderQID, g_tSKServersShm->caOFOrderQID, sizeof((g_tSKClientShm + nFirstIdx)->caOFOrderQID));
            memcpy((g_tSKClientShm + nFirstIdx)->caFTSOrderQID, g_tSKServersShm->caFTSOrderQID, sizeof((g_tSKClientShm + nFirstIdx)->caFTSOrderQID));
            memcpy((g_tSKClientShm + nFirstIdx)->caFTSAlterQID, g_tSKServersShm->caFTSAlterQID, sizeof((g_tSKClientShm + nFirstIdx)->caFTSAlterQID));

            // 回傳index
            return nFirstIdx;
        }
        */
    }
    catch (exception& e)
    {
        string strMsg = "SKClients_GetIdxFromShmMap catch error, exception: " + string(e.what());
        g_pServers->EnQueueLogger(e_ltServerError, strMsg, false);
    }
    catch (...)
    {
        string strMsg = "SKClients_GetIdxFromShmMap catch fatal error.";
        g_pServers->EnQueueLogger(e_ltServerError, strMsg, false);
    }

    return -1;
}

/// <summary>
/// 檢查SKClient是否有效
/// </summary>
void CSKClients::CheckClientVector()
{
    try
    {
        vector<shared_ptr<CSKClient> >::iterator iter = m_vClient.begin();
        while (iter != m_vClient.end())
        {
            CSKClient* pClient = (*iter).get();
            if (pClient->GetStatus() == csTerminated && (*iter).unique())
            {
                // 補減少連線數: LoginID判斷是否有成功登入過, 斷線時沒有減少連線數的話這邊補減(ex:登入中斷線)
                if (pClient->GetLogin() == true && pClient->GetDecreaseConn() == false)
                {
                    pClient->SetDecreaseConn(true);
                    ReducedClientConn(pClient->GetLoginID(), pClient->GetClientSocketFd());
                }
                EraseClientFromVector(iter);
            }
            else
            {
                iter++;
            }
        }

        string strMsg = "Client Counts:" + to_string(m_vClient.size());
        g_pServers->EnQueueLogger(e_ltServerInfo, strMsg, false);
    }
    catch (exception& e)
    {
        string strMsg = "SKClients_CheckClientVector catch error, exception: " + string(e.what());
        g_pServers->EnQueueLogger(e_ltServerError, strMsg, false);
    }
    catch (...)
    {
        string strMsg = "SKClients_CheckClientVector catch fatal error.";
        g_pServers->EnQueueLogger(e_ltServerError, strMsg, false);
    }
}

/// <summary>
/// 通知前端所有 Client 即將斷線
/// </summary>
/// <param name="strReConnType">斷線原因 1:每日排程 2:切轉</param>
/// <param name="strReConnIP">斷線重連IP/DNS</param>
/// <param name="strReConnPort">斷線重連Port</param>
void CSKClients::ClientReConn(const string& strReConnType/*, const string& strReConnIP, const string& strReConnPort*/)
{
    try
    {
        //string strMsg = "ClientReConn: Type=" + strReConnType + ",IP=" + strReConnIP + ",Port=" + strReConnPort;
        string strMsg = "ClientReConn: Type=" + strReConnType;
        g_pServers->EnQueueLogger(e_ltServerInfo, strMsg, false);

        vector<shared_ptr<CSKClient> >::iterator iter = m_vClient.begin();
        while (iter != m_vClient.end())
        {
            CSKClient* pClient = (*iter).get();
            if (pClient->GetStatus() != csTerminated && pClient->GetStatus() != csOffline && pClient != NULL)
            {
                pClient->SendReConn(strReConnType/*, strReConnIP, strReConnPort*/);
                pClient->SetStatus(csGoingToClose);
            }

            iter++;
        }
    }
    catch (exception& e)
    {
        string strMsg = "SKClients_ClientReConn catch error, exception: " + string(e.what());
        g_pServers->EnQueueLogger(e_ltServerError, strMsg, false);
    }
    catch (...)
    {
        string strMsg = "SKClients_ClientReConn catch fatal error.";
        g_pServers->EnQueueLogger(e_ltServerError, strMsg, false);
    }
}

/// <summary>
/// 客戶當前連線數-1
/// </summary>
/// <param name="nShmIndex">客戶ShmIndex</param>
void CSKClients::ReducedClientConn(const string& strUserID, int nSocketID)
{
    try
    {
        g_pServers->EnQueueDeConn(strUserID, nSocketID);
    }
    catch (...)
    {
    
    }
}

/// <summary>
/// vector新增SKClient
/// </summary>
/// <param name="shpClient">SKClient</param>
void CSKClients::PushBackClientToVector(shared_ptr<CSKClient>& shpClient)
{
    try
    {
        lock_guard<std::mutex> lock(m_mtxClientVectorLock);
        m_vClient.push_back(move(shpClient));
    }
    catch(...)
    {
    }
}

/// <summary>
/// vector移除SKClient
/// </summary>
/// <param name="iter">SKClient</param>
void CSKClients::EraseClientFromVector(vector<shared_ptr<CSKClient> >::iterator iter)
{
    try
    {
        lock_guard<std::mutex> lock(m_mtxClientVectorLock);
        m_vClient.erase(iter);
    }
    catch (...)
    {
    }
}

/// <summary>
/// map新增目前Shm使用中的資料
/// </summary>
/// <param name="strLoginID">客戶ID</param>
/// <param name="nUsingIndex">該客戶使用的shm index</param>
void CSKClients::InsertToShmMap(const string& strLoginID, int nUsingIndex)
{
    m_mapClientShm.insert({ strLoginID, nUsingIndex });
}

/// <summary>
/// queue新增目前閒置中的shm index
/// </summary>
/// <param name="nIdleIndex">閒置中的shm index</param>
void CSKClients::InsertToShmQueue(int nIdleIndex)
{
    m_qIdleIndex.push(nIdleIndex);
}

//void CSKClients::ShutdownClient(int nSocket)
//{
//    if(m_pServerSocket)
//        m_pServerSocket->ShutdownClient(nSocket);
//}
