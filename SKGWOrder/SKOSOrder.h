#ifndef __SKOSORDER_H__
#define __SKOSORDER_H__

#include "SKBaseGWOrder.h"

class CSKOSOrder : public CSKBaseGWOrder
{
public:
    CSKOSOrder(const string& strServer);
    virtual ~CSKOSOrder();
    void Run(const TSKOSOrder& tSKOrder, const struct timeval& tvStart, const string& strThreadID);
    int CheckOverTimeToReply(const TSKOSOrder& tSKOrder, const struct timeval& tvStart);
    void SetLogger();
private:
    bool SetPostData(const TSKOSOrder& tSKOrder);
    void ParseGWData(const string& strData) override;
    void SetReplyData() override;
};

#endif