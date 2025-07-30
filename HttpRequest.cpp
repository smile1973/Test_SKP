#include "HttpRequest.h"
#include <openssl/crypto.h>
#include <pthread.h>

bool g_CUrlGlobalInitReady = false;
static CURLSH* curlShareHandle= NULL;

void CHttpRequest::GlobalInit()
{
    if (!g_CUrlGlobalInitReady)
    {
        g_CUrlGlobalInitReady = true;
        if (!curlShareHandle)
        {
            curlShareHandle = curl_share_init();
            if (curlShareHandle == NULL)
            {
                exit(1);
            }
           // curl_share_setopt(curlShareHandle, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS); //不支持多thread
        }
        curl_global_init(CURL_GLOBAL_ALL);
    }
}

CHttpRequest::CHttpRequest()
{
    m_Curl = NULL;
    m_Timeout = 120;
    InitCurl();
}

CHttpRequest::~CHttpRequest()
{
    curl_easy_cleanup(m_Curl);
    if (m_Curl != NULL)
    {
        m_Curl = NULL;
    }
}

void CHttpRequest::InitCurl()
{
    m_Curl = curl_easy_init();
    if (m_Curl == NULL)
    {
        exit(1);
    }

    long cacheTimeOut = 60 * 5;

    //// share
    curl_easy_setopt(m_Curl, CURLOPT_SHARE, curlShareHandle);
    curl_easy_setopt(m_Curl, CURLOPT_DNS_CACHE_TIMEOUT, cacheTimeOut);

    // init
    curl_easy_setopt(m_Curl, CURLOPT_VERBOSE, 0L);
    curl_easy_setopt(m_Curl, CURLOPT_READFUNCTION, NULL);
    curl_easy_setopt(m_Curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(m_Curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(m_Curl, CURLOPT_TCP_KEEPALIVE, 1L);           // enable TCP keep-alive for this transfer
    curl_easy_setopt(m_Curl, CURLOPT_TCP_KEEPIDLE, 10L);           // set keep-alive idle time to x seconds
    curl_easy_setopt(m_Curl, CURLOPT_TCP_KEEPINTVL, 10L);          // interval time between keep-alive probes: x seconds
    curl_easy_setopt(m_Curl, CURLOPT_CONNECTTIMEOUT, m_Timeout); // set transport and time out time
    curl_easy_setopt(m_Curl, CURLOPT_TIMEOUT, m_Timeout);
    curl_easy_setopt(m_Curl, CURLOPT_WRITEFUNCTION, req_reply);
}

/// <summary>
/// Http回傳資料寫入變數
/// </summary>
/// <param name="ptr">接收到的資料緩衝區指標</param>
/// <param name="size">每個元素的大小</param>
/// <param name="nmemb">緩衝區元素個數</param>
/// <param name="stream">用戶自定義的指針, 可以在回調函數中使用</param>
/// <returns>數據大小</returns>
size_t CHttpRequest::req_reply(void* ptr, size_t size, size_t nmemb, void* stream)
{
    if (ptr == NULL || stream == NULL)
        return 0;

    stringstream* response_stream = static_cast<stringstream*>(stream);
    
    response_stream->write(static_cast<char*>(ptr), size * nmemb);
    
    return size * nmemb;
}

/// <summary>
/// POST方式傳輸
/// </summary>
/// <param name="strURL">傳輸網址</param>
/// <param name="strPostData">Post資料</param>
/// <param name="strResponse">回傳資料(傳址)</param>
/// <returns>Curl代碼</returns>
//CURLcode CHttpRequest::Post(const string& strURL, const string& strPostData, string &strResponse)
//{
//    curl_easy_setopt(m_Curl, CURLOPT_URL, strURL.c_str());
//    curl_easy_setopt(m_Curl, CURLOPT_POST, 1);                         // post req
//    curl_easy_setopt(m_Curl, CURLOPT_POSTFIELDS, strPostData.c_str()); // params
//    curl_easy_setopt(m_Curl, CURLOPT_WRITEDATA, (void*)&strResponse);
//
//    CURLcode res;
//    if (m_Curl)
//    {
//        res = curl_easy_perform(m_Curl);
//    }
//    return res;
//}

CURLcode CHttpRequest::Post(const string& strURL, const string& strPostData, stringstream& ssResponse, const string& strThreadID)
{
    CURLcode res;

    if (m_Curl != NULL)
    {
        curl_easy_setopt(m_Curl, CURLOPT_URL, strURL.c_str());
        if(g_nHttpMode==0)
            curl_easy_setopt(m_Curl, CURLOPT_FORBID_REUSE, 1L);
        curl_easy_setopt(m_Curl, CURLOPT_POST, 1L);                         // post req
        curl_easy_setopt(m_Curl, CURLOPT_POSTFIELDS, strPostData.c_str()); // params
        curl_easy_setopt(m_Curl, CURLOPT_WRITEDATA, (void *)&ssResponse);

        res = curl_easy_perform(m_Curl);
    }
    return res;
}
