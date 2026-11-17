#include <nn/pia/common/Time.h>
#include <pead/peadTypes.h>

namespace nn::pia::common
{
    s64 TimeSpan::GetTicksPerMSec()
    {
        return pead::TickSpan::getFrequency() / 1000;
    }

    s64 TimeSpan::GetTicksPerSec()
    {
        return pead::TickSpan::getFrequency();
    }

    void Time::SetNow()
    {
        m_ticks = (u64) pead::TickTime::now();
    }

    Time Time::GetNow()
    {
        Time time;
        time.SetNow();
        return time;
    }
} // namespace nn::pia::common
