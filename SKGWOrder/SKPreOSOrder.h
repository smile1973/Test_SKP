#ifndef __SKPREOSORDER_H__
#define __SKPREOSORDER_H__

#include "SKBaseGWOrder.h"

class CSKPreOSOrder : public CSKBaseGWOrder
{
public:
    CSKPreOSOrder(const string& strServer);
    virtual ~CSKPreOSOrder();
    void Run(const TSKOSOrder& tSKOrder, const struct timeval& tvStart, const string& strThreadID);
    int CheckOverTimeToReply(const TSKOSOrder& tSKOrder, const struct timeval& tvStart);
    void SetLogger();
private:
    bool SetPostData(const TSKOSOrder& tSKOrder);
    void ParseGWData(const string& strData) override;
    void SetReplyData() override;
};

#endif