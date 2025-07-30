#ifndef __SKTFALTER_H__
#define __SKTFALTER_H__

#include "SKBaseGWOrder.h"

class CSKTFAlter : public CSKBaseGWOrder
{
public:
    CSKTFAlter(const string& strServer);
    virtual ~CSKTFAlter();
    void Run(const TSKTFOrder& tSKOrder, const struct timeval& tvStart, const string& strThreadID);
    int CheckOverTimeToReply(const TSKTFOrder& tSKOrder, const struct timeval& tvStart);
    void SetLogger();
private:
    bool SetPostData(const TSKTFOrder& tSKOrder);
    void ParseGWData(const string& strData) override;
    void SetReplyData() override;
};

#endif