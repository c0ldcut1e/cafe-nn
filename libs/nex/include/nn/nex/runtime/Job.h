/**
 * @file
 * @brief This is one unit of work on the scheduler, which runs Execute until it reports completion.
 */

#pragma once

#include <deque>
#include <nn/nex/types.h>
#include <vector>

namespace nn::nex
{

    class Scheduler;

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

} // namespace nn::nex
