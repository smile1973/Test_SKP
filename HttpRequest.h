#ifndef __HTTPREQUEST_H__
#define __HTTPREQUEST_H__

#include <string>
#include <curl/curl.h>
#include <sstream>
#include "SKGlobal.h"
//enum TRequestMthod { rmNone, rmGet, rmPost };

using namespace std;

class CHttpRequest
{
public:
    static void GlobalInit();
    CHttpRequest();
    CHttpRequest(const CHttpRequest& Request) = delete;
    CHttpRequest& operator=(const CHttpRequest&) = delete;
    ~CHttpRequest();
    //CURLcode Post(const string& strURL, const string& strPostData, string& strResponse);       //post方式傳輸
    CURLcode Post(const string& strURL, const string& strPostData, stringstream& ssResponse,const string& strThreadID); //post方式傳輸
    static size_t req_reply(void* ptr, size_t size, size_t nmemb, void* stream);	     //回傳結果寫入
private:
    CURL* m_Curl;
    long m_Timeout;
    void InitCurl();
};

#endif