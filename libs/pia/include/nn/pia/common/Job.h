/**
 * @file
 * @brief A Job is one piece of pia's cooperative work. The Scheduler runs it from Dispatch, and what ExecuteCore returns decides whether it
 * runs again, waits, suspends or completes.
 */

#pragma once

#include <nn/pia/Result.h>
#include <nn/pia/common/Time.h>

namespace nn::pia::common
{
    class Job;

    namespace detail
    {
        // This is an intrusive list of jobs. A job knows which list holds it, so it can unlink itself from either scheduler.
        class JobList
        {
        public:
            void InsertByTime(Job *job);
            void PushBack(Job *job);
            void Remove(Job *job);
            Job *PopFront();
            void MoveAllInto(JobList &other);

            // The caller has to hold the Scheduler lock. We run every job that is due at the time we store in dispatchTime.
            void RunDue(u32 timeoutMs, Time &dispatchTime);

            Job *GetFront() const
            {
                return m_head;
            }
            bool IsEmpty() const
            {
                return m_head == nullptr;
            }

        private:
            Job *m_head = nullptr;
            Job *m_tail = nullptr;
        };
    } // namespace detail

    class Job
    {
    public:
        /**
         * @brief ExecuteCore returns one of these. The top byte is the action and the low 16 bits are a wait in milliseconds.
         */
        using ExecuteResult = u32;

        enum State
        {
            State_None,
            State_Waiting,
            State_Suspended,
            State_Ready,
            State_Running,
            State_Completed,
        };

        static constexpr ExecuteResult Continue()
        {
            return 0x0000000;
        }
        static constexpr ExecuteResult Completed()
        {
            return 0x2000000;
        }
        static constexpr ExecuteResult Suspend()
        {
            return 0x3000000;
        }
        static constexpr ExecuteResult Wait(u16 ms)
        {
            return 0x4000000 | ms;
        }
        static constexpr ExecuteResult WaitNextDispatch()
        {
            return 0x5000000;
        }
        static constexpr ExecuteResult MoveToForeground()
        {
            return 0x6000000;
        }
        static constexpr ExecuteResult MoveToBackground()
        {
            return 0x7000000;
        }

        Job()                       = default;
        Job(const Job &)            = delete;
        Job &operator=(const Job &) = delete;

        /**
         * @brief The job has to be idle when it is destroyed, so call Reset first if it may still be scheduled.
         */
        virtual ~Job() = default;

        /**
         * @brief Schedules an idle job to run on the next Dispatch.
         *
         * @param background True runs it on the BackgroundScheduler instead.
         * @return InvalidState unless the job is idle and the Scheduler exists.
         */
        Result Ready(bool background = false);

        /**
         * @brief Schedules a suspended job again.
         *
         * @return InvalidState unless the job is suspended.
         */
        Result Resume(bool background = false);

        /**
         * @brief Takes the job off whichever scheduler holds it and makes it idle. Don't call this from the job's own ExecuteCore.
         */
        virtual void Reset();

        State GetState() const;
        u32 GetExecuteCount() const
        {
            return m_executeCount;
        }
        const Time &GetTime() const
        {
            return m_time;
        }
        bool IsBackground() const
        {
            return m_internalState >= InternalState_BackgroundWaiting && m_internalState <= InternalState_BackgroundRunning;
        }

    protected:
        virtual ExecuteResult ExecuteCore() = 0;

    private:
        friend class Scheduler;
        friend class BackgroundScheduler;
        friend class detail::JobList;

        enum InternalState : u8
        {
            InternalState_None,
            InternalState_Waiting,
            InternalState_Ready,
            InternalState_Running,
            InternalState_BackgroundWaiting,
            InternalState_BackgroundReady,
            InternalState_BackgroundRunning,
            InternalState_Suspended,
            InternalState_Completed,
        };

        // The caller has to hold the Scheduler lock. We release it around ExecuteCore so the job can schedule other jobs.
        void Execute(Time dispatchTime);
        void Entry(bool background, bool next);

        InternalState m_internalState = InternalState_None;
        u32 m_executeCount            = 0;
        Time m_time;
        Job *m_prev              = nullptr;
        Job *m_next              = nullptr;
        detail::JobList *m_owner = nullptr;
    };
} // namespace nn::pia::common
