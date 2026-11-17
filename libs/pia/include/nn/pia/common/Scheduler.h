/**
 * @file
 * @brief The Scheduler runs pia's foreground jobs. The game has to keep calling Dispatch from its loop, since pia has no thread of its
 * own for them.
 */

#pragma once

#include <nn/pia/common/Job.h>
#include <pead/peadCriticalSection.h>

namespace nn::pia::common
{
    class Scheduler
    {
    public:
        static Result CreateInstance();
        static void DestroyInstance();
        static Scheduler *GetInstance()
        {
            return s_instance;
        }

        /**
         * @brief Runs every job that is due.
         *
         * @param timeoutMs When it is above zero we stop starting new jobs once this many milliseconds have passed.
         */
        void Dispatch(u32 timeoutMs = 0);

        void EntryJob(Job *job);
        void EntryJobNext(Job *job);
        void CancelJob(Job *job);

        u32 GetDispatchCount() const
        {
            return m_dispatchCount;
        }
        const Time &GetDispatchTime() const
        {
            return m_dispatchTime;
        }

        void Lock()
        {
            m_lock.lock();
        }
        void Unlock()
        {
            m_lock.unlock();
        }

    private:
        Scheduler()  = default;
        ~Scheduler() = default;

        static Scheduler *s_instance;

        pead::detail::RecursiveLock m_lock;
        detail::JobList m_jobs;
        detail::JobList m_nextJobs;
        Time m_dispatchTime;
        u32 m_dispatchCount = 0;
    };

    /**
     * @brief This runs background jobs. Whoever creates it has to call Dispatch from a thread of their own, and it shares the Scheduler lock.
     */
    class BackgroundScheduler
    {
    public:
        static Result CreateInstance();
        static void DestroyInstance();
        static BackgroundScheduler *GetInstance()
        {
            return s_instance;
        }

        void Dispatch(u32 timeoutMs = 0);
        void EntryJob(Job *job);
        void CancelJob(Job *job);

    private:
        BackgroundScheduler()  = default;
        ~BackgroundScheduler() = default;

        static BackgroundScheduler *s_instance;

        detail::JobList m_jobs;
    };
} // namespace nn::pia::common
