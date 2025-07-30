#ifndef __SKLOGGERSERVER_H__
#define __SKLOGGERSERVER_H__

#include "SKLogger.h"
#include "../SKClient.h"
#include "../SKGlobal.h"
#include <sstream>
#include <string>
#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>
#include <unistd.h>
#include <sys/file.h>

using namespace std;

extern void FprintfStderrLog(const string& strMsg);

class CSKLoggerServer
{
public:
    CSKLoggerServer()
    {
        m_nAtomNowThreads = 0;
        m_nThreadCountLC = 0;
        m_nThreadCountGW = 0;
        m_bTerminated = false;

        m_strLogName[0] = "ClientInfo";
        m_strLogName[1] = "ClientData";
        m_strLogName[2] = "ClientError";
        m_strLogName[3] = "ServerInfo";
        m_strLogName[4] = "ServerData";
        m_strLogName[5] = "ServerError";

        if (access("./SKProxyLog", R_OK) != 0)
            system("mkdir SKProxyLog");
    }

    virtual ~CSKLoggerServer()
    {
        m_bTerminated = true;
        m_cvLoggerLC.notify_all();
        m_cvLoggerGW.notify_all();
    }

    bool EnQueueLoggerLC(TLogType tLogType, const string& strLocalLog)
    {
        try
        {
            if (m_bTerminated)
                return false;

            TSKLoggerLCQ* pTSKLoggerLCQ = new TSKLoggerLCQ();
            pTSKLoggerLCQ->tLogType = tLogType;
            pTSKLoggerLCQ->strLogMsg.assign(strLocalLog);

            if (true)
            {
                std::unique_lock<std::mutex> lock(m_mtxLoggerLC);
                m_qLoggerLC.push(pTSKLoggerLCQ);
            }
            m_cvLoggerLC.notify_one();

            return true;
        }
        catch (exception& e)
        {
            FprintfStderrLog("EnQueueLogger Error: " + string(e.what()));
        }
        catch (...)
        {
            FprintfStderrLog("EnQueueLogger Error.");
        }
        return false;
    }

    bool EnQueueLoggerGW(const TSKLoggerGW &tSKLoggerGW)
    {
        try
        {
            if (m_bTerminated)
                return false;

            TSKLoggerGWQ* pTSKLoggerGWQ = new TSKLoggerGWQ();
            pTSKLoggerGWQ->tSKLoggerGW.strClientIP.assign(tSKLoggerGW.strClientIP);
            pTSKLoggerGWQ->tSKLoggerGW.strCode.assign(tSKLoggerGW.strCode);
            pTSKLoggerGWQ->tSKLoggerGW.strExceptionMsg.assign(tSKLoggerGW.strExceptionMsg);
            pTSKLoggerGWQ->tSKLoggerGW.strFunc.assign(tSKLoggerGW.strFunc);
            pTSKLoggerGWQ->tSKLoggerGW.strFunction.assign(tSKLoggerGW.strFunction);
            pTSKLoggerGWQ->tSKLoggerGW.strInfo.assign(tSKLoggerGW.strInfo);
            pTSKLoggerGWQ->tSKLoggerGW.strMessage.assign(tSKLoggerGW.strMessage);
            pTSKLoggerGWQ->tSKLoggerGW.strOrderNo.assign(tSKLoggerGW.strOrderNo);
            pTSKLoggerGWQ->tSKLoggerGW.strORKEY.assign(tSKLoggerGW.strORKEY);
            pTSKLoggerGWQ->tSKLoggerGW.strReplyData.assign(tSKLoggerGW.strReplyData);
            pTSKLoggerGWQ->tSKLoggerGW.strSeqNo.assign(tSKLoggerGW.strSeqNo);
            pTSKLoggerGWQ->tSKLoggerGW.strServerIP.assign(tSKLoggerGW.strServerIP);
            pTSKLoggerGWQ->tSKLoggerGW.strType.assign(tSKLoggerGW.strType);
            pTSKLoggerGWQ->tSKLoggerGW.strUserData.assign(tSKLoggerGW.strUserData);
            pTSKLoggerGWQ->tSKLoggerGW.strUserID.assign(tSKLoggerGW.strUserID);
            pTSKLoggerGWQ->tSKLoggerGW.strUseTime.assign(tSKLoggerGW.strUseTime);
            pTSKLoggerGWQ->tSKLoggerGW.strStartTime.assign(tSKLoggerGW.strStartTime);
            pTSKLoggerGWQ->tSKLoggerGW.strEndTime.assign(tSKLoggerGW.strEndTime);
            pTSKLoggerGWQ->tSKLoggerGW.strTotalTime.assign(tSKLoggerGW.strTotalTime);
            if (true)
            {
                std::unique_lock<std::mutex> lock(m_mtxLoggerGW);
                m_qLoggerGW.push(pTSKLoggerGWQ);
            }
            m_cvLoggerGW.notify_one();

            return true;
        }
        catch (exception& e)
        {
            FprintfStderrLog("EnQueueLogger Error: " + string(e.what()));
        }
        catch (...)
        {
            FprintfStderrLog("EnQueueLogger Error.");
        }
        return false;
    }
    void SetConfig(TFuncType tFuncType, string strURL, int nThreadCountLC, int nThreadCountGW)
    {
        m_tFuncType = tFuncType;
        m_strURL = strURL;
        m_nThreadCountLC = nThreadCountLC;
        m_nThreadCountGW = nThreadCountGW;
    }

    void StartThreads()
    {
        for (int nID = 0; nID < m_nThreadCountLC; nID++)
        {
            thread tNew = std::thread(&CSKLoggerServer::ThreadsProc, this, 0);
            tNew.detach();
            m_nAtomNowThreads++;
            //m_mapLoggerThreads[tNew.get_id()] = move(tNew);
        }

        for (int nID = 0; nID < m_nThreadCountGW; nID++)
        {
            thread tNew = std::thread(&CSKLoggerServer::ThreadsProc, this, 1);
            tNew.detach();
            m_nAtomNowThreads++;
            //m_mapLoggerThreads[tNew.get_id()] = move(tNew);
        }
    }

    size_t GetQueueNum()
    {
        return m_qLoggerLC.size() + m_qLoggerGW.size();
    }

    size_t GetThreadNum()
    {
        //return m_mapLoggerThreads.size();
        int nCount = m_nAtomNowThreads;
        return nCount;
    }

    int LoggerGWSize()
    {
        return m_qLoggerGW.size();
    }

    int LoggerLCSize()
    {
        return m_qLoggerLC.size();
    }
private:
    void ThreadsProc(int nType)
    {
        auto tID = this_thread::get_id();
        stringstream ssID;
        ssID << tID;
        string strThreadID = ssID.str();
        string strLogMsg = "";

        try
        {
            strLogMsg = g_strFuncType[m_tFuncType] + " thread " + strThreadID + " has been created!";
            //FprintfStderrLog(strLogMsg);
            WriteLocalLog(e_ltServerInfo, strLogMsg);

            // 執行ing
            switch (nType)
            {
            case 0:
                DispatchLoggerDataLC();
                break;
            case 1:
                DispatchLoggerDataGW();
                break;
            default:
                break;
            }

            m_nAtomNowThreads--;

            // 找到自己ID並移除自己, 辛苦了QQ
            /*auto iter = m_mapLoggerThreads.find(tID);
            if (iter != m_mapLoggerThreads.end())
            {
                if (iter->second.joinable())
                    iter->second.detach();
                m_mapLoggerThreads.erase(iter);
            }*/

            strLogMsg = g_strFuncType[m_tFuncType] + " thread " + strThreadID + " has been deleted!";
            //FprintfStderrLog(strLogMsg);
            WriteLocalLog(e_ltServerInfo, strLogMsg);
        }
        catch (exception& e)
        {
            strLogMsg = g_strFuncType[m_tFuncType] + " thread " + strThreadID + " catch error, exception: " + e.what();
            //FprintfStderrLog(strLogMsg);
            WriteLocalLog(e_ltServerError, strLogMsg);
        }
        catch (...)
        {
            strLogMsg = g_strFuncType[m_tFuncType] + " thread " + strThreadID + " catch error.";
            //FprintfStderrLog(strLogMsg);
            WriteLocalLog(e_ltServerError, strLogMsg);
        }
    }

    void DispatchLoggerDataLC()
    {
        TSKLoggerLCQ* pTSKLoggerLCQ;
        
        while (true)
        {
            if (true)
            {
                std::unique_lock<std::mutex> lock(m_mtxLoggerLC);
                if (m_bTerminated)
                    break;
                if (m_qLoggerLC.empty())
                {
                    m_cvLoggerLC.wait(lock);
                    continue;
                }
                pTSKLoggerLCQ = m_qLoggerLC.front();
                m_qLoggerLC.pop();
            }

            if (pTSKLoggerLCQ != NULL)
            {
                WriteLocalLog(pTSKLoggerLCQ->tLogType, pTSKLoggerLCQ->strLogMsg);

                delete pTSKLoggerLCQ;
                pTSKLoggerLCQ = NULL;
            }
        }
    }

    void DispatchLoggerDataGW()
    {
        TSKLoggerGWQ* pTSKLoggerGWQ;
        CSKLogger cSKLogger(m_strURL);
        cSKLogger.write_local_log_ = std::bind(&CSKLoggerServer::WriteLocalLog, this, std::placeholders::_1, std::placeholders::_2);

        while (true)
        {
            if (true)
            {
                std::unique_lock<std::mutex> lock(m_mtxLoggerGW);
                if (m_bTerminated)
                    break;
                if (m_qLoggerGW.empty())
                {
                    m_cvLoggerGW.wait(lock);
                    continue;
                }
                pTSKLoggerGWQ = m_qLoggerGW.front();
                m_qLoggerGW.pop();
            }

            if (pTSKLoggerGWQ != NULL)
            {
                /*auto tID = this_thread::get_id();
                stringstream ssID;
                ssID << tID;
                string strThreadID = ssID.str();*/

                cSKLogger.Run(pTSKLoggerGWQ->tSKLoggerGW, "");
                cSKLogger.CleanData();

                delete pTSKLoggerGWQ;
                pTSKLoggerGWQ = NULL;
            }
        }
    }

    /// <summary>
    /// 寫LOG進檔案
    /// </summary>
    /// <param name="tLogType">LOG模式</param>
    /// <param name="strMsg">文字</param>
    void WriteLocalLog(TLogType tLogType, const string& strMsg)
    {
        try
        {
            FILE* flog;
            struct timeval tv_NowTime;
            struct tm tm_NowTime;
            int writeSuccess = 0;
            gettimeofday(&tv_NowTime, NULL);
            localtime_r(&tv_NowTime.tv_sec, &tm_NowTime);

            char cDate[25];
            char cWriteData[4096];
            memset(cWriteData, 0, 4096);
            strftime(cDate, 25, "%Y/%m/%d %H:%M:%S", &tm_NowTime); //日期時間(無毫秒)
            string strDate(cDate);

            memset(cDate, 0, 25);
            strftime(cDate, 25, "%Y%m%d", &tm_NowTime);	//日期
            string strLogDate(cDate);

            string strLogName = "./SKProxyLog/" + strLogDate + "_" + m_strLogName[tLogType];

            lock_guard<mutex> lg_Log(m_mtxLog[tLogType]);
            int fd;
            if (tLogType == e_ltServerError)
            {
                while (writeSuccess != 1)
                {
                    fd = open(strLogName.c_str(), O_RDWR | O_CREAT | O_APPEND, 0666);
                    if (fd != -1)
                    {
                        if (flock(fd, LOCK_EX | LOCK_NB) != 0)
                        {
                            sleep(1);
                        }
                        else
                        {
                            snprintf(cWriteData,sizeof(cWriteData), "[%s.%06ld]%s\n", strDate.c_str(), tv_NowTime.tv_usec, strMsg.c_str());
                            string strWriteData= cWriteData;
                            write(fd, strWriteData.c_str(), strlen(strWriteData.c_str()));

                            writeSuccess = 1;
                        }
                        flock(fd, LOCK_UN);
                        close(fd);
                    }

                }
              

            }
            else
            {
                if ((flog = fopen(strLogName.c_str(), "a")) != NULL)
                {
                    fprintf(flog, "[%s.%06ld] %s\n", strDate.c_str(), tv_NowTime.tv_usec, strMsg.c_str());
                    fclose(flog);
                }
            }
        }
        catch (...)
        {
        }
    }

    TFuncType m_tFuncType;
    string m_strURL;
    bool m_bTerminated;
    int m_nThreadCountLC;
    int m_nThreadCountGW;

    //map<thread::id, thread> m_mapLoggerThreads;

    // Local Log
    queue<TSKLoggerLCQ*> m_qLoggerLC;
    mutex m_mtxLoggerLC;
    condition_variable m_cvLoggerLC;

    // GW Log
    queue<TSKLoggerGWQ*> m_qLoggerGW;
    mutex m_mtxLoggerGW;
    condition_variable m_cvLoggerGW;

    mutex m_mtxLog[e_LogTypeEnd];        //!< 檔案Lock
    string m_strLogName[e_LogTypeEnd];   //!< 檔案名稱

    atomic<int> m_nAtomNowThreads;
};

#endif