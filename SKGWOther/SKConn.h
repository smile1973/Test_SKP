#ifndef __SKCONN_H__
#define __SKCONN_H__

#include "../SKBaseGW.h"

class CSKConn : public CSKBaseGW
{
public:
    CSKConn(const string& strServer);
    virtual ~CSKConn();
    void Run(const string& strUserID, int nSocketID);
    std::function<bool(TLogType, const string&, bool)> write_log_;

private:
    //bool SetPostData(const string& strUserID, int nSocketID);
    void ParseGWData(const string& strData) override;
    void SetReplyData() override;
    void SendClientCallback() override;
    void EnQueueLogger();
};

#endif