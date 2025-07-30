#ifndef __SKINTERFACE_ISKHEARTBEATCALLBACK_H__
#define __SKINTERFACE_ISKHEARTBEATCALLBACK_H__

class ISKHeartbeatCallback 
{
public:
    virtual void OnHeartbeatLost()=0;
    virtual void OnHeartbeatRequest()=0;

    virtual void OnHeartbeatError(int nData, const char* pErrorMessage)=0;

    virtual ~ISKHeartbeatCallback(){};
};
#endif
