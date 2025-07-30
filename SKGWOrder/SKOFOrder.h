#ifndef __SKOFORDER_H__
#define __SKOFORDER_H__

#include "SKBaseGWOrder.h"

class CSKOFOrder : public CSKBaseGWOrder
{
public:
    CSKOFOrder(const string& strServer);
    virtual ~CSKOFOrder();
    void Run(const TSKOFOrder& tSKOrder, const struct timeval& tvStart, const string& strThreadID);
    int CheckOverTimeToReply(const TSKOFOrder& tSKOrder, const struct timeval& tvStart);
    void SetLogger();
private:
    bool SetPostData(const TSKOFOrder& tSKOrder);
    void ParseGWData(const string& strData) override;
    void SetReplyData() override;
};

#endif