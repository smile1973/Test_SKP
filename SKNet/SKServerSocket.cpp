#include <string.h>
#include <sstream>
#include <stddef.h>
#include <sys/socket.h>
#include <string>
#include <unistd.h>
#include <errno.h>
#include <cstdio>
#include <iostream>
#include <mutex>
#include <thread>
#include "SKServerSocket.h"
#include "../SKGlobal.h"

using namespace std;

static mutex* mutexes;

static void locking_callback(int mode, int n, const char* file, int line)
{
    if (mode & CRYPTO_LOCK)
    {
        mutexes[n].lock();
    }
    else
    {
        mutexes[n].unlock();
    }
}

static unsigned long thread_id_callback()
{
    return static_cast<unsigned long>(hash<thread::id>()(this_thread::get_id()));
}

void init_openssl_locks()
{
    int num_locks = CRYPTO_num_locks();
    mutexes = new mutex[num_locks];
    CRYPTO_set_locking_callback(locking_callback);
    CRYPTO_set_id_callback(thread_id_callback);
}

void cleanup_openssl_locks()
{
    CRYPTO_set_locking_callback(nullptr);
    CRYPTO_set_id_callback(nullptr);
    delete[] mutexes;
}

CSKServerSocket::CSKServerSocket() 
{
    m_pServerSocketCallback = NULL;
    m_nSocket = 0;
    m_sssServerSocketStatus = sssNone;
    m_AddrRes = NULL;
    memset( &m_AddrInfo, 0 , sizeof( struct addrinfo));
}

CSKServerSocket::CSKServerSocket(ISKServerSocketCallback* pServerSocketCallback)
{
    init_openssl_locks();

    m_pServerSocketCallback = pServerSocketCallback;
    m_sssServerSocketStatus = sssNone;
    m_nSocket = 0;
    m_AddrSize = sizeof(sockaddr_in);
    m_AddrRes = NULL;
    memset(&m_AddrInfo, 0 , sizeof(struct addrinfo));

    m_tvSSLTimeout.tv_sec = 1;
    m_tvSSLTimeout.tv_usec = 0;
    m_tvNoTimeout.tv_sec = 0;
    m_tvNoTimeout.tv_usec = 0;

    // Initialize SSL library
    SSL_library_init();
    // Loading all SSL algorithms
    OpenSSL_add_all_algorithms();
    // Loading all SSL Error Strings
    SSL_load_error_strings();

    // TLS1.2
    m_SslCtx = SSL_CTX_new(TLSv1_2_server_method());
    if (m_SslCtx == NULL)
    {
        ERR_print_errors_fp(stdout);
        exit(1);
    }

    if (SSL_CTX_set_cipher_list(m_SslCtx, "HIGH:!aNULL:!MD5:!RC4") != 1)
    {
        ERR_print_errors_fp(stdout);
        exit(1);
    }

    const long flags = SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1 | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION;
    SSL_CTX_set_options(m_SslCtx, flags);

    // Loading Server Cert (Contains Public Key)
    if (SSL_CTX_use_certificate_file(m_SslCtx, g_strServerCrt.c_str(), SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stdout);
        exit(1);
    }

    // Loading Server Chain Cert
    /*if (SSL_CTX_use_certificate_chain_file(m_SslCtx, g_strServerChainCrt.c_str()) <= 0)
    {
        ERR_print_errors_fp(stdout);
        exit(1);
    }*/

    // Loading Server Private Key
    if (SSL_CTX_use_PrivateKey_file(m_SslCtx, g_strServerKey.c_str(), SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stdout);
        exit(1);
    }

    // Check Private Key
    if (!SSL_CTX_check_private_key(m_SslCtx))
    {
        ERR_print_errors_fp(stdout);
        exit(1);
    }
}

CSKServerSocket::~CSKServerSocket() 
{
    cleanup_openssl_locks();
    SSL_CTX_free(m_SslCtx);
}

void CSKServerSocket::Listen(string strPort, int nBacklog)
{
    //m_SocketStatus = sssStartListeningUp;

    m_AddrInfo.ai_family = AF_INET;
    m_AddrInfo.ai_socktype = SOCK_STREAM;
    m_AddrInfo.ai_flags = AI_PASSIVE;

    getaddrinfo( NULL, strPort.c_str(), &m_AddrInfo, &m_AddrRes);

    // make a socket:
    m_nSocket = socket( m_AddrRes->ai_family, m_AddrRes->ai_socktype, m_AddrRes->ai_protocol);

    // reuse addr!
    int nYes = 1;
    setsockopt(m_nSocket, SOL_SOCKET, SO_REUSEADDR, &nYes, sizeof(nYes));

    // bind!
    int nRs = bind(m_nSocket, m_AddrRes->ai_addr, m_AddrRes->ai_addrlen);
    if(nRs == -1)
    {
        perror("Bind socket error");
        exit(1);
    }

    // listen!
    nRs = listen(m_nSocket, nBacklog);
    if(nRs == -1)
    {
        m_sssServerSocketStatus = sssShutdown;
        perror("Listen error");
        exit(1);
    }
    else
    {
        m_sssServerSocketStatus = sssListening;

        if(m_pServerSocketCallback)
        {
            m_pServerSocketCallback->OnListening();
            cout << "Listening ..." << endl;
        }
    }
}

int CSKServerSocket::Accept(struct sockaddr_in* pClientAddr, SSL** sslClientSSL)
{
    int nSSLRet = -1;
    int nClientSocket = -1 ;
    
    nClientSocket = accept(m_nSocket, (struct sockaddr *)pClientAddr, &m_AddrSize);
    if (nClientSocket == -1)
    {
        *sslClientSSL = NULL;
        return -2;
    }

    if (setsockopt(nClientSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&m_tvSSLTimeout, sizeof(m_tvSSLTimeout)) < 0)
    {
        shutdown(nClientSocket, SHUT_RDWR);
        close(nClientSocket);
        *sslClientSSL = NULL;
        return -1;
    }

    *sslClientSSL = SSL_new(m_SslCtx);
    if (*sslClientSSL == NULL)
    {
        shutdown(nClientSocket, SHUT_RDWR);
        close(nClientSocket);
        *sslClientSSL = NULL;
        return -1;
    }

    nSSLRet = SSL_set_fd(*sslClientSSL, nClientSocket);
    if (nSSLRet == 0)
    {
        SSL_shutdown(*sslClientSSL);
        SSL_free(*sslClientSSL);
        shutdown(nClientSocket, SHUT_RDWR);
        close(nClientSocket);

        *sslClientSSL = NULL;
        return -1;
    }

    nSSLRet = SSL_accept(*sslClientSSL);
    if (nSSLRet <= 0)
    {
        SSL_shutdown(*sslClientSSL);
        SSL_free(*sslClientSSL);
        shutdown(nClientSocket, SHUT_RDWR);
        close(nClientSocket);

        *sslClientSSL = NULL;
        return -1;
    }

    if (setsockopt(nClientSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&m_tvNoTimeout, sizeof(m_tvNoTimeout)) < 0)
    {
        SSL_shutdown(*sslClientSSL);
        SSL_free(*sslClientSSL);
        shutdown(nClientSocket, SHUT_RDWR);
        close(nClientSocket);

        *sslClientSSL = NULL;
        return -1;
    }

    return nClientSocket;
}

void CSKServerSocket::ShutdownServer()
{
    if (m_sssServerSocketStatus == sssListening)
    {
        m_sssServerSocketStatus = sssShutdown;
        shutdown(m_nSocket, SHUT_RDWR); 
        close(m_nSocket);
        m_nSocket = 0;
    }

    freeaddrinfo(m_AddrRes);
}

TSKServerSocketStatus CSKServerSocket::GetStatus()
{
    return m_sssServerSocketStatus;
}

void CSKServerSocket::SetStatus(TSKServerSocketStatus sssType)
{
    m_sssServerSocketStatus = sssType;
}