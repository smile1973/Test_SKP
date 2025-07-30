#ifndef __SKTHREAD_H__
#define __SKTHREAD_H__

#include <thread>
#include <atomic>

class CSKThread 
{
public:
    CSKThread();
    virtual ~CSKThread();

    void Start();
    void Interrupt();
    bool IsInterrupted();
    bool Join();
    bool Detach();
    virtual void Run();
    std::thread::id GetThreadID();

private:
    std::atomic<bool> m_IsInterrupted;
    std::thread m_Thread;

};
#endif
