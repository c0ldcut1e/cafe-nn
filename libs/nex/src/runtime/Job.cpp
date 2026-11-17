#include <algorithm>
#include <nn/nex/platform/Time.h>
#include <nn/nex/runtime/Job.h>
#include <nn/nex/runtime/Scheduler.h>
#include <nn/nex/types.h>

namespace nn::nex
{

    void Job::Resume()
    {
        if (m_state == Waiting)
        {
            m_state = Ready;
            Scheduler::GetInstance()->Queue(this);
        }
    }

    bool Job::WaitForCompletion(qUnsignedInt32 timeout)
    {
        TimeValue deadline = Time::ConvertTimeoutToDeadline(timeout);
        while (m_state != Complete)
        {
            if (Time::GetTime() >= deadline)
                return false;
            Scheduler::GetInstance()->Dispatch(1);
        }
        return true;
    }

} // namespace nn::nex
