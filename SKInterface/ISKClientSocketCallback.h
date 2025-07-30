#ifndef __SKINTERFACE_ISKCLIENTSOCKETCALLBACK_H__
#define __SKINTERFACE_ISKCLIENTSOCKETCALLBACK_H__

class ISKClientSocketCallback 
{
public:
    virtual void OnConnect()=0;
    virtual void OnDisconnect()=0;
    virtual void OnData( unsigned char* pBuf, int nSize)=0;

    virtual ~ISKClientSocketCallback(){};
};
#endif
