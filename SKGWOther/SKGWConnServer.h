#ifndef __SKGWCONNSERVER_H__
#define __SKGWCONNSERVER_H__

#include "SKConn.h"
#include "../SKClient.h"
#include "../SKGlobal.h"
#include <sstream>
#include <string>
#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>

using namespace std;

class CSKGWConnServer
{
public:
    CSKGWConnServer()
    {
        m_nAtomNowThreads = 0;
        m_strURL = "";
        m_nThreadCount = 0;
        m_bTerminated = false;
    }

    virtual ~CSKGWConnServer()
    {
        m_bTerminated = true;
        m_cvDeConn.notify_all();
    }

    bool EnQueueDeConn(const string& strUserID, int nSocketID)
    {
        try
        {
            if (m_bTerminated)
                return false;

            TSKDeConnQ* pTSKDeConnQ = new TSKDeConnQ();
            pTSKDeConnQ->strUserID.assign(strUserID);
            pTSKDeConnQ->nSocketID = nSocketID;

            if (true)
            {
                std::unique_lock<std::mutex> lock(m_mtxDeConn);
                m_qDeConn.push(pTSKDeConnQ);
            }
            m_cvDeConn.notify_one();

            return true;
        }
        catch (exception& e)
        {
            string strLogMsg = "GWConnServer EnQueueDeConn Error: " + string(e.what());
            write_log_(e_ltServerError, strLogMsg, true);
        }
        catch (...)
        {
            string strLogMsg = "GWConnServer EnQueueDeConn Error.";
            write_log_(e_ltServerError, strLogMsg, true);
        }
        return false;
    }

    void SetConfig(string strURL, int nThreadCount)
    {
        m_strURL = strURL;
        m_nThreadCount = nThreadCount;
    }

    void StartThreads()
    {
        for (int nID = 0; nID < m_nThreadCount; nID++)
        {
            thread tNew = std::thread(&CSKGWConnServer::ThreadsProc, this);
            tNew.detach();
            m_nAtomNowThreads++;
            //m_mapThreads[tNew.get_id()] = move(tNew);
        }
    }

    size_t GetQueueNum()
    {
        return m_qDeConn.size();
    }

    std::function<bool(TLogType, const string&, bool)> write_log_;

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
            strLogMsg = "GWConn thread " + strThreadID + " has been created!";
            write_log_(e_ltServerInfo, strLogMsg, true);

            // 執行ing
            DispatchDeConnData();
            m_nAtomNowThreads--;
            // 找到自己ID並移除自己, 辛苦了QQ
            /*auto iter = m_mapThreads.find(tID);
            if (iter != m_mapThreads.end())
            {
                if (iter->second.joinable())
                    iter->second.detach();
                m_mapThreads.erase(iter);
            }*/

            strLogMsg = "GWConn thread " + strThreadID + " has been deleted!";
            write_log_(e_ltServerInfo, strLogMsg, true);
        }
        catch (exception& e)
        {
            strLogMsg = "GWConn thread " + strThreadID + " catch error, exception: " + e.what();
            write_log_(e_ltServerError, strLogMsg, true);
        }
        catch (...)
        {
            strLogMsg = "GWConn thread " + strThreadID + " catch error.";
            write_log_(e_ltServerError, strLogMsg, true);
        }
    }

    void DispatchDeConnData()
    {
        TSKDeConnQ* pTSKDeConnQ;
        CSKConn cSKConn(m_strURL);
        cSKConn.write_log_ = write_log_;

        while (true)
        {
            if (true)
            {
                std::unique_lock<std::mutex> lock(m_mtxDeConn);
                if (m_bTerminated)
                    break;
                if (m_qDeConn.empty())
                {
                    m_cvDeConn.wait(lock);
                    continue;
                }
                pTSKDeConnQ = m_qDeConn.front();
                m_qDeConn.pop();
            }

            if (pTSKDeConnQ != NULL)
            {
                cSKConn.Run(pTSKDeConnQ->strUserID, pTSKDeConnQ->nSocketID);
                cSKConn.CleanData();

                delete pTSKDeConnQ;
                pTSKDeConnQ = NULL;
            }
        }
    }

    string m_strURL;
    bool m_bTerminated;
    int m_nThreadCount;

    //map<thread::id, thread> m_mapThreads;
    queue<TSKDeConnQ*> m_qDeConn;
    mutex m_mtxDeConn;
    condition_variable m_cvDeConn;
    atomic<int> m_nAtomNowThreads;
};

#endif