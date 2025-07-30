#ifndef __SKHEARTBEAT_H__
#define __SKHEARTBEAT_H__

#ifdef _WIN32
#include <Windows.h>
#endif

#include "pevents.h"
#include "SKThread.h"
#include "SKInterface/ISKHeartbeatCallback.h"

using namespace neosmart;

class CSKHeartbeat: public CSKThread
{
public:
    CSKHeartbeat(ISKHeartbeatCallback* pHeartbeatCallback);
    virtual ~CSKHeartbeat();

    void SetTimeInterval(int nTimeInterval);

    void TriggerGetReplyEvent();
    void TriggerTerminateEvent();

protected:
    void Run();

private:
    int m_nTimeInterval;
    int m_nIdleTime;

    ISKHeartbeatCallback* m_pHeartbeatCallback;
    neosmart_event_t m_PEvent[2];
};
#endif
