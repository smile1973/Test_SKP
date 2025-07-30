#ifndef __SKTSORDER_H__
#define __SKTSORDER_H__

#include "SKBaseGWOrder.h"

class CSKTSOrder : public CSKBaseGWOrder
{
public:
    CSKTSOrder(const string& strServer);
    virtual ~CSKTSOrder();
    void Run(const TSKTSOrder& tSKOrder, const struct timeval& tvStart, const string& strThreadID);
    int CheckOverTimeToReply(const TSKTSOrder& tSKOrder, const struct timeval& tvStart);
    void SetLogger();
private:
    bool SetPostData(const TSKTSOrder& tSKOrder);
    void ParseGWData(const string& strData) override;
    void SetReplyData() override;
    string strAction;
    string strBlockNum;
};

#endif