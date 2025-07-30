#include "SKServers.h"
#include <sstream>
#include <unistd.h>
#include <iomanip>

extern void FprintfStderrLog(const string& strMsg);
extern int Atoi(char* ptr, int n);

CSKServers* CSKServers::instance = NULL;
mutex CSKServers::m_mtxInstance;

CSKServers::CSKServers()
{
    m_bTerminated = false;
}

CSKServers::~CSKServers()
{
    m_bTerminated = true;
}

CSKServers* CSKServers::GetInstance()
{
    if (instance == NULL)
    {
        lock_guard<std::mutex> lock(m_mtxInstance);

        if (instance == NULL)
        {
            instance = new CSKServers();
            FprintfStderrLog("SERVERS ONE");
        }
    }

    return instance;
}

/// <summary>
/// 設定基本資訊
/// </summary>
/// <param name="tSKConfig">Config檔</param>
void CSKServers::SetConfig(struct TSKServersConfig tSKConfig)
{
    m_ServersConfig = tSKConfig;
}

/// <summary>
/// 更新Shm
/// </summary>
void CSKServers::ShmTimer()
{
    try
    {
        // 讀取當前時間
        m_nNowTime = time(0);
        localtime_r(&m_nNowTime, &m_ptmNowTime);

        // 判斷 每日即將重啟通知 時間
        int nDailyReConnMin = Atoi(g_tSKServersShm->caDailyReConnMin, sizeof(g_tSKServersShm->caDailyReConnMin));
        int nDailyReConnHour = Atoi(g_tSKServersShm->caDailyReConnHour, sizeof(g_tSKServersShm->caDailyReConnHour));
        if (m_ptmNowTime.tm_min == nDailyReConnMin && m_ptmNowTime.tm_hour == nDailyReConnHour)
        {
            memcpy(g_tSKServersShm->caReConnType, "1", sizeof("1"));
            memcpy(g_tSKServersShm->caReConn, "Y", sizeof("Y"));
        }

        // 判斷 動態增加Thread 時間
        int nIntradayStart = Atoi(g_tSKServersShm->caIntradayStart, sizeof(g_tSKServersShm->caIntradayStart));
        int nIntradayEnd = Atoi(g_tSKServersShm->caIntradayEnd, sizeof(g_tSKServersShm->caIntradayEnd));
        if (m_ptmNowTime.tm_hour >= nIntradayStart && m_ptmNowTime.tm_hour < nIntradayEnd)
        {
            g_bIntraday = true;
            memcpy(g_tSKServersShm->caIntraday, "1", sizeof(g_tSKServersShm->caIntraday));
        }
        else
        {
            g_bIntraday = false;
            memcpy(g_tSKServersShm->caIntraday, "0", sizeof(g_tSKServersShm->caIntraday));
        }

        // 修改 g_tSKServersShm 當前 Queue 數量
        size_t nQueueNum, nThreadNum;
        char caQueueNum[sizeof(g_tSKServersShm->caLoggerQNum)];
        char caThreadNum[sizeof(g_tSKServersShm->caLoggerThNum)];
        string strQueueNum, strThreadNum;

        // 遍尋所有 FuncType
        for (int nFunc = 0; nFunc < e_FuncTypeEnd; nFunc++)
        {
            // 清空參數
            strQueueNum = strThreadNum = "";
            nQueueNum = nThreadNum = 0;
            memset(caQueueNum, 0, sizeof(caQueueNum));
            memset(caThreadNum, 0, sizeof(caThreadNum));

            // 根據不同功能代號取不同Queue的數量
            switch (nFunc)
            {
            case e_ftLogger: nQueueNum = m_SKLoggerServer->GetQueueNum(); nThreadNum = m_SKLoggerServer->GetThreadNum(); break;
            case e_ftLogin: nQueueNum = m_SKLoginServer->GetQueueNum(); nThreadNum = m_SKLoginServer->GetThreadNum(); break;
            case e_ftSession: nQueueNum = m_SKSessionServer->GetQueueNum(); nThreadNum = m_SKSessionServer->GetThreadNum(); break;
            case e_ftTSOrder: nQueueNum = m_TSOrderServer->GetQueueNum(); nThreadNum = m_TSOrderServer->GetThreadNum(); break;
            case e_ftTSAlter: nQueueNum = m_TSAlterServer->GetQueueNum(); nThreadNum = m_TSAlterServer->GetThreadNum(); break;
            case e_ftTFOrder: nQueueNum = m_TFOrderServer->GetQueueNum(); nThreadNum = m_TFOrderServer->GetThreadNum(); break;
            case e_ftTFAlter: nQueueNum = m_TFAlterServer->GetQueueNum(); nThreadNum = m_TFAlterServer->GetThreadNum(); break;
            case e_ftOFOrder: nQueueNum = m_OFOrderServer->GetQueueNum(); nThreadNum = m_OFOrderServer->GetThreadNum(); break;
            case e_ftFTSOrder: nQueueNum = 0; nThreadNum = 0; break;
            case e_ftFTSAlter: nQueueNum = 0; nThreadNum = 0; break;
            case e_ftOSOrder: nQueueNum = m_OSOrderServer->GetQueueNum(); nThreadNum = m_OSOrderServer->GetThreadNum(); break;
            case e_ftOSBOrder: nQueueNum = m_OSBOrderServer->GetQueueNum(); nThreadNum = m_OSBOrderServer->GetThreadNum(); break;
            case e_ftPreTSAlter: nQueueNum = m_PreTSAlterServer->GetQueueNum(); nThreadNum = m_PreTSAlterServer->GetThreadNum(); break;
            case e_ftPreTFAlter: nQueueNum = m_PreTFAlterServer->GetQueueNum(); nThreadNum = m_PreTFAlterServer->GetThreadNum(); break;
            case e_ftPreOFOrder: nQueueNum = m_PreOFOrderServer->GetQueueNum(); nThreadNum = m_PreOFOrderServer->GetThreadNum(); break;
            case e_ftPreOSOrder: nQueueNum = m_PreOSOrderServer->GetQueueNum(); nThreadNum = m_PreOSOrderServer->GetThreadNum(); break;
            default:
                break;
            }

            // Queue量>99999, Shm大小不足, 只顯示99999
            if (nQueueNum > 99999)
                memcpy(caQueueNum, "99999", sizeof(caQueueNum));
            else
            {
                strQueueNum = to_string(nQueueNum);
                memcpy(caQueueNum, strQueueNum.c_str(), strQueueNum.size());
            }

            // Thread量>999, Shm大小不足, 只顯示999
            if (nThreadNum > 999)
                memcpy(caThreadNum, "999", sizeof(caThreadNum));
            else
            {
                strThreadNum = to_string(nThreadNum);
                memcpy(caThreadNum, strThreadNum.c_str(), strThreadNum.size());
            }

            // 根據不同功能代號, 寫入g_tSKServersShm不同欄位
            switch (nFunc)
            {
            case e_ftLogger: 
                memcpy(g_tSKServersShm->caLoggerQNum, caQueueNum, sizeof(g_tSKServersShm->caLoggerQNum));
                memcpy(g_tSKServersShm->caLoggerThNum, caThreadNum, sizeof(g_tSKServersShm->caLoggerThNum));
                break;
            case e_ftLogin: 
                memcpy(g_tSKServersShm->caLoginQNum, caQueueNum, sizeof(g_tSKServersShm->caLoginQNum));
                memcpy(g_tSKServersShm->caLoginThNum, caThreadNum, sizeof(g_tSKServersShm->caLoginThNum));
                break;
            case e_ftSession:
                memcpy(g_tSKServersShm->caSessionQNum, caQueueNum, sizeof(g_tSKServersShm->caSessionQNum));
                memcpy(g_tSKServersShm->caSessionThNum, caThreadNum, sizeof(g_tSKServersShm->caSessionThNum));
                break;
            case e_ftTSOrder:
                memcpy(g_tSKServersShm->caTSOrderQNum, caQueueNum, sizeof(g_tSKServersShm->caTSOrderQNum));
                memcpy(g_tSKServersShm->caTSOrderThNum, caThreadNum, sizeof(g_tSKServersShm->caTSOrderThNum));
                break;
            case e_ftTSAlter:
                memcpy(g_tSKServersShm->caTSAlterQNum, caQueueNum, sizeof(g_tSKServersShm->caTSAlterQNum));
                memcpy(g_tSKServersShm->caTSAlterThNum, caThreadNum, sizeof(g_tSKServersShm->caTSAlterThNum));
                break;
            case e_ftTFOrder:
                memcpy(g_tSKServersShm->caTFOrderQNum, caQueueNum, sizeof(g_tSKServersShm->caTFOrderQNum));
                memcpy(g_tSKServersShm->caTFOrderThNum, caThreadNum, sizeof(g_tSKServersShm->caTFOrderThNum));
                break;
            case e_ftTFAlter:
                memcpy(g_tSKServersShm->caTFAlterQNum, caQueueNum, sizeof(g_tSKServersShm->caTFAlterQNum));
                memcpy(g_tSKServersShm->caTFAlterThNum, caThreadNum, sizeof(g_tSKServersShm->caTFAlterThNum));
                break;
            case e_ftOFOrder:
                memcpy(g_tSKServersShm->caOFOrderQNum, caQueueNum, sizeof(g_tSKServersShm->caOFOrderQNum));
                memcpy(g_tSKServersShm->caOFOrderThNum, caThreadNum, sizeof(g_tSKServersShm->caOFOrderThNum));
                break;
            case e_ftFTSOrder:
                memcpy(g_tSKServersShm->caFTSOrderQNum, caQueueNum, sizeof(g_tSKServersShm->caFTSOrderQNum));
                memcpy(g_tSKServersShm->caFTSOrderThNum, caThreadNum, sizeof(g_tSKServersShm->caFTSOrderThNum));
                break;
            case e_ftFTSAlter:
                memcpy(g_tSKServersShm->caFTSAlterQNum, caQueueNum, sizeof(g_tSKServersShm->caFTSAlterQNum));
                memcpy(g_tSKServersShm->caFTSAlterThNum, caThreadNum, sizeof(g_tSKServersShm->caFTSAlterThNum));
                break;
            case e_ftOSOrder:
                memcpy(g_tSKServersShm->caOSOrderQNum, caQueueNum, sizeof(g_tSKServersShm->caOSOrderQNum));
                memcpy(g_tSKServersShm->caOSOrderThNum, caThreadNum, sizeof(g_tSKServersShm->caOSOrderThNum));
                break;
            case e_ftOSBOrder:
                memcpy(g_tSKServersShm->caOSBOrderQNum, caQueueNum, sizeof(g_tSKServersShm->caOSBOrderQNum));
                memcpy(g_tSKServersShm->caOSBOrderThNum, caThreadNum, sizeof(g_tSKServersShm->caOSBOrderThNum));
                break;
            case e_ftPreTSAlter:
                memcpy(g_tSKServersShm->caPreTSAlterQNum, caQueueNum, sizeof(g_tSKServersShm->caPreTSAlterQNum));
                memcpy(g_tSKServersShm->caPreTSAlterThNum, caThreadNum, sizeof(g_tSKServersShm->caPreTSAlterThNum));
                break;
            case e_ftPreTFAlter:
                memcpy(g_tSKServersShm->caPreTFAlterQNum, caQueueNum, sizeof(g_tSKServersShm->caPreTFAlterQNum));
                memcpy(g_tSKServersShm->caPreTFAlterThNum, caThreadNum, sizeof(g_tSKServersShm->caPreTFAlterThNum));
                break;
            case e_ftPreOFOrder:
                memcpy(g_tSKServersShm->caPreOFOrderQNum, caQueueNum, sizeof(g_tSKServersShm->caPreOFOrderQNum));
                memcpy(g_tSKServersShm->caPreOFOrderThNum, caThreadNum, sizeof(g_tSKServersShm->caPreOFOrderThNum));
                break;
            case e_ftPreOSOrder:
                memcpy(g_tSKServersShm->caPreOSOrderQNum, caQueueNum, sizeof(g_tSKServersShm->caPreOSOrderQNum));
                memcpy(g_tSKServersShm->caPreOSOrderThNum, caThreadNum, sizeof(g_tSKServersShm->caPreOSOrderThNum));
                break;
            default:
                break;
            }
        }
    }
    catch (exception& e)
    {
        string strMsg = "SKServers_ShmTimer catch error, exception: " + string(e.what());
        EnQueueLogger(e_ltServerError, strMsg, false);
    }
    catch (...)
    {
        string strMsg = "SKServers_ShmTimer catch fatal error.";
        EnQueueLogger(e_ltServerError, strMsg, false);
    }
}

/// <summary>
/// 啟動各功能Server
/// </summary>
void CSKServers::StartServer()
{
#ifdef CPUMODE
    int nMode = 0;
#else
    int nMode = 1;
#endif

    m_SKGWConnServer = new CSKGWConnServer();
    m_SKLoggerServer = new CSKLoggerServer();
    m_SKLoginServer = new CSKGWLoginServer<TSKLoginQ, CSKLogin>();
    m_SKSessionServer = new CSKGWLoginServer<TSKSessionQ, CSKSession>();
    m_TSOrderServer = new CSKGWOrderServer<TSKTSOrderQ, CSKTSOrder>();
    m_TSAlterServer = new CSKGWOrderServer<TSKTSOrderQ, CSKTSAlter>();
    m_TFOrderServer = new CSKGWOrderServer<TSKTFOrderQ, CSKTFOrder>();
    m_TFAlterServer = new CSKGWOrderServer<TSKTFOrderQ, CSKTFAlter>();
    m_OFOrderServer = new CSKGWOrderServer<TSKOFOrderQ, CSKOFOrder>();
    m_OSOrderServer = new CSKGWOrderServer<TSKOSOrderQ, CSKOSOrder>();
    m_OSBOrderServer = new CSKGWOrderServer<TSKOSOrderQ, CSKOSBOrder>();
    m_PreTSAlterServer = new CSKGWOrderServer<TSKTSOrderQ, CSKPreTSAlter>();
    m_PreTFAlterServer = new CSKGWOrderServer<TSKTFOrderQ, CSKPreTFAlter>();
    m_PreOFOrderServer = new CSKGWOrderServer<TSKOFOrderQ, CSKPreOFOrder>();
    m_PreOSOrderServer = new CSKGWOrderServer<TSKOSOrderQ, CSKPreOSOrder>();

    m_SKGWConnServer->write_log_ = std::bind(&CSKServers::EnQueueLogger, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    m_SKLoginServer->write_log_ = std::bind(&CSKServers::EnQueueLogger, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    m_SKSessionServer->write_log_ = std::bind(&CSKServers::EnQueueLogger, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    m_TSOrderServer->write_log_ = std::bind(&CSKServers::EnQueueLogger, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    m_TSAlterServer->write_log_ = std::bind(&CSKServers::EnQueueLogger, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    m_TFOrderServer->write_log_ = std::bind(&CSKServers::EnQueueLogger, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    m_TFAlterServer->write_log_ = std::bind(&CSKServers::EnQueueLogger, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    m_OFOrderServer->write_log_ = std::bind(&CSKServers::EnQueueLogger, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    m_OSOrderServer->write_log_ = std::bind(&CSKServers::EnQueueLogger, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    m_OSBOrderServer->write_log_ = std::bind(&CSKServers::EnQueueLogger, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    m_PreTSAlterServer->write_log_ = std::bind(&CSKServers::EnQueueLogger, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    m_PreTFAlterServer->write_log_ = std::bind(&CSKServers::EnQueueLogger, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    m_PreOFOrderServer->write_log_ = std::bind(&CSKServers::EnQueueLogger, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    m_PreOSOrderServer->write_log_ = std::bind(&CSKServers::EnQueueLogger, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    m_SKLoginServer->write_log_gw_ = std::bind(&CSKServers::EnQueueLoggerToGW, this, std::placeholders::_1);
    m_SKSessionServer->write_log_gw_ = std::bind(&CSKServers::EnQueueLoggerToGW, this, std::placeholders::_1);
    m_TSOrderServer->write_log_gw_ = std::bind(&CSKServers::EnQueueLoggerToGW, this, std::placeholders::_1);
    m_TSAlterServer->write_log_gw_ = std::bind(&CSKServers::EnQueueLoggerToGW, this, std::placeholders::_1);
    m_TFOrderServer->write_log_gw_ = std::bind(&CSKServers::EnQueueLoggerToGW, this, std::placeholders::_1);
    m_TFAlterServer->write_log_gw_ = std::bind(&CSKServers::EnQueueLoggerToGW, this, std::placeholders::_1);
    m_OFOrderServer->write_log_gw_ = std::bind(&CSKServers::EnQueueLoggerToGW, this, std::placeholders::_1);
    m_OSOrderServer->write_log_gw_ = std::bind(&CSKServers::EnQueueLoggerToGW, this, std::placeholders::_1);
    m_OSBOrderServer->write_log_gw_ = std::bind(&CSKServers::EnQueueLoggerToGW, this, std::placeholders::_1);
    m_PreTSAlterServer->write_log_gw_ = std::bind(&CSKServers::EnQueueLoggerToGW, this, std::placeholders::_1);
    m_PreTFAlterServer->write_log_gw_ = std::bind(&CSKServers::EnQueueLoggerToGW, this, std::placeholders::_1);
    m_PreOFOrderServer->write_log_gw_ = std::bind(&CSKServers::EnQueueLoggerToGW, this, std::placeholders::_1);
    m_PreOSOrderServer->write_log_gw_ = std::bind(&CSKServers::EnQueueLoggerToGW, this, std::placeholders::_1);

    m_SKGWConnServer->SetConfig(m_ServersConfig.strConnURL, m_ServersConfig.nGWConnThreadCount);
    m_SKGWConnServer->StartThreads();

    m_SKLoggerServer->SetConfig(e_ftLogger, m_ServersConfig.strLoggerURL, m_ServersConfig.nLoggerThreadCountLC, m_ServersConfig.nLoggerThreadCountGW);
    m_SKLoggerServer->StartThreads();

    m_SKLoginServer->SetConfig(e_ftLogin, m_ServersConfig.strLoginURL, m_ServersConfig.strConnURL, m_ServersConfig.strLoginSalesURL, m_ServersConfig.nLoginThreadCount);
    m_SKLoginServer->StartThreads();

    m_SKSessionServer->SetConfig(e_ftSession, m_ServersConfig.strSessionURL, m_ServersConfig.strConnURL, "", m_ServersConfig.nSessionThreadCount);
    m_SKSessionServer->StartThreads();

    m_TSOrderServer->SetConfig(e_ftTSOrder, nMode,
        m_ServersConfig.strTSOrderURL,
        m_ServersConfig.nTSOrderMinThreadCount,
        m_ServersConfig.nTSOrderMaxThreadCount,
        m_ServersConfig.nTSOrderIdleLessToNewThread,
        m_ServersConfig.nTSOrderThreadSleepUS);
    m_TSOrderServer->StartThreads();

    m_TSAlterServer->SetConfig(e_ftTSAlter, nMode,
        m_ServersConfig.strTSAlterURL,
        m_ServersConfig.nTSAlterMinThreadCount,
        m_ServersConfig.nTSAlterMaxThreadCount,
        m_ServersConfig.nTSAlterIdleLessToNewThread,
        m_ServersConfig.nTSAlterThreadSleepUS);
    m_TSAlterServer->StartThreads();

    m_TFOrderServer->SetConfig(e_ftTFOrder, nMode,
        m_ServersConfig.strTFOrderURL,
        m_ServersConfig.nTFOrderMinThreadCount,
        m_ServersConfig.nTFOrderMaxThreadCount,
        m_ServersConfig.nTFOrderIdleLessToNewThread,
        m_ServersConfig.nTFOrderThreadSleepUS);
    m_TFOrderServer->StartThreads();

    m_TFAlterServer->SetConfig(e_ftTFAlter, nMode,
        m_ServersConfig.strTFAlterURL,
        m_ServersConfig.nTFAlterMinThreadCount,
        m_ServersConfig.nTFAlterMaxThreadCount,
        m_ServersConfig.nTFAlterIdleLessToNewThread,
        m_ServersConfig.nTFAlterThreadSleepUS);
    m_TFAlterServer->StartThreads();

    m_OFOrderServer->SetConfig(e_ftOFOrder, nMode,
        m_ServersConfig.strOFOrderURL,
        m_ServersConfig.nOFOrderMinThreadCount,
        m_ServersConfig.nOFOrderMaxThreadCount,
        m_ServersConfig.nOFOrderIdleLessToNewThread,
        m_ServersConfig.nOFOrderThreadSleepUS);
    m_OFOrderServer->StartThreads();

    m_OSOrderServer->SetConfig(e_ftOSOrder, 1,
        m_ServersConfig.strOSOrderURL,
        m_ServersConfig.nOSOrderMinThreadCount,
        m_ServersConfig.nOSOrderMaxThreadCount,
        m_ServersConfig.nOSOrderIdleLessToNewThread,
        m_ServersConfig.nOSOrderThreadSleepUS);
    m_OSOrderServer->StartThreads();

    m_OSBOrderServer->SetConfig(e_ftOSBOrder, 1,
        m_ServersConfig.strOSBOrderURL,
        m_ServersConfig.nOSBOrderMinThreadCount,
        m_ServersConfig.nOSBOrderMaxThreadCount,
        m_ServersConfig.nOSBOrderIdleLessToNewThread,
        m_ServersConfig.nOSBOrderThreadSleepUS);
    m_OSBOrderServer->StartThreads();

    m_PreTSAlterServer->SetConfig(e_ftPreTSAlter, nMode,
        m_ServersConfig.strPreTSAlterURL,
        m_ServersConfig.nPreTSAlterMinThreadCount,
        m_ServersConfig.nPreTSAlterMaxThreadCount,
        m_ServersConfig.nPreTSAlterIdleLessToNewThread,
        m_ServersConfig.nPreTSAlterThreadSleepUS);
    m_PreTSAlterServer->StartThreads();

    m_PreTFAlterServer->SetConfig(e_ftPreTFAlter, nMode,
        m_ServersConfig.strPreTFAlterURL,
        m_ServersConfig.nPreTFAlterMinThreadCount,
        m_ServersConfig.nPreTFAlterMaxThreadCount,
        m_ServersConfig.nPreTFAlterIdleLessToNewThread,
        m_ServersConfig.nPreTFAlterThreadSleepUS);
    m_PreTFAlterServer->StartThreads();

    m_PreOFOrderServer->SetConfig(e_ftPreOFOrder, nMode,
        m_ServersConfig.strPreOFOrderURL,
        m_ServersConfig.nPreOFOrderMinThreadCount,
        m_ServersConfig.nPreOFOrderMaxThreadCount,
        m_ServersConfig.nPreOFOrderIdleLessToNewThread,
        m_ServersConfig.nPreOFOrderThreadSleepUS);
    m_PreOFOrderServer->StartThreads();

    m_PreOSOrderServer->SetConfig(e_ftPreOSOrder, nMode,
        m_ServersConfig.strPreOSOrderURL,
        m_ServersConfig.nPreOSOrderMinThreadCount,
        m_ServersConfig.nPreOSOrderMaxThreadCount,
        m_ServersConfig.nPreOSOrderIdleLessToNewThread,
        m_ServersConfig.nPreOSOrderThreadSleepUS);
    m_PreOSOrderServer->StartThreads();
}

bool CSKServers::EnQueueDeConn(const string& strUserID, int nSocketID)
{
    if (m_bTerminated)
        return false;

    return m_SKGWConnServer->EnQueueDeConn(strUserID, nSocketID);
}

bool CSKServers::EnQueueLogger(TLogType tLogType, const string& strLogMsg, bool b_Fprintf)
{
    if (m_bTerminated)
        return false;

    if (b_Fprintf)
        FprintfStderrLog(strLogMsg);

    return m_SKLoggerServer->EnQueueLoggerLC(tLogType, strLogMsg);
}

bool CSKServers::EnQueueLoggerToGW(const struct TSKLoggerGW& tSKLoggerGW)
{
    if (m_bTerminated)
        return false;

    return m_SKLoggerServer->EnQueueLoggerGW(tSKLoggerGW);
}

bool CSKServers::EnQueueLogin(shared_ptr<CSKClient>& shpClient, const unsigned char* pcData, const struct timeval& tvStart)
{
    if (m_bTerminated)
        return false;

    return m_SKLoginServer->EnQueueLogin(shpClient, pcData, tvStart);
}
bool CSKServers::EnQueueSession(shared_ptr<CSKClient>& shpClient, const unsigned char* pcData, const struct timeval& tvStart)
{
    if (m_bTerminated)
        return false;

    return m_SKSessionServer->EnQueueLogin(shpClient, pcData, tvStart);
}
bool CSKServers::EnQueueTSOrder(shared_ptr<CSKClient>& shpClient, const unsigned char* pcData, const struct timeval& tvStart)
{
    if (m_bTerminated)
        return false;

    return m_TSOrderServer->EnQueueOrder(shpClient, pcData, tvStart);
}
bool CSKServers::EnQueueTSAlter(shared_ptr<CSKClient>& shpClient, const unsigned char* pcData, const struct timeval& tvStart)
{
    if (m_bTerminated)
        return false;

    return m_TSAlterServer->EnQueueOrder(shpClient, pcData, tvStart);
}
bool CSKServers::EnQueueTFOrder(shared_ptr<CSKClient>& shpClient, const unsigned char* pcData, const struct timeval& tvStart)
{
    if (m_bTerminated)
        return false;

    return m_TFOrderServer->EnQueueOrder(shpClient, pcData, tvStart);
}
bool CSKServers::EnQueueTFAlter(shared_ptr<CSKClient>& shpClient, const unsigned char* pcData, const struct timeval& tvStart)
{
    if (m_bTerminated)
        return false;

    return m_TFAlterServer->EnQueueOrder(shpClient, pcData, tvStart);
}
bool CSKServers::EnQueueOFOrder(shared_ptr<CSKClient>& shpClient, const unsigned char* pcData, const struct timeval& tvStart)
{
    if (m_bTerminated)
        return false;

    return m_OFOrderServer->EnQueueOrder(shpClient, pcData, tvStart);
}
bool CSKServers::EnQueueOSOrder(shared_ptr<CSKClient>& shpClient, const unsigned char* pcData, const struct timeval& tvStart)
{
    if (m_bTerminated)
        return false;

    return m_OSOrderServer->EnQueueOrder(shpClient, pcData, tvStart);
}
bool CSKServers::EnQueueOSBOrder(shared_ptr<CSKClient>& shpClient, const unsigned char* pcData, const struct timeval& tvStart)
{
    if (m_bTerminated)
        return false;

    return m_OSBOrderServer->EnQueueOrder(shpClient, pcData, tvStart);
}

bool CSKServers::EnQueuePreTSAlter(shared_ptr<CSKClient>& shpClient, const unsigned char* pcData, const struct timeval& tvStart)
{
    if (m_bTerminated)
        return false;

    return m_PreTSAlterServer->EnQueueOrder(shpClient, pcData, tvStart);
}

bool CSKServers::EnQueuePreTFAlter(shared_ptr<CSKClient>& shpClient, const unsigned char* pcData, const struct timeval& tvStart)
{
    if (m_bTerminated)
        return false;

    return m_PreTFAlterServer->EnQueueOrder(shpClient, pcData, tvStart);
}

bool CSKServers::EnQueuePreOFOrder(shared_ptr<CSKClient>& shpClient, const unsigned char* pcData, const struct timeval& tvStart)
{
    if (m_bTerminated)
        return false;

    return m_PreOFOrderServer->EnQueueOrder(shpClient, pcData, tvStart);
}

bool CSKServers::EnQueuePreOSOrder(shared_ptr<CSKClient>& shpClient, const unsigned char* pcData, const struct timeval& tvStart)
{
    if (m_bTerminated)
        return false;

    return m_PreOSOrderServer->EnQueueOrder(shpClient, pcData, tvStart);
}

bool CSKServers::CheckSessionID(const string& strLoginID, const string& strSessionID, const string& strSource, const string& strIP)
{
    bool bSuccess = CSKSession::RunSync(m_ServersConfig.strSessionURL, strLoginID, strSessionID, strSource, strIP);
    return bSuccess;
}

bool CSKServers::IsAllQueueEmpty()
{
    if (m_SKLoggerServer->GetQueueNum() > 0 || m_SKLoginServer->GetQueueNum() > 0 || m_SKSessionServer->GetQueueNum() >0 || 
        m_TSOrderServer->GetQueueNum() >0 || m_TSAlterServer->GetQueueNum() > 0 || m_TFOrderServer->GetQueueNum() > 0 || m_TFAlterServer->GetQueueNum() > 0 ||
        m_OFOrderServer->GetQueueNum() > 0 || m_OSOrderServer->GetQueueNum() > 0 || m_OSBOrderServer->GetQueueNum() > 0 || m_PreTSAlterServer->GetQueueNum() > 0 ||
        m_PreTFAlterServer->GetQueueNum() > 0 || m_PreOFOrderServer->GetQueueNum() > 0 || m_SKGWConnServer->GetQueueNum() > 0 || m_PreOSOrderServer->GetQueueNum() > 0)
        return false;
    else
    {
        return true;
    }

}