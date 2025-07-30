#ifndef __SKLOGGER_H__
#define __SKLOGGER_H__

#include "../SKBaseGW.h"

class CSKLogger : public CSKBaseGW
{
public:
    CSKLogger(const string& strServer);
    virtual ~CSKLogger();
    void Run(const TSKLoggerGW &tSKLoggerGW, const string& strThreadID);
    std::function<void(TLogType, const string&)> write_local_log_;

private:
    bool SetPostData(const TSKLoggerGW& tSKLoggerGW);
    void ParseGWData(const string& strData) override;
    void SetReplyData() override;
    void SendClientCallback() override;
    void EnQueueLogger();
    string TrimUserData(const string& strUserData);
    void TrimPostJson();
};
#endif