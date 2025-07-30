#include "SKHeartbeat.h"
#include <cstdio>
#include <cstring>
#include <cerrno> 
#include <sys/socket.h>
#include <unistd.h>
//#include <assert.h>

using namespace std;

CSKHeartbeat::CSKHeartbeat(ISKHeartbeatCallback* pHeartbeatCallback)
{
    m_nTimeInterval = 0;
    m_nIdleTime = 0;

    m_pHeartbeatCallback = pHeartbeatCallback;

    m_PEvent[0] = CreateEvent();
    m_PEvent[1] = CreateEvent();
}

CSKHeartbeat::~CSKHeartbeat()
{
    DestroyEvent(m_PEvent[0]);
    DestroyEvent(m_PEvent[1]);
}

void CSKHeartbeat::Run()
{
    while(m_pHeartbeatCallback)
    {
        int nIndex = -1;
        int nResult = WaitForMultipleEvents(m_PEvent, 2, false, 10000, nIndex);

        if(nResult == WAIT_TIMEOUT)
        {
            m_nIdleTime += 10;

            if(m_nIdleTime < m_nTimeInterval)
            {
                continue;
            }
            else if(m_nIdleTime == m_nTimeInterval || m_nIdleTime == m_nTimeInterval + 10)
            {
                if(m_pHeartbeatCallback)
                    m_pHeartbeatCallback->OnHeartbeatRequest();
            }
            else if(m_nIdleTime == m_nTimeInterval + 20)
            {
                if(m_pHeartbeatCallback)
                    m_pHeartbeatCallback->OnHeartbeatLost();//end the session
                break;
            }
            else
            {
                if(m_pHeartbeatCallback)
                    m_pHeartbeatCallback->OnHeartbeatError(m_nIdleTime, "HEARTBEAT_IDLETIME_ERROR");
            }
        }
        else if(nResult != 0)
        {
            if(m_pHeartbeatCallback)
                m_pHeartbeatCallback->OnHeartbeatError(0, "HEARTBEAT_ERROR_IN_WAIT");
        }
        else if(nIndex == 0)//get client reply
        {
            m_nIdleTime = 0;
        }
        else if(nIndex == 1)//terminate heartbeat
        {
            if(m_pHeartbeatCallback)
                m_pHeartbeatCallback->OnHeartbeatError(0, "HEARTBEAT_TERMINATE");
            break;
        }
        else
        {
            if(m_pHeartbeatCallback)
                m_pHeartbeatCallback->OnHeartbeatError(nIndex, "HEARTBEAT_UNEXPECTED_INDEX_ERROR");
        }
    }
    return;
}

void CSKHeartbeat::SetTimeInterval(int nTimeInterval)
{
    m_nTimeInterval = nTimeInterval;
}

void CSKHeartbeat::TriggerGetReplyEvent()
{
    SetEvent(m_PEvent[0]);
}

void CSKHeartbeat::TriggerTerminateEvent()
{
    SetEvent(m_PEvent[1]);
}
