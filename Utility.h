#ifndef __UTILITY_H__
#define __UTILITY_H__

//#pragma warning(disable: 4786)

#include <iconv.h>
#include <map>
#include <mutex>
#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include <string.h>

using namespace std;

class CUtility
{
private:
    static iconv_t cdB2U;
    static iconv_t cdU2B;
public:
    static mutex m_mIconv;
    static vector<string> DivideByDollar(string strBuf);
    static vector<string> DivideBySharp(string strBuf);
    static vector<string> DivideByComma(string strBuf);
    static vector<string> DivideByEqual(string strBuf);
    static vector<string> DivideByPattern(string strBuf, const string& strPattern);
    //static multimap<string, string> GetJason(const string& strResult);
    //static string GetHostAddress();
    //static string HexToBin(string strBuf);
    //static string Urlencode(const string& strMsg); //URL字串處理
    //static string ConvertUtf8ToBig5(const char* szUtf8); //UTF-8轉BIG5
    //static string ConvertBig5ToUtf8(const char* szBig5); //BIG5轉UTF-8
    static string Trim(const string& strBuf); //清除空白類型字元
    static string TrimCA(const char* pcCA, int nSize); //清除空白類型字元
    static void TrimByRef(string& strRef); //清除空白類型字元(傳址)
    static string TrimSessionID(const string& strBuf); //清除空白類型字元+大括號
    static string TrimCASessionID(const char* pcCA, int nSize); //清除空白類型字元+大括號
    static void TrimSessionIDByRef(string& strBuf); //清除空白類型字元+大括號(傳址)
    static string GetServerIP();

    template<typename ... Args>	static string format(const std::string& format, Args ... args)
    {
        size_t size = snprintf(nullptr, 0, format.c_str(), args ...) + 1;
        unique_ptr<char[]> buf(new char[size]);
        snprintf(buf.get(), size, format.c_str(), args ...);
        return string(buf.get(), buf.get() + size - 1);
    }
};
#endif