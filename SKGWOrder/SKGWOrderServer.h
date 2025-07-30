#ifndef __SKGWORDERSERVER_H__
#define __SKGWORDERSERVER_H__

#include "../SKClient.h"
#include "../SKGlobal.h"
#include <unistd.h>
#include <sstream>
#include <string>
#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>
using namespace std;

template <typename QType, class SKOrderType>
class CSKGWOrderServer
{
public:
    CSKGWOrderServer()
    {
        m_bTerminated = false;
        m_strURL = "";
        m_nServerType = 0;
        m_nMinThreadCount = 0;
        m_nMaxThreadCount = 0;
        m_nAtomNowThreads = 0;
        m_nAtomIdleThreads = 0;
        m_nIdleLessToNewThread = 0;
        m_nThreadSleepUS = 0;
        m_nCheckOverTimeThreadSleepUS = 100000;
        m_nWarnQueueNum = 40;
    }

    virtual ~CSKGWOrderServer()
    {
        m_bTerminated = true;
    }

    bool EnQueueOrder(shared_ptr<CSKClient>& shpClient, const unsigned char* pcData, const struct timeval& tvStart)
    {
        try
        {
            if (m_bTerminated)
                return false;

            QType* pTSKOrderQ = new QType();
            pTSKOrderQ->shpClient = shpClient;
            memset(&pTSKOrderQ->tSKOrder, 0, sizeof(pTSKOrderQ->tSKOrder));
            memcpy(&pTSKOrderQ->tSKOrder, pcData, sizeof(pTSKOrderQ->tSKOrder));
            pTSKOrderQ->tvStart = tvStart;

            if (true)
            {
                std::unique_lock<std::mutex> lock(m_mtxOrder);
                m_qOrder.push(pTSKOrderQ);
            }

            if (m_qOrder.size() > m_nWarnQueueNum)
            {
                write_log_(e_ltServerData, g_strFuncType[m_tFuncType]+" queue size=" + to_string(m_qOrder.size()), false);
            }

            if(m_nServerType == 1)
                m_cvOrder.notify_one();

            // 當前時間可動態增加thread
            if (g_bIntraday)
                if (m_nAtomIdleThreads <= m_nIdleLessToNewThread) // 當前閒置threads低於threads最小閒置數量時, 新增thread 
                    if (m_nAtomNowThreads < m_nMaxThreadCount) // 當前總threads低於threads最大數量時, 新增thread
                    {
                        m_nAtomNowThreads++;
                        m_nAtomIdleThreads++;
                        StartThread();
                    }

            return true;
        }
        catch (exception& e)
        {
            string strLogMsg = g_strFuncType[m_tFuncType] + " EnQueueOrder Error: " + string(e.what());
            write_log_(e_ltServerError, strLogMsg, false);
        }
        catch (...)
        {
            string strLogMsg = g_strFuncType[m_tFuncType] + " EnQueueOrder Error.";
            write_log_(e_ltServerError, strLogMsg, false);
        }
        return false;
    }

    void SetConfig(TFuncType tFuncType, int nServerType, string strURL, int nMinThreadCount, int nMaxThreadCount, int nIdleLessToNewThread, int nThreadSleepUS)
    {
        m_tFuncType = tFuncType;
        m_nServerType = nServerType;
        
        m_strURL = strURL;
        m_nMinThreadCount = nMinThreadCount;
        m_nMaxThreadCount = nMaxThreadCount;
        m_nIdleLessToNewThread = nIdleLessToNewThread;
        m_nThreadSleepUS = nThreadSleepUS;
    }

    void StartThreads()
    {
        for (int nID = 0; nID < m_nMinThreadCount; nID++)
        {
            m_nAtomNowThreads++;
            m_nAtomIdleThreads++;
            StartThread();
        }
        checkOverTime = std::thread(&CSKGWOrderServer::ThreadReturnOverTime, this);
    }

    size_t GetQueueNum()
    {
        return m_qOrder.size();
    }

    size_t GetThreadNum()
    {
        int nCount = m_nAtomNowThreads;
        return nCount;
    }

    std::function<bool(TLogType, const string&, bool)> write_log_;
    std::function<bool(const TSKLoggerGW&)> write_log_gw_;

private:
    void StartThread()
    {
        thread tNew = std::thread(&CSKGWOrderServer::ThreadsProc, this);
        tNew.detach();
    }

    void ThreadsProc()
    {
        auto tID = this_thread::get_id();
        stringstream ssID;
        ssID << tID;
        string strThreadID = ssID.str();
        string strLogMsg = "";

        try
        {
            strLogMsg = g_strFuncType[m_tFuncType] + " thread " + strThreadID + " has been created! Type=" + to_string(m_nServerType);
            write_log_(e_ltServerInfo, strLogMsg, true);

            // 執行ing
            if(m_nServerType == 0)
                DispatchOrderData_Type0();
            else if(m_nServerType == 1)
                DispatchOrderData_Type1();

            strLogMsg = g_strFuncType[m_tFuncType] + " thread " + strThreadID + " has been deleted! Type=" + to_string(m_nServerType);
            write_log_(e_ltServerInfo, strLogMsg, true);
        }
        catch (exception& e)
        {
            strLogMsg = g_strFuncType[m_tFuncType] + " thread " + strThreadID + " catch error, exception: " + e.what();
            write_log_(e_ltServerError, strLogMsg, true);
        }
        catch (...)
        {
            strLogMsg = g_strFuncType[m_tFuncType] + " thread " + strThreadID + " catch error.";
            write_log_(e_ltServerError, strLogMsg, true);
        }
    }

    void DispatchOrderData_Type0()
    {
        QType* pTSKOrderQ;
        SKOrderType cSKOrder(m_strURL);
        cSKOrder.write_log_gw_ = write_log_gw_;
        cSKOrder.write_log_ = write_log_;
        while (true)
        {
            if (m_bTerminated && m_qOrder.empty())
                break;

            if (true)
            {
                this_thread::sleep_for(chrono::microseconds(m_nThreadSleepUS));
                unique_lock<std::mutex> lock(m_mtxOrder);
                if (m_qOrder.empty())
                {
                    // 當前時間不可動態增加thread
                    if (!g_bIntraday)
                    {
                        if (m_nAtomNowThreads > m_nMinThreadCount) // 當前總threads高於threads最小數量時, 減少thread
                        {
                            m_nAtomNowThreads--;
                            m_nAtomIdleThreads--;
                            break;
                        }
                    }
                    continue;
                }
                pTSKOrderQ = m_qOrder.front();
                m_qOrder.pop();
            }

            if (pTSKOrderQ != NULL)
            {
                m_nAtomIdleThreads--;

                if (pTSKOrderQ->shpClient != NULL)
                {
                    /*auto tID = this_thread::get_id();
                    stringstream ssID;
                    ssID << tID;
                    string strThreadID = ssID.str();*/

                    cSKOrder.SetCallback(pTSKOrderQ->shpClient);
                    cSKOrder.SetClientIP(pTSKOrderQ->shpClient->GetIP());
                    cSKOrder.Run(pTSKOrderQ->tSKOrder, pTSKOrderQ->tvStart, "");
                    cSKOrder.CleanData();
                }

                delete pTSKOrderQ;
                pTSKOrderQ = NULL;

                m_nAtomIdleThreads++;
            }
        }
    }

    void DispatchOrderData_Type1()
    {
        QType* pTSKOrderQ;
        SKOrderType cSKOrder(m_strURL);
        cSKOrder.write_log_gw_ = write_log_gw_;
        cSKOrder.write_log_ = write_log_;
        cv_status cvs_Timeout;

        while (true)
        {
            if (m_bTerminated && m_qOrder.empty())
                break;

            if (true)
            {
                std::unique_lock<std::mutex> lock(m_mtxOrder);
                if (m_qOrder.empty())
                {
                    cvs_Timeout = m_cvOrder.wait_for(lock, chrono::seconds(60));
                    // 當前時間不可動態增加thread && 時間到喚醒
                    if (!g_bIntraday && cvs_Timeout == cv_status::timeout)
                    {
                        if (m_nAtomNowThreads > m_nMinThreadCount) // 當前總threads高於threads最小數量時, 減少thread
                        {
                            m_nAtomNowThreads--;
                            m_nAtomIdleThreads--;
                            break;
                        }
                    }
                    continue;
                }
                pTSKOrderQ = m_qOrder.front();
                m_qOrder.pop();
            }

            if (pTSKOrderQ != NULL)
            {
                m_nAtomIdleThreads--;

                if (pTSKOrderQ->shpClient != NULL)
                {
                    cSKOrder.SetCallback(pTSKOrderQ->shpClient);
                    cSKOrder.SetClientIP(pTSKOrderQ->shpClient->GetIP());

                 /*   auto tID = this_thread::get_id();
                    stringstream ssID;
                    ssID << tID;
                    string strThreadID = ssID.str();*/
                   

                    cSKOrder.Run(pTSKOrderQ->tSKOrder, pTSKOrderQ->tvStart, "");
                    cSKOrder.CleanData();
                }

                delete pTSKOrderQ;
                pTSKOrderQ = NULL;

                m_nAtomIdleThreads++;
            }
        }
    }

    void ThreadReturnOverTime() //20231127
    {
        QType* pTSKOrderQ;
        SKOrderType cSKOrder(m_strURL);
        cSKOrder.write_log_gw_ = write_log_gw_;
        while (true)
        {
            if (m_bTerminated && m_qOrder.empty())
                break;
            this_thread::sleep_for(chrono::microseconds(m_nCheckOverTimeThreadSleepUS));
            while (m_qOrder.size()> m_nWarnQueueNum)
            {
                m_mtxOrder.lock();
                if (!m_qOrder.empty())
                {
                    pTSKOrderQ = m_qOrder.front();
                   // m_qOrder.pop();
                }

                if (pTSKOrderQ != NULL)
                {
                    if (pTSKOrderQ->shpClient != NULL)
                    {
                        cSKOrder.SetCallback(pTSKOrderQ->shpClient);
                        cSKOrder.SetClientIP(pTSKOrderQ->shpClient->GetIP());
                        int result=cSKOrder.CheckOverTimeToReply(pTSKOrderQ->tSKOrder, pTSKOrderQ->tvStart);
                        if (result == 1)
                        {
                            m_qOrder.pop();
                            m_mtxOrder.unlock();
                            cSKOrder.CleanData();
                            write_log_(e_ltServerData, "Order OverTime!", false);
                            g_b_mtxTimeOutOrderNumPer30s.lock();
                            g_bTimeOutOrderNumPer30s = g_bTimeOutOrderNumPer30s + 1;
                            g_b_mtxTimeOutOrderNumPer30s.unlock();
                        }
                        else
                        {
                            m_mtxOrder.unlock();
                            cSKOrder.CleanData();
                            break;
                        }
                    }
                    else
                    {
                        m_mtxOrder.unlock();
                    }
                    delete pTSKOrderQ;
                    pTSKOrderQ = NULL;
                }
                else
                {
                    m_mtxOrder.unlock();
                }
            }
        }
    }

    TFuncType m_tFuncType;
    string m_strURL;
    int m_nServerType;              // Server執行模式: 0=輪詢模式(燒CPU), 1=一般模式 
    int m_nMinThreadCount;          // 最小 Threads 數量
    int m_nMaxThreadCount;          // 最大 Threads 數量
    int m_nIdleLessToNewThread;     // 閒置 Threads 低於此數量時動態新增
    int m_nThreadSleepUS;           // Thread 休眠時間(micro second) Server模式=0時才有效 
    int m_nCheckOverTimeThreadSleepUS; 
    int m_nWarnQueueNum;            //如果此時queue累積的封包數大於此數，需要紀錄在log
    bool m_bTerminated;

    queue<QType*> m_qOrder;
    mutex m_mtxOrder;
    atomic<int> m_nAtomIdleThreads;
    atomic<int> m_nAtomNowThreads;
    condition_variable m_cvOrder;
    thread checkOverTime; 
};

#endif
