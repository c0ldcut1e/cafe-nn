#include <new>
#include <nn/pia/common/Scheduler.h>

namespace nn::pia::common
{
    Scheduler *Scheduler::s_instance;
    BackgroundScheduler *BackgroundScheduler::s_instance;

    Result Scheduler::CreateInstance()
    {
        if (s_instance)
            return ResultInvalidState();
        s_instance = new (std::nothrow) Scheduler();
        return s_instance ? ResultSuccess() : ResultInvalidState();
    }

    void Scheduler::DestroyInstance()
    {
        delete s_instance;
        s_instance = nullptr;
    }

    void Scheduler::Dispatch(u32 timeoutMs)
    {
        Lock();
        m_dispatchCount++;
        m_jobs.RunDue(timeoutMs, m_dispatchTime);
        m_nextJobs.MoveAllInto(m_jobs);
        Unlock();
    }

    void Scheduler::EntryJob(Job *job)
    {
        Lock();
        if (job->IsBackground() && BackgroundScheduler::GetInstance())
            BackgroundScheduler::GetInstance()->EntryJob(job);
        else
            m_jobs.InsertByTime(job);
        Unlock();
    }

    void Scheduler::EntryJobNext(Job *job)
    {
        Lock();
        m_nextJobs.PushBack(job);
        Unlock();
    }

    void Scheduler::CancelJob(Job *job)
    {
        Lock();
        m_jobs.Remove(job);
        m_nextJobs.Remove(job);
        Unlock();
    }

    Result BackgroundScheduler::CreateInstance()
    {
        if (s_instance || !Scheduler::GetInstance())
            return ResultInvalidState();
        s_instance = new (std::nothrow) BackgroundScheduler();
        return s_instance ? ResultSuccess() : ResultInvalidState();
    }

    void BackgroundScheduler::DestroyInstance()
    {
        delete s_instance;
        s_instance = nullptr;
    }

    void BackgroundScheduler::Dispatch(u32 timeoutMs)
    {
        Scheduler *scheduler = Scheduler::GetInstance();
        scheduler->Lock();
        Time dispatchTime;
        m_jobs.RunDue(timeoutMs, dispatchTime);
        scheduler->Unlock();
    }

    void BackgroundScheduler::EntryJob(Job *job)
    {
        Scheduler *scheduler = Scheduler::GetInstance();
        scheduler->Lock();
        m_jobs.InsertByTime(job);
        scheduler->Unlock();
    }

    void BackgroundScheduler::CancelJob(Job *job)
    {
        Scheduler *scheduler = Scheduler::GetInstance();
        scheduler->Lock();
        m_jobs.Remove(job);
        scheduler->Unlock();
    }
} // namespace nn::pia::common
