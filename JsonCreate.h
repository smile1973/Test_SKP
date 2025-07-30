#ifndef __JSONCREATE_H__
#define __JSONCREATE_H__

#include <ctime>
#include <string>

using namespace std;

class CJsonCreate
{
private:
    string m_strData;

protected:


public:
    CJsonCreate();
    ~CJsonCreate();
    string GetJsonString();
    void CleanJsonString();

    void InPutString(string strKey, string strValue);
    void InPutInt(string strKey, int nValue);
    void InPutInt(string strKey, string strValue);
    void InPutDouble(string strKey, double dValue);
    void InPutDouble(string strKey, string strValue);
    void InPutBoolean(string strKey, bool bValue);
    void InPutDateTime(string strKey, std::time_t tDateTime);
};

#endif