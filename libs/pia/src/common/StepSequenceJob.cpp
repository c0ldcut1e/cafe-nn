#include <nn/pia/common/StepSequenceJob.h>
#include <pead/peadThread.h>

namespace nn::pia::common
{
    void StepSequenceJob::Reset()
    {
        Job::Reset();
        m_cancelRequested = false;
    }

    Job::ExecuteResult StepSequenceJob::ExecuteCore()
    {
        for (;;)
        {
            if (m_cancelRequested)
            {
                CancelCleanup();
                m_cancelRequested = false;
                return Completed();
            }
            if (!m_step)
                return Completed();
            ExecuteResult result = (this->*m_step)();
            if ((result >> 24) != 0)
                return result;
        }
    }

    bool StepSequenceJob::WaitForCompletion(u32 timeoutMs)
    {
        Time deadline = Time::GetNow() + TimeSpan::FromMilliSeconds(timeoutMs);
        while (GetState() != State_Completed && GetState() != State_None)
        {
            if (Time::GetNow() > deadline)
                return false;
            pead::Thread::sleep(pead::TickSpan::fromMilliSeconds(1));
        }
        return true;
    }
} // namespace nn::pia::common
