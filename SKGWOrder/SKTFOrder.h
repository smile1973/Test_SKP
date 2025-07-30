#ifndef __SKTFORDER_H__
#define __SKTFORDER_H__

#include "SKBaseGWOrder.h"

class CSKTFOrder : public CSKBaseGWOrder
{
public:
    CSKTFOrder(const string& strServer);
    virtual ~CSKTFOrder();
    void Run(const TSKTFOrder& tSKOrder, const struct timeval& tvStart, const string& strThreadID);
    int CheckOverTimeToReply(const TSKTFOrder& tSKOrder, const struct timeval& tvStart);
    void SetLogger();
private:
    bool SetPostData(const TSKTFOrder& tSKOrder);
    void ParseGWData(const string& strData) override;
    void SetReplyData() override;
};

#endif