#include <cstdio>
#include <sys/time.h>
#include <string>
using namespace std;

void FprintfStderrLog(const timeval& tvStart, const timeval& tvEnd)
{
    struct tm tmstart, tmend;
    tmstart = *localtime(&tvStart.tv_sec);
    tmend = *localtime(&tvEnd.tv_sec);

    fprintf(stderr, "[%02d:%02d:%02d.%06d] Start\n", tmstart.tm_hour, tmstart.tm_min, tmstart.tm_sec, tvStart.tv_usec);
    fprintf(stderr, "[%02d:%02d:%02d.%06d] End\n", tmend.tm_hour, tmend.tm_min, tmend.tm_sec, tvEnd.tv_usec);
}

/// <summary>
/// 顯示文字(含當前時間)
/// </summary>
/// <param name="strMsg">顯示文字</param>
void FprintfStderrLog(const string& strMsg)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm tm = *localtime(&tv.tv_sec);

    fprintf(stderr, "[%02d:%02d:%02d.%06d] %s\n", tm.tm_hour, tm.tm_min, tm.tm_sec, tv.tv_usec, strMsg.c_str());
}

void FprintfStderrLog(const char* pCause, int nError, int nData, const char* pFile = NULL, int nLine = 0,
    unsigned char* pMessage1 = NULL, int nMessage1Length = 0, 
    unsigned char* pMessage2 = NULL, int nMessage2Length = 0)
{
    struct timeval tv;
    //time_t t = time(NULL);
    gettimeofday(&tv,NULL);
    struct tm tm = *localtime(&tv.tv_sec);

    fprintf(stderr, "[%02d:%02d:%02d'%06d]", tm.tm_hour, tm.tm_min, tm.tm_sec, tv.tv_usec);
    fprintf(stderr, "[%s][%d,%d]", pCause, nError, nData);
    fprintf(stderr, "%s:%d", pFile, nLine);
    fprintf(stderr, "{");

    for(int i=0;i<nMessage1Length;i++)
    {
        if(isprint(pMessage1[i]))
        {
            fprintf(stderr, "%c", pMessage1[i]);
        }
        else
        {
            fprintf(stderr, "[%#x]", pMessage1[i]);
        }
    }
    fprintf(stderr, "}");

    fprintf(stderr, "{");
    for(int i=0;i<nMessage2Length;i++)
    {
        if(isprint(pMessage2[i]))
        {
            fprintf(stderr, "%c", pMessage2[i]);
        }
        else
        {
            fprintf(stderr, "[%#x]", pMessage2[i]);
        }
    }

    fprintf(stderr, "}\n");
}