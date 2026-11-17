#include <chrono>
#include <nn/nex/platform/Time.h>

#ifdef __WIIU__
#include <coreinit/time.h>
#endif

namespace nn::nex
{

    TimeValue Time::GetTime()
    {
#ifdef __WIIU__
        return (TimeValue) OSTicksToMilliseconds(OSGetSystemTime());
#else
        return (TimeValue) std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
#endif
    }

} // namespace nn::nex
