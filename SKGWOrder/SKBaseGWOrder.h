#ifndef __SKBASEGWORDER_H__
#define __SKBASEGWORDER_H__

#include "../SKBaseGW.h"

class CSKBaseGWOrder : public CSKBaseGW
{
public:
    CSKBaseGWOrder(const string& strServer) : CSKBaseGW(strServer)
    {
        m_strORKEY = "";
        m_strOrderNo = "";
        m_strSeqNo = "";
        memset(m_uncaReplyData, 0, sizeof(m_uncaReplyData));
    };
    
    virtual ~CSKBaseGWOrder()
    {
    };

    /// <summary>
    /// 清除資料
    /// </summary>
    virtual void CleanData()
    {
        CSKBaseGW::CleanData();

        m_strORKEY = "";
        m_strOrderNo = "";
        m_strSeqNo = "";
        memset(m_uncaReplyData, 0, sizeof(m_uncaReplyData));
    }

protected:
    /// <summary>
    /// 發送回傳資料給Client
    /// </summary>
    virtual void SendClientCallback() override
    {
        struct TSKReplyData tskReplyData;
        memcpy(tskReplyData.uncaReplyData, m_uncaReplyData, sizeof(m_uncaReplyData));

        tskReplyData.tSKLoggerGW = m_tSKLoggerGW;
        tskReplyData.tvStart = m_tvStart;
        tskReplyData.tvSendT = m_tvSendT;
      
        if (m_shpClient != NULL)
           m_shpClient->SendReply("GW_ORDER_REPLY", tskReplyData, sizeof(tskReplyData.uncaReplyData));

    }

    // Non-const Variable
    string m_strORKEY;      //!< Client 單號識別碼 (User自帶)
    string m_strOrderNo;    //!< 委託書號 (GW回傳)
    string m_strSeqNo;      //!< 委託序號 (GW回傳)
    unsigned char m_uncaReplyData[PACKET_HEADER_LENGTH + ORDER_REP_LENGTH]; //!< 回傳Client訊息
    TSKLoggerGW m_tSKLoggerGW;
    struct timeval m_tvStart;
    struct timeval m_tvSendT;
    string m_strStartT;
    
};
#endif