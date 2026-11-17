/**
 * @file
 * @brief NEX runs everything as jobs on one scheduler, and nothing makes progress unless the game keeps calling Scheduler::Dispatch.
 */

#pragma once

#include <deque>
#include <nn/nex/Platform.h>
#include <vector>

namespace nn::nex
{

    /**
     * @brief This is one unit of work on the scheduler, which runs Execute until it reports completion.
     */
    class Job
    {
    public:
        enum JobType
        {
            UserJob   = 0,
            SystemJob = 1
        };
        enum State
        {
            Initial,
            Ready,
            Running,
            Waiting,
            Complete
        };

        explicit Job(JobType type = UserJob) : m_type(type)
        {}
        virtual ~Job() = default;

        void SetToReady()
        {
            m_state = Ready;
        }

        /**
         * @brief A job that returns Waiting from Execute stays parked until something calls Resume.
         */
        void Resume();
        bool WaitForCompletion(qUnsignedInt32 timeout);
        State GetState() const
        {
            return m_state;
        }
        JobType GetType() const
        {
            return m_type;
        }

    protected:
        virtual State Execute() = 0;

        /**
         * @brief A job can't delete itself inside Execute because the scheduler still touches it afterwards, so it asks us to delete it instead.
         */
        void DeleteWhenComplete()
        {
            m_deleteWhenComplete = true;
        }

    private:
        friend class Scheduler;
        JobType m_type;
        State m_state             = Initial;
        bool m_deleteWhenComplete = false;
    };

    /**
     * @brief Anything that must make progress on its own, such as a socket waiting for datagrams, registers one of these.
     */
    class SchedulerTask
    {
    public:
        virtual ~SchedulerTask()         = default;
        virtual void Poll(TimeValue now) = 0;
    };

    /**
     * @brief NEX does all of its work on whichever thread calls Dispatch, so the game must keep calling it.
     */
    class Scheduler
    {
    public:
        static Scheduler *GetInstance();

        /**
         * @brief We run until the timeout expires or maxJobs jobs have run, whichever happens first.
         *
         * @param timeout Milliseconds to keep running, where 0 runs one pass.
         * @param maxJobs The most jobs to run.
         */
        void Dispatch(qUnsignedInt32 timeout = 0, qUnsignedInt32 maxJobs = 0xFFFFFFFF);
        void Queue(Job *job, bool front = false);
        void Remove(Job *job);

        void RegisterTask(SchedulerTask *task);
        void UnregisterTask(SchedulerTask *task);

    private:
        Scheduler() = default;
        qUnsignedInt32 RunOnce(qUnsignedInt32 maxJobs);

        std::deque<Job *> m_ready;
        std::vector<SchedulerTask *> m_tasks;
        bool m_tasksChanged = false;
    };

} // namespace nn::nex
