#ifndef __SKPRETSALTER_H__
#define __SKPRETSALTER_H__

#include "SKBaseGWOrder.h"

class CSKPreTSAlter : public CSKBaseGWOrder
{
public:
    CSKPreTSAlter(const string& strServer);
    virtual ~CSKPreTSAlter();
    void Run(const TSKTSOrder& tSKOrder, const struct timeval& tvStart, const string& strThreadID);
    int CheckOverTimeToReply(const TSKTSOrder& tSKOrder, const struct timeval& tvStart);
    void SetLogger();
private:
    bool SetPostData(const TSKTSOrder& tSKOrder);
    void ParseGWData(const string& strData) override;
    void SetReplyData() override;
};

#endif