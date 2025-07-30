#ifndef __SKPRETFALTER_H__
#define __SKPRETFALTER_H__

#include "SKBaseGWOrder.h"

class CSKPreTFAlter : public CSKBaseGWOrder
{
public:
    CSKPreTFAlter(const string& strServer);
    virtual ~CSKPreTFAlter();
    void Run(const TSKTFOrder& tSKOrder, const struct timeval& tvStart, const string& strThreadID);
    int CheckOverTimeToReply(const TSKTFOrder& tSKOrder, const struct timeval& tvStart);
    void SetLogger();
private:
    bool SetPostData(const TSKTFOrder& tSKOrder);
    void ParseGWData(const string& strData) override;
    void SetReplyData() override;
};

#endif