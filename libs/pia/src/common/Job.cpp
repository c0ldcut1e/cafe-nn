#include <nn/pia/common/Scheduler.h>

namespace nn::pia::common
{
    namespace detail
    {
        void JobList::InsertByTime(Job *job)
        {
            // Jobs with the same time keep the order they were added in, so we walk back past every job that is not later.
            Job *after = m_tail;
            while (after && after->m_time > job->m_time) after = after->m_prev;
            job->m_prev  = after;
            job->m_next  = after ? after->m_next : m_head;
            job->m_owner = this;
            if (job->m_next)
                job->m_next->m_prev = job;
            else
                m_tail = job;
            if (after)
                after->m_next = job;
            else
                m_head = job;
        }

        void JobList::PushBack(Job *job)
        {
            job->m_prev  = m_tail;
            job->m_next  = nullptr;
            job->m_owner = this;
            if (m_tail)
                m_tail->m_next = job;
            else
                m_head = job;
            m_tail = job;
        }

        void JobList::Remove(Job *job)
        {
            if (job->m_owner != this)
                return;
            if (job->m_prev)
                job->m_prev->m_next = job->m_next;
            else
                m_head = job->m_next;
            if (job->m_next)
                job->m_next->m_prev = job->m_prev;
            else
                m_tail = job->m_prev;
            job->m_prev  = nullptr;
            job->m_next  = nullptr;
            job->m_owner = nullptr;
        }

        Job *JobList::PopFront()
        {
            Job *job = m_head;
            if (job)
                Remove(job);
            return job;
        }

        void JobList::MoveAllInto(JobList &other)
        {
            while (Job *job = PopFront()) other.InsertByTime(job);
        }

        void JobList::RunDue(u32 timeoutMs, Time &dispatchTime)
        {
            dispatchTime  = Time::GetNow();
            Time deadline = dispatchTime + TimeSpan::FromMilliSeconds(timeoutMs);
            while (Job *job = m_head)
            {
                if (job->m_time > dispatchTime)
                    break;
                if (timeoutMs > 0 && Time::GetNow() > deadline)
                    break;
                Remove(job);
                job->Execute(dispatchTime);
            }
        }
    } // namespace detail

    Job::State Job::GetState() const
    {
        switch (m_internalState)
        {
            case InternalState_Waiting:
            case InternalState_BackgroundWaiting:
                return State_Waiting;
            case InternalState_Ready:
            case InternalState_BackgroundReady:
                return State_Ready;
            case InternalState_Running:
            case InternalState_BackgroundRunning:
                return State_Running;
            case InternalState_Suspended:
                return State_Suspended;
            case InternalState_Completed:
                return State_Completed;
            default:
                return State_None;
        }
    }

    void Job::Entry(bool background, bool next)
    {
        Scheduler *scheduler = Scheduler::GetInstance();
        if (background && BackgroundScheduler::GetInstance())
            BackgroundScheduler::GetInstance()->EntryJob(this);
        else if (next)
            scheduler->EntryJobNext(this);
        else
            scheduler->EntryJob(this);
    }

    Result Job::Ready(bool background)
    {
        Scheduler *scheduler = Scheduler::GetInstance();
        if (!scheduler)
            return ResultInvalidState();
        scheduler->Lock();
        if (m_internalState != InternalState_None)
        {
            scheduler->Unlock();
            return ResultInvalidState();
        }
        m_internalState = background ? InternalState_BackgroundReady : InternalState_Ready;
        m_time.SetNow();
        Entry(background, false);
        scheduler->Unlock();
        return ResultSuccess();
    }

    Result Job::Resume(bool background)
    {
        Scheduler *scheduler = Scheduler::GetInstance();
        if (!scheduler)
            return ResultInvalidState();
        scheduler->Lock();
        if (m_internalState != InternalState_Suspended)
        {
            scheduler->Unlock();
            return ResultInvalidState();
        }
        m_internalState = background ? InternalState_BackgroundReady : InternalState_Ready;
        m_time.SetNow();
        Entry(background, false);
        scheduler->Unlock();
        return ResultSuccess();
    }

    void Job::Reset()
    {
        Scheduler *scheduler = Scheduler::GetInstance();
        if (scheduler)
            scheduler->Lock();
        if (m_owner)
            m_owner->Remove(this);
        m_internalState = InternalState_None;
        m_executeCount  = 0;
        m_time          = Time();
        if (scheduler)
            scheduler->Unlock();
    }

    void Job::Execute(Time dispatchTime)
    {
        Scheduler *scheduler = Scheduler::GetInstance();
        bool background      = IsBackground();
        m_internalState      = background ? InternalState_BackgroundRunning : InternalState_Running;

        scheduler->Unlock();
        ExecuteResult result = ExecuteCore();
        scheduler->Lock();
        m_executeCount++;

        // A job that runs again at once must land after this Dispatch's time, or the same Dispatch would pick it up forever.
        Time now = Time::GetNow();
        if (now <= dispatchTime)
            now = Time(dispatchTime.GetTicks() + 1);

        InternalState waiting = background ? InternalState_BackgroundWaiting : InternalState_Waiting;
        InternalState ready   = background ? InternalState_BackgroundReady : InternalState_Ready;
        switch ((result >> 24) & 0xff)
        {
            case 1:
            case 2:
                m_internalState = InternalState_Completed;
                break;
            case 3:
                m_internalState = InternalState_Suspended;
                break;
            case 4:
                m_internalState = waiting;
                m_time          = now + TimeSpan::FromMilliSeconds(result & 0xffff);
                Entry(background, false);
                break;
            case 5:
                m_internalState = waiting;
                m_time          = now;
                Entry(background, true);
                break;
            case 6:
                m_internalState = InternalState_Ready;
                m_time          = now;
                Entry(false, false);
                break;
            case 7:
                m_internalState = InternalState_BackgroundReady;
                m_time          = now;
                Entry(true, false);
                break;
            default:
                m_internalState = ready;
                m_time          = now;
                Entry(background, false);
                break;
        }
    }
} // namespace nn::pia::common
