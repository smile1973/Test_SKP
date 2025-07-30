#include "SKThread.h"
#include <functional>

CSKThread::CSKThread(): m_IsInterrupted(false)
{
}

CSKThread::~CSKThread() 
{
    if (!IsInterrupted()) 
    {
        Interrupt();
    }

    if (m_Thread.joinable())
    {
        m_Thread.join();
    }
}

void CSKThread::Start()
{
    std::thread Thread(std::bind(&CSKThread::Run, this));
    m_Thread = std::move(Thread);
}

void CSKThread::Interrupt()
{
    m_IsInterrupted = true;
}

bool CSKThread::IsInterrupted()
{
    return m_IsInterrupted;
}

bool CSKThread::Join()
{
    if (!m_Thread.joinable())
        return false;
    else
        m_Thread.join();

    return true;
}

bool CSKThread::Detach()
{
    if (!m_Thread.joinable())
        return false;
    else
        m_Thread.detach();

    return true;
}

std::thread::id CSKThread::GetThreadID()
{
    return m_Thread.get_id();
}

void CSKThread::Run()
{

}