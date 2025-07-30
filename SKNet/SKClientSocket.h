#ifndef __SKCLIENTSOCKET_H__
#define __SKCLIENTSOCKET_H__

#include <string>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "../SKInterface/ISKClientSocketCallback.h"

using namespace std;

enum TSKClientSocketStatus 
{
    cssNone,
    cssConnecting,
    cssConnected,
    cssDisconnect
};


class CSKClientSocket 
{
public:
    CSKClientSocket();
    CSKClientSocket(ISKClientSocketCallback* pClientSocketCallback);
    virtual ~CSKClientSocket();

    void Connect(string strHost, string strPort);
    void Disconnect();

    int Send(const unsigned char* pBuf, int nSize);
    int Recv(unsigned char* pBuf, int nSize);

    int Recv();

    TSKClientSocketStatus GetStatus();

protected:
    void Close();

private:
    ISKClientSocketCallback* m_pClientSocketCallback;

    int m_nSocket;

    TSKClientSocketStatus m_cssClientSocketStatus;

    struct addrinfo m_AddrInfo;
    struct addrinfo* m_AddrRes;
};
#endif
