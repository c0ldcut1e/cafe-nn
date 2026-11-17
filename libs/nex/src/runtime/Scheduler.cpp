#include <algorithm>
#include <nn/nex/platform/Time.h>
#include <nn/nex/runtime/Job.h>
#include <nn/nex/runtime/Scheduler.h>
#include <nn/nex/types.h>

namespace nn::nex
{

    Scheduler *Scheduler::GetInstance()
    {
        static Scheduler instance;
        return &instance;
    }

    void Scheduler::Queue(Job *job, bool front)
    {
        if (std::find(m_ready.begin(), m_ready.end(), job) != m_ready.end())
            return;
        job->m_state = Job::Ready;
        if (front)
            m_ready.push_front(job);
        else
            m_ready.push_back(job);
    }

    void Scheduler::Remove(Job *job)
    {
        m_ready.erase(std::remove(m_ready.begin(), m_ready.end(), job), m_ready.end());
    }

    void Scheduler::RegisterTask(SchedulerTask *task)
    {
        if (std::find(m_tasks.begin(), m_tasks.end(), task) == m_tasks.end())
            m_tasks.push_back(task);
        m_tasksChanged = true;
    }

    void Scheduler::UnregisterTask(SchedulerTask *task)
    {
        m_tasks.erase(std::remove(m_tasks.begin(), m_tasks.end(), task), m_tasks.end());
        m_tasksChanged = true;
    }

    qUnsignedInt32 Scheduler::RunOnce(qUnsignedInt32 maxJobs)
    {
        TimeValue now = Time::GetTime();
        // A task may unregister itself or another task while polling, so we walk a copy and recheck membership
        std::vector<SchedulerTask *> tasks = m_tasks;
        m_tasksChanged                     = false;
        for (SchedulerTask *task : tasks)
        {
            if (m_tasksChanged && std::find(m_tasks.begin(), m_tasks.end(), task) == m_tasks.end())
                continue;
            task->Poll(now);
        }

        qUnsignedInt32 ran = 0;
        while (!m_ready.empty() && ran < maxJobs)
        {
            Job *job = m_ready.front();
            m_ready.pop_front();
            job->m_state    = Job::Running;
            Job::State next = job->Execute();
            ran++;
            if (next == Job::Complete)
            {
                job->m_state = Job::Complete;
                if (job->m_deleteWhenComplete)
                    delete job;
            }
            else if (next == Job::Ready)
            {
                job->m_state = Job::Running;
                Queue(job);
            }
            else
            {
                job->m_state = next;
            }
        }
        return ran;
    }

    void Scheduler::Dispatch(qUnsignedInt32 timeout, qUnsignedInt32 maxJobs)
    {
        TimeValue deadline = Time::ConvertTimeoutToDeadline(timeout);
        qUnsignedInt32 ran = 0;
        do
        {
            ran += RunOnce(maxJobs - ran);
        } while (ran < maxJobs && Time::GetTime() < deadline);
    }

} // namespace nn::nex
