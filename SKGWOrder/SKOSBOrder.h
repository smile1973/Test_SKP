#ifndef __SKOSBORDER_H__
#define __SKOSBORDER_H__

#include "SKBaseGWOrder.h"

class CSKOSBOrder : public CSKBaseGWOrder
{
public:
    CSKOSBOrder(const string& strServer);
    virtual ~CSKOSBOrder();
    void Run(const TSKOSOrder& tSKOrder, const struct timeval& tvStart, const string& strThreadID);
    int CheckOverTimeToReply(const TSKOSOrder& tSKOrder, const struct timeval& tvStart);
    void SetLogger();
private:
    bool SetPostData(const TSKOSOrder& tSKOrder);
    void ParseGWData(const string& strData) override;
    void SetReplyData() override;
};

#endif