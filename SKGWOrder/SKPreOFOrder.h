#ifndef __SKPREOFORDER_H__
#define __SKPREOFORDER_H__

#include "SKBaseGWOrder.h"

class CSKPreOFOrder : public CSKBaseGWOrder
{
public:
    CSKPreOFOrder(const string& strServer);
    virtual ~CSKPreOFOrder();
    void Run(const TSKOFOrder& tSKOrder, const struct timeval& tvStart, const string& strThreadID);
    int CheckOverTimeToReply(const TSKOFOrder& tSKOrder, const struct timeval& tvStart);
    void SetLogger();
private:
    bool SetPostData(const TSKOFOrder& tSKOrder);
    void ParseGWData(const string& strData) override;
    void SetReplyData() override;
};

#endif