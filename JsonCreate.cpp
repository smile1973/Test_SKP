#include "JsonCreate.h"
#include "Utility.h"

CJsonCreate::CJsonCreate()
{
    m_strData = "";
}

string CJsonCreate::GetJsonString()
{
    m_strData = "{" + m_strData.substr(0, m_strData.length() - 1) + "}";
    return m_strData;
}

void CJsonCreate::CleanJsonString()
{
    m_strData = "";
}

void CJsonCreate::InPutString(string strKey, string strValue)
{
    m_strData += "\"" + strKey + "\":\"" + strValue + "\",";
}

void CJsonCreate::InPutInt(string strKey, int nValue)
{
    string strValue;
    strValue = CUtility::format("%d", nValue);
    m_strData += "\"" + strKey + "\":" + strValue + ",";
}

void CJsonCreate::InPutInt(string strKey, string strValue)
{
    if (strValue == "")
        strValue = "0";
    m_strData += "\"" + strKey + "\":" + strValue + ",";
}

void CJsonCreate::InPutDouble(string strKey, double dValue)
{
    string strValue;
    strValue = CUtility::format("%f", dValue);
    m_strData += "\"" + strKey + "\":" + strValue + ",";
}

void CJsonCreate::InPutDouble(string strKey, string strValue)
{
    if (strValue == "")
        strValue = "0";
    m_strData += "\"" + strKey + "\":" + strValue + ",";
}

void CJsonCreate::InPutBoolean(string strKey, bool bValue)
{
    m_strData += "\"" + strKey + "\":" + ((bValue == true) ? "true" : "false") + ",";
}

void CJsonCreate::InPutDateTime(string strKey, std::time_t tDateTime)
{
    string strTime = "";
    strTime = CUtility::format("%d", tDateTime);
    strTime += "000";
    m_strData += "\"" + strKey + "\":\"\\/Date(" + strTime + ")\\/\",";
}

CJsonCreate::~CJsonCreate()
{
}
