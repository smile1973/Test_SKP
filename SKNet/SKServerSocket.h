#ifndef __SKSERVERSOCKET_H__
#define __SKSERVERSOCKET_H__

#include <string>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "../SKInterface/ISKServerSocketCallback.h"

using namespace std;

enum TSKServerSocketStatus 
{
    sssNone,
    sssListening,
    sssShutdown
};

class CSKServerSocket 
{
public:
    CSKServerSocket();
    CSKServerSocket(ISKServerSocketCallback* pServerSocketCallback);
    virtual ~CSKServerSocket();

    void Listen(string strPort, int nBacklog = 20);
    int Accept(struct sockaddr_in* pClientAddr, SSL** sslClientSSL);

    void ShutdownServer();

    TSKServerSocketStatus GetStatus();
    void SetStatus(TSKServerSocketStatus sssType);

private:
    ISKServerSocketCallback* m_pServerSocketCallback;

    int m_nSocket;
    socklen_t m_AddrSize;
    SSL_CTX* m_SslCtx;
    struct timeval m_tvSSLTimeout;
    struct timeval m_tvNoTimeout;

    TSKServerSocketStatus m_sssServerSocketStatus;

    struct addrinfo m_AddrInfo;
    struct addrinfo* m_AddrRes;
};
#endif
