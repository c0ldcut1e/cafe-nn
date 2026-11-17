/**
 * @file
 * @brief A StepSequenceJob runs a chain of member function steps. Each step picks the next one with SetStep and returns Continue to run it
 * straight away, or another ExecuteResult to give the scheduler back control.
 */

#pragma once

#include <nn/pia/common/Job.h>

namespace nn::pia::common
{
    class StepSequenceJob : public Job
    {
    public:
        using Step = ExecuteResult (StepSequenceJob::*)();

        void Reset() override;

        /**
         * @brief Asks the job to stop. We call CancelCleanup on its next run instead of the current step.
         */
        void Cancel()
        {
            m_cancelRequested = true;
        }
        bool IsCancelRequested() const
        {
            return m_cancelRequested;
        }

        /**
         * @brief Sleeps until the job completes. Only use this for a background job, since a foreground one needs this thread to Dispatch.
         *
         * @return false when timeoutMs passed first.
         */
        bool WaitForCompletion(u32 timeoutMs);

    protected:
        template<typename T>
        void SetStep(ExecuteResult (T::*step)())
        {
            m_step = static_cast<Step>(step);
        }

        virtual void CancelCleanup()
        {}

        ExecuteResult ExecuteCore() override;

    private:
        Step m_step            = nullptr;
        bool m_cancelRequested = false;
    };
} // namespace nn::pia::common
