/**
 * @file
 * @brief NEX does all of its work on whichever thread calls Dispatch, so the game must keep calling it.
 */

#pragma once

#include <deque>
#include <nn/nex/platform/Time.h>
#include <nn/nex/runtime/Job.h>
#include <nn/nex/types.h>
#include <vector>

namespace nn::nex
{

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
