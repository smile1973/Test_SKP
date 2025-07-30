#ifndef __SKTSALTER_H__
#define __SKTSALTER_H__

#include "SKBaseGWOrder.h"

class CSKTSAlter : public CSKBaseGWOrder
{
public:
    CSKTSAlter(const string& strServer);
    virtual ~CSKTSAlter();
    void Run(const TSKTSOrder& tSKOrder, const struct timeval& tvStart, const string& strThreadID);
    int CheckOverTimeToReply(const TSKTSOrder& tSKOrder, const struct timeval& tvStart);
    void SetLogger();
private:
    bool SetPostData(const TSKTSOrder& tSKOrder);
    void ParseGWData(const string& strData) override;
    void SetReplyData() override;
};

#endif