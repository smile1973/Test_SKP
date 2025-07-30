#include "Utility.h"
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

mutex CUtility::m_mIconv;
iconv_t CUtility::cdB2U = NULL;
iconv_t CUtility::cdU2B = NULL;

string g_strBlanks("\f\v\r\t\n ");
string g_strSessionBlanks("{}\f\v\r\t\n ");

/// <summary>
/// 將字串依照 '$' 做分割
/// </summary>
/// <param name="strBuf">包含分割字元 '$' 所組成的字串</param>
/// <returns>依照分隔字元所拆解後字串(string)的集合</returns>
vector<string> CUtility::DivideByDollar(string strBuf)
{
    vector<string> Values;

    while (strBuf.find("$") != -1)
    {
        string strValue = strBuf.substr(0, strBuf.find("$"));
        Values.push_back(strValue);

        strBuf = strBuf.substr(strBuf.find("$") + 1, strBuf.length() - 1);
    }

    if (strBuf != "")
        Values.push_back(strBuf);

    return Values;
}

/// <summary>
/// 將字串依照 '#' 做分割
/// </summary>
/// <param name="strBuf">包含分割字元 '#' 所組成的字串</param>
/// <returns>依照分隔字元所拆解後字串(string)的集合</returns>
vector<string> CUtility::DivideBySharp(string strBuf)
{
    vector<string> Values;

    string strTemp;

    while (strBuf != "")
    {
        strTemp = strBuf.find("#") == -1 ?
            strBuf : strBuf.substr(0, strBuf.find("#"));

        if (strTemp != "")
            Values.push_back(strTemp);

        if (strBuf.find("#") == -1)
            break;

        strBuf = strBuf.substr(strBuf.find("#") + 1, strBuf.length() - 1);
    }

    return Values;
}

/// <summary>
/// 將字串依照 ',' 做分割
/// </summary>
/// <param name="strBuf">包含分割字元 ',' 所組成的字串</param>
/// <returns>依照分隔字元所拆解後字串(string)的集合</returns>
vector<string> CUtility::DivideByComma(string strBuf)
{
    vector<string> Values;

    string strTemp;

    while (true)
    {
        strTemp = strBuf.find(",") == -1 ?
            strBuf : strBuf.substr(0, strBuf.find(","));

        //if ( !strTemp.IsEmpty())
        Values.push_back(strTemp);

        if (strBuf.find(",") == -1)
            break;

        strBuf = strBuf.substr(strBuf.find(",") + 1, strBuf.length() - 1);
    }

    return Values;
}

/// <summary>
/// 將字串依照 '=' 做分割
/// </summary>
/// <param name="strBuf">包含分割字元 '=' 所組成的字串</param>
/// <returns>依照分隔字元所拆解後字串(string)的集合</returns>
vector<string> CUtility::DivideByEqual(string strBuf)
{
    short sPos = strBuf.find("=");

    if (sPos == 0)
        throw "'=' Not Found ";

    vector<string> Values;

    Values.push_back(strBuf.substr(0, sPos)); // Key
    Values.push_back(strBuf.substr(sPos + 1, strBuf.length() - 1)); // Value

    return Values;
}

/// <summary>
/// 將字串依照使用者定義的 patten  做分割
/// </summary>
/// <param name="strBuf">以 pattern 當分隔符號所組成的字串</param>
/// <param name="strPattern">分隔符號</param>
/// <returns>依照 pattern 所拆解後字串(string)的集合</returns>
vector<string> CUtility::DivideByPattern(string strBuf, const string& strPattern)
{
    vector<string> Values;

    string strTemp;

    while (true)
    {
        strTemp = strBuf.find(strPattern) == -1 ?
            strBuf : strBuf.substr(0, strBuf.find(strPattern));

        //if ( !strTemp.IsEmpty())
        Values.push_back(strTemp);

        if (strBuf.find(strPattern) == -1)
            break;

        int nlen = strPattern.length();

        strBuf = strBuf.substr(strBuf.find(strPattern) + nlen, strBuf.length() - 1);
    }

    return Values;
}

//multimap<string, string> CUtility::GetJason(const string& strResult)
//{
//    multimap<string, string> Jasons;
//
//    if (strResult == "")
//        return Jasons;
//
//    string strJason = strResult;
//    string strKey = "";
//    string strValue = "";
//    bool bGet = false;
//    int nKey = 1;
//
//    while (true)
//    {
//        int nPos1 = strJason.find("\"");
//        if (nPos1 == -1)
//            break;
//
//        int nPos2 = strJason.find("[");
//        int nPos3 = strJason.find("{");
//
//        if (nPos2 != -1 && nPos2 < nPos1)
//        {
//            strJason = strJason.substr(nPos2 + 1, strJason.length());
//            nKey = 1;
//            continue;
//        }
//        else if (nPos3 != -1 && nPos3 < nPos1)
//        {
//            strJason = strJason.substr(nPos3 + 1, strJason.length());
//            nKey = 1;
//            continue;
//        }
//
//        int nPos5 = strJason.find("[");
//
//        if (nPos5 == nPos1 + 1)
//            strJason = strJason.substr(nPos5, strJason.length());
//        else
//            strJason = strJason.substr(nPos1 + 1, strJason.length());
//
//        int nPos7 = strJason.find("\"");
//        int nPos4 = strJason.find("\\\"");
//
//        string Value;
//
//        if (nPos2 == -1 && nPos4 != -1)
//        {
//            Value = strJason.substr(0, nPos4);
//            strJason = strJason.substr(nPos4 + 2, strJason.length());
//        }
//        else if (nPos5 == nPos1 + 1)
//        {
//            int nPos6 = strJason.find("]");
//            Value = strJason.substr(0, nPos6);
//            strJason = strJason.substr(nPos5, strJason.length());
//        }
//        else
//        {
//            Value = strJason.substr(0, nPos7);
//            strJason = strJason.substr(nPos7 + 1, strJason.length());
//        }
//
//        if (nKey == 1)
//        {
//            strKey = Value;
//            nKey = 2;
//        }
//        else if (nKey == 2)
//        {
//            strValue = Value;
//            nKey = 1;
//
//            Jasons.insert(make_pair(strKey, strValue));
//        }
//    }
//
//    return Jasons;
//}

//string CUtility::GetHostAddress()
//{
//    struct hostent* he;
//    char hostname[100];
//    char ip[16];
//
//    gethostname(hostname, 100);
//    if ((he = gethostbyname(hostname)) == NULL)
//        throw "gethostbyname error";
//
//    strcpy(ip, inet_ntoa(*(struct in_addr*)*he->h_addr_list));
//
//    return ip;
//}

//string CUtility::HexToBin(string strBuf)
//{
//    string straBuf(strBuf);
//    string strResult;
//    int a;
//
//    for (int nPos = 0; nPos< straBuf.length(); nPos += 2)
//    {
//        stringstream ss;
//
//        ss << hex << straBuf.substr(nPos, 2);
//
//        ss >> a;
//
//        strResult += (char)a;
//    }
//
//    return string(strResult);
//}

//string CUtility::Urlencode(const string& strMsg)
//{
//    string strEncode;
//
//    for (int i = 0; i< strMsg.length(); i++)
//    {
//        //unsigned char cChar = ((CStringA)strMsg).GetAt(i);
//        unsigned char cChar = strMsg[i];
//
//        if ((cChar >= 'a' && cChar <= 'z') ||
//            (cChar >= 'A' && cChar <= 'Z') ||
//            (cChar >= '0' && cChar <= '9'))
//        {
//            strEncode += (char)cChar;
//        }
//        else
//        {
//            char caBuf[5] = { 0, };
//
//            sprintf(caBuf, "%%%x", cChar);
//
//            strEncode += caBuf;
//        }
//    }
//
//    return strEncode;
//}

/// <summary>
/// 清除字串頭尾空白類型字元
/// </summary>
/// <param name="strBuf">被清除字串</param>
/// <returns>已清除字串</returns>
string CUtility::Trim(const string& strBuf)
{
    if (strBuf.empty())
    {
        return strBuf;
    }

    string strRes(strBuf), strBlanks("\f\v\r\t\n ");
    strRes.erase(0, strRes.find_first_not_of(strBlanks));
    strRes.erase(strRes.find_last_not_of(strBlanks) + 1);
    return strRes;
}

/// <summary>
/// 清除字元陣列頭尾空白類型字元
/// </summary>
/// <param name="ca">被清除字元陣列</param>
/// <param name="nSize">字元陣列大小</param>
/// <returns>已清除字串</returns>
string CUtility::TrimCA(const char* pcCA, int nSize)
{
    int nS = -1, nE = -1, i;

    for (i = 0; i < nSize; i++)
        if (isspace(*(pcCA + i)) == 0 && *(pcCA + i) != '\x0')
        {
            nS = i;
            break;
        }
    for (i = nSize - 1; i >= 0; i--)
        if (isspace(*(pcCA + i)) == 0 && *(pcCA + i) != '\x0')
        {
            nE = i;
            break;
        }

    if (nE >= nS && nE >= 0 && nS >= 0)
        return string(pcCA + nS, nE - nS + 1);
    else
        return string();
}

/// <summary>
/// 清除字串頭尾空白類型字元
/// </summary>
/// <param name="strRef">(傳址)清除字串</param>
void CUtility::TrimByRef(string& strRef)
{
    if (strRef.empty())
    {
        return;
    }

    strRef.erase(0, strRef.find_first_not_of('\0'));
    strRef.erase(strRef.find_last_not_of('\0') + 1);
    strRef.erase(0, strRef.find_first_not_of(g_strBlanks));
    strRef.erase(strRef.find_last_not_of(g_strBlanks) + 1);
}

/// <summary>
/// SessionID 清除字串頭尾空白類型字元(包含大括號)
/// </summary>
/// <param name="strBuf">被清除字串</param>
/// <returns>已清除字串</returns>
string CUtility::TrimSessionID(const string& strBuf)
{
    if (strBuf.empty())
    {
        return strBuf;
    }

    string strRes(strBuf), strBlanks("{}\f\v\r\t\n ");
    strRes.erase(0, strRes.find_first_not_of(strBlanks));
    strRes.erase(strRes.find_last_not_of(strBlanks) + 1);
    return strRes;
}

/// <summary>
/// SessionID 清除字元陣列頭尾空白類型字元(包含大括號)
/// </summary>
/// <param name="ca">被清除字元陣列</param>
/// <param name="nSize">字元陣列大小</param>
/// <returns>已清除字串</returns>
string CUtility::TrimCASessionID(const char* pcCA, int nSize)
{
    int nS = -1, nE = -1, i;

    for (i = 0; i < nSize; i++)
        if (isspace(*(pcCA + i)) == 0 && *(pcCA + i) != '\x0' && *(pcCA + i) != '{')
        {
            nS = i;
            break;
        }
    for (i = nSize - 1; i >= 0; i--)
        if (isspace(*(pcCA + i)) == 0 && *(pcCA + i) != '\x0' && *(pcCA + i) != '}')
        {
            nE = i;
            break;
        }

    if (nE >= nS && nE >= 0 && nS >= 0)
        return string(pcCA + nS, nE - nS + 1);
    else
        return string();
}

/// <summary>
/// SessionID 清除字串頭尾空白類型字元(包含大括號)
/// </summary>
/// <param name="strRef">(傳址)清除字串</param>
void CUtility::TrimSessionIDByRef(string& strRef)
{
    if (strRef.empty())
    {
        return;
    }

    strRef.erase(0, strRef.find_first_not_of('\0'));
    strRef.erase(strRef.find_last_not_of('\0') + 1);
    strRef.erase(0, strRef.find_first_not_of(g_strSessionBlanks));
    strRef.erase(strRef.find_last_not_of(g_strSessionBlanks) + 1);
}

//string CUtility::ConvertUtf8ToBig5(const char* szUtf8)
//{
//    size_t srclen = strlen(szUtf8) + 1;
//    size_t tarlen = strlen(szUtf8) + 1; 
//    char tmp[tarlen];
//    char *src = new char[srclen], *tar = tmp;
//    strcpy(src, szUtf8);
//    memset(tar, 0, tarlen);
//    {
//        lock_guard<mutex> mLock(m_mIconv);
//        if (!cdU2B)
//            cdU2B = iconv_open("BIG5", "UTF-8");
//        size_t error = iconv(cdU2B, &src, &srclen, &tar, &tarlen);
//    }
//    //iconv_close(cd);
//    return string(tmp);
//}

//string CUtility::ConvertBig5ToUtf8(const char* szBig5)
//{
//    size_t srclen = strlen(szBig5) + 1;
//    size_t tarlen = strlen(szBig5) / 2 * 3 + 1;
//    char tmp[tarlen];
//    char *src = new char[srclen], *tar = tmp;
//    strcpy(src, szBig5);
//    memset(tar, 0, tarlen);
//    {
//        lock_guard<mutex> mLock(m_mIconv);
//        if (!cdB2U)
//            cdB2U = iconv_open("UTF-8", "BIG5");
//        size_t error = iconv(cdB2U, &src, &srclen, &tar, &tarlen);
//        //iconv_close(cdB2U);
//    }
//    return string(tmp);
//}

/// <summary>
/// 取得 ServerIP
/// </summary>
string CUtility::GetServerIP()
{
    struct hostent* hd;
    char cHostname[100];
    char cIp[16];
    gethostname(cHostname, 100);
    if ((hd = gethostbyname(cHostname)) == NULL)
    {
        throw "gethostbyname error";
    }
    strcpy(cIp, inet_ntoa(*(struct in_addr*)*hd->h_addr_list));
    return cIp;
}