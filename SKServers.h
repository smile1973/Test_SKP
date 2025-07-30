#ifndef __SKSERVERS_H__
#define __SKSERVERS_H__

#include "SKFormat.h"
#include "SKClient.h"
#include "SKGWOther/SKLogin.h"
#include "SKGWOther/SKSession.h"
#include "SKGWOrder/SKTSOrder.h"
#include "SKGWOrder/SKTSAlter.h"
#include "SKGWOrder/SKTFOrder.h"
#include "SKGWOrder/SKTFAlter.h"
#include "SKGWOrder/SKOFOrder.h"
#include "SKGWOrder/SKOSOrder.h"
#include "SKGWOrder/SKOSBOrder.h"
#include "SKGWOrder/SKPreTSAlter.h"
#include "SKGWOrder/SKPreTFAlter.h"
#include "SKGWOrder/SKPreOFOrder.h"
#include "SKGWOrder/SKPreOSOrder.h"
#include "SKGWLogger/SKLoggerServer.h"
#include "SKGWOther/SKGWLoginServer.h"
#include "SKGWOrder/SKGWOrderServer.h"
#include "SKGWOther/SKGWConnServer.h"
#include <string>
#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>

using namespace std;

class CSKServers
{
public:
    static CSKServers* GetInstance();
    void SetConfig(struct TSKServersConfig tSKConfig);
    void StartServer();
    void ShmTimer();
    
    bool EnQueueDeConn(const string& strUserID, int nSocketID);
    bool EnQueueLogger(TLogType tLogType, const string& strLogMsg, bool b_Fprintf);
    bool EnQueueLoggerToGW(const struct TSKLoggerGW& tSKLoggerGW);
    bool EnQueueLogin(shared_ptr<CSKClient>& shpClient, const unsigned char* pcData, const struct timeval& tvStart);
    bool EnQueueSession(shared_ptr<CSKClient>& shpClient, const unsigned char* pcData, const struct timeval& tvStart);
    bool EnQueueTSOrder(shared_ptr<CSKClient>& shpClient, const unsigned char* pcData, const struct timeval& tvStart);
    bool EnQueueTSAlter(shared_ptr<CSKClient>& shpClient, const unsigned char* pcData, const struct timeval& tvStart);
    bool EnQueueTFOrder(shared_ptr<CSKClient>& shpClient, const unsigned char* pcData, const struct timeval& tvStart);
    bool EnQueueTFAlter(shared_ptr<CSKClient>& shpClient, const unsigned char* pcData, const struct timeval& tvStart);
    bool EnQueueOFOrder(shared_ptr<CSKClient>& shpClient, const unsigned char* pcData, const struct timeval& tvStart);
    bool EnQueueOSOrder(shared_ptr<CSKClient>& shpClient, const unsigned char* pcData, const struct timeval& tvStart);
    bool EnQueueOSBOrder(shared_ptr<CSKClient>& shpClient, const unsigned char* pcData, const struct timeval& tvStart);
    bool EnQueuePreTSAlter(shared_ptr<CSKClient>& shpClient, const unsigned char* pcData, const struct timeval& tvStart);
    bool EnQueuePreTFAlter(shared_ptr<CSKClient>& shpClient, const unsigned char* pcData, const struct timeval& tvStart);
    bool EnQueuePreOFOrder(shared_ptr<CSKClient>& shpClient, const unsigned char* pcData, const struct timeval& tvStart);
    bool EnQueuePreOSOrder(shared_ptr<CSKClient>& shpClient, const unsigned char* pcData, const struct timeval& tvStart);

    bool CheckSessionID(const string& strLoginID, const string& strSessionID, const string& strSource, const string& strIP);
    CSKServers();
    virtual ~CSKServers();

    bool IsAllQueueEmpty();
private:
  //  CSKServers();
  //  virtual ~CSKServers();
    static CSKServers* instance;
    static mutex m_mtxInstance;

    struct TSKServersConfig m_ServersConfig;
    bool m_bTerminated;

    // Timer
    long m_nNowTime;
    tm m_ptmNowTime;

    CSKGWConnServer* m_SKGWConnServer;
    CSKLoggerServer* m_SKLoggerServer;
    CSKGWLoginServer<TSKLoginQ, CSKLogin>* m_SKLoginServer;
    CSKGWLoginServer<TSKSessionQ, CSKSession>* m_SKSessionServer;
    CSKGWOrderServer<TSKTSOrderQ, CSKTSOrder>* m_TSOrderServer;
    CSKGWOrderServer<TSKTSOrderQ, CSKTSAlter>* m_TSAlterServer;
    CSKGWOrderServer<TSKTFOrderQ, CSKTFOrder>* m_TFOrderServer;
    CSKGWOrderServer<TSKTFOrderQ, CSKTFAlter>* m_TFAlterServer;
    CSKGWOrderServer<TSKOFOrderQ, CSKOFOrder>* m_OFOrderServer;
    CSKGWOrderServer<TSKOSOrderQ, CSKOSOrder>* m_OSOrderServer;
    CSKGWOrderServer<TSKOSOrderQ, CSKOSBOrder>* m_OSBOrderServer;
    CSKGWOrderServer<TSKTSOrderQ, CSKPreTSAlter>* m_PreTSAlterServer;
    CSKGWOrderServer<TSKTFOrderQ, CSKPreTFAlter>* m_PreTFAlterServer;
    CSKGWOrderServer<TSKOFOrderQ, CSKPreOFOrder>* m_PreOFOrderServer;
    CSKGWOrderServer<TSKOSOrderQ, CSKPreOSOrder>* m_PreOSOrderServer;
};

#endif