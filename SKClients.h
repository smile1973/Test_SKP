#ifndef __CSKCLIENTS_H__
#define __CSKCLIENTS_H__

#include <string>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <vector>
#include <memory>
#include <mutex>
#include <iostream>
#include <openssl/rsa.h>
#include <openssl/bn.h>
#include <queue>
#include "SKInterface/ISKServerSocketCallback.h"
#include "SKThread.h"
#include "SKNet/SKServerSocket.h"
#include "SKClient.h" 
#include "SKGlobal.h"

using namespace std;

class CSKClients: public CSKThread, public ISKServerSocketCallback
{
public:
    static CSKClients* GetInstance();
    void SetConfig(string& strListenPort, int nClientShmSize);

    void ShmTimer();
    void InsertToShmMap(const string& strLoginID, int nUsingIndex);
    void InsertToShmQueue(int nIdleIndex);
    int GetIdxFromShmMap(const string& strLoginID);
    void CheckClientVector();
    void ClientReConn(const string& strReConnType/*, const string& strReConnIP, const string& strReConnPort*/);
    void PushBackClientToVector(shared_ptr<CSKClient>& shpClient);
    void EraseClientFromVector(vector<shared_ptr<CSKClient> >::iterator iter);
    void ShutdownServer();
    CSKClients();
    virtual ~CSKClients();
    void CheckBeforeDisConn(vector<shared_ptr<CSKClient> >::iterator iter);
protected:
    void Run();
    void OnListening();
    void OnShutdown();

private:
    //CSKClients();
    //virtual ~CSKClients();
    static CSKClients* instance;
    static mutex m_mtxInstance;
    void ReducedClientConn(const string &strUserID, int nSocketID);

    CSKServerSocket* m_pServerSocket;
    struct timeval m_tvSendTimeout;
    int m_nClientShmSize;                                       //!< ClientShm 數量
    string m_strListenPort;                                     //!< 監聽中的Port
    vector<shared_ptr<CSKClient>> m_vClient;                    //!< 儲存當前連線的Vector
    map<string/*LoginID*/, int/*Using Index*/> m_mapClientShm;  //!< 目前Shm有的客戶
    queue<int/*Idle Index*/> m_qIdleIndex;                      //!< 目前Shm可用的位置Index
    //mutex m_mtxaShmLock[CLIENT_SHM_SIZE];                       //!< 每個位置Index對應的Lock
    mutex m_mtxClientShmMapLock;                                //!< ClientShmMap Lock
    mutex m_mtxClientVectorLock;                                //!< ClientVector Lock
};
#endif
