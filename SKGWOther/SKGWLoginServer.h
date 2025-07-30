#ifndef __SKGWLOGINSERVER_H__
#define __SKGWLOGINSERVER_H__

#include "../SKClient.h"
#include "../SKGlobal.h"
#include <sstream>
#include <string>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <condition_variable>

using namespace std;

extern void FprintfStderrLog(const string& strMsg);

template <typename QType, class SKLoginType>
class CSKGWLoginServer
{
public:
    CSKGWLoginServer()
    {
        m_nAtomNowThreads = 0;
        m_strURL = "";
        m_strConnURL = "";
        m_strLoginSalesURL = "";
        m_nThreadCount = 0;
        m_bTerminated = false;
    }

    virtual ~CSKGWLoginServer()
    {
        m_bTerminated = true;
        m_cvLogin.notify_all();
    }

    bool EnQueueLogin(shared_ptr<CSKClient>& shpClient, const unsigned char* pcData, const struct timeval& tvStart)
    {
        try
        {
            if (m_bTerminated)
                return false;

            QType* pTSKLoginQ = new QType();
            pTSKLoginQ->shpClient = shpClient;
            memset(&pTSKLoginQ->tSKData, 0, sizeof(pTSKLoginQ->tSKData));
            memcpy(&pTSKLoginQ->tSKData, pcData, sizeof(pTSKLoginQ->tSKData));
            pTSKLoginQ->tvStart = tvStart;

            if (true)
            {
                std::unique_lock<std::mutex> lock(m_mtxLogin);
                m_qLogin.push(pTSKLoginQ);
            }
            m_cvLogin.notify_one();

            return true;
        }
        catch (exception& e)
        {
            string strLogMsg = g_strFuncType[m_tFuncType] + " EnQueueLogin Error: " + string(e.what());
            write_log_(e_ltServerError, strLogMsg, true);
        }
        catch (...)
        {
            string strLogMsg = g_strFuncType[m_tFuncType] + " EnQueueLogin Error.";
            write_log_(e_ltServerError, strLogMsg, true);
        }
        return false;
    }

    void SetConfig(TFuncType tFuncType, string strURL, string strConnURL, string strLoginSalesURL, int nThreadCount)
    {
        m_tFuncType = tFuncType;
        m_strURL = strURL;
        m_strConnURL = strConnURL;
        m_strLoginSalesURL = strLoginSalesURL;
        m_nThreadCount = nThreadCount;
    }

    void StartThreads()
    {
        for (int nID = 0; nID < m_nThreadCount; nID++)
        {
            thread tNew = std::thread(&CSKGWLoginServer::ThreadsProc, this);
            tNew.detach();
            m_nAtomNowThreads++;
            //m_mapThreads[tNew.get_id()] = move(tNew);
        }
    }

    size_t GetQueueNum()
    {
        return m_qLogin.size();
    }

    size_t GetThreadNum()
    {
        //return m_mapThreads.size();
        int nCount = m_nAtomNowThreads;
        return nCount;
    }

    std::function<bool(TLogType, const string&, bool)> write_log_;
    std::function<bool(const TSKLoggerGW&)> write_log_gw_;

private:
    void ThreadsProc()
    {
        auto tID = this_thread::get_id();
        stringstream ssID;
        ssID << tID;
        string strThreadID = ssID.str();
        string strLogMsg = "";

        try
        {
            strLogMsg = g_strFuncType[m_tFuncType] + " thread " + strThreadID + " has been created!";
            write_log_(e_ltServerInfo, strLogMsg, true);

            // 執行ing
            DispatchLoginData();
            m_nAtomNowThreads--;
            // 找到自己ID並移除自己, 辛苦了QQ
            /*auto iter = m_mapThreads.find(tID);
            if (iter != m_mapThreads.end())
            {
                if (iter->second.joinable())
                    iter->second.detach();
                m_mapThreads.erase(iter);
            }*/
            
            strLogMsg = g_strFuncType[m_tFuncType] + " thread " + strThreadID + " has been deleted!";
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

    void DispatchLoginData()
    {
        QType* pTSKLoginQ;
        SKLoginType cSKLogin(m_strURL, m_strConnURL, m_strLoginSalesURL);
        cSKLogin.write_log_gw_ = write_log_gw_;
        cSKLogin.write_log_ = write_log_;

        while (true)
        {
            if (true)
            {
                std::unique_lock<std::mutex> lock(m_mtxLogin);
                if (m_bTerminated)
                    break;
                if (m_qLogin.empty())
                {
                    m_cvLogin.wait(lock);
                    continue;
                }
                pTSKLoginQ = m_qLogin.front();
                m_qLogin.pop();
            }

            if (pTSKLoginQ != NULL)
            {
                if (pTSKLoginQ->shpClient != NULL)
                {
                    cSKLogin.SetCallback(pTSKLoginQ->shpClient);
                    cSKLogin.SetClientIP(pTSKLoginQ->shpClient->GetIP());
                    cSKLogin.Run(pTSKLoginQ->tSKData, pTSKLoginQ->tvStart);
                    cSKLogin.CleanData();
                }

                delete pTSKLoginQ;
                pTSKLoginQ = NULL;
            }
        }
    }

    TFuncType m_tFuncType;
    string m_strURL;
    string m_strConnURL;
    string m_strLoginSalesURL;
    bool m_bTerminated;
    int m_nThreadCount;

    //map<thread::id, thread> m_mapThreads;
    queue<QType*> m_qLogin;
    mutex m_mtxLogin;
    condition_variable m_cvLogin;
    atomic<int> m_nAtomNowThreads;
};

#endif