#ifndef __SKINTERFACE_ISKSERVERSOCKETCALLBACK_H__
#define __SKINTERFACE_ISKSERVERSOCKETCALLBACK_H__

class ISKServerSocketCallback 
{
public:
    virtual void OnListening()=0;
    virtual void OnShutdown()=0;

    virtual ~ISKServerSocketCallback(){};
};
#endif 
