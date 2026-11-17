#include <pead/peadTypes.h>

#ifdef __WIIU__
#include <coreinit/core.h>
#include <coreinit/time.h>
#else
#include <chrono>
#include <thread>
#endif

namespace pead
{

    namespace
    {

        // This maps each CoreId to the hardware core it runs on. Nintendo keeps the main core on core 1 because the system runs on core 0.
        constexpr u32 cPlatformCore[CoreId::cUndef] = {1, 0, 2};

    } // namespace

    u32 CoreInfo::s_numCores      = CoreId::cUndef;
    CoreId CoreInfo::s_mainCoreId = CoreId::cMain;

    s64 TickSpan::getFrequency()
    {
#ifdef __WIIU__
        return (s64) OSTimerClockSpeed;
#else
        return 1000000000;
#endif
    }

    TickSpan TickSpan::fromNanoSeconds(s64 ns)
    {
#ifdef __WIIU__
        return TickSpan((s64) OSNanosecondsToTicks(ns));
#else
        return TickSpan(ns);
#endif
    }

    s64 TickSpan::toNanoSeconds() const
    {
#ifdef __WIIU__
        return (s64) OSTicksToNanoseconds(m_span);
#else
        return m_span;
#endif
    }

    s64 TickTime::now()
    {
#ifdef __WIIU__
        return (s64) OSGetSystemTime();
#else
        return (s64) std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
#endif
    }

    void CoreInfo::configure()
    {
#ifdef __WIIU__
        s_numCores = CoreId::cUndef;
#else
        u32 hardware = (u32) std::thread::hardware_concurrency();
        s_numCores   = hardware == 0 || hardware > CoreId::cUndef ? (u32) CoreId::cUndef : hardware;
#endif
        s_mainCoreId = CoreId::cMain;
    }

    CoreId CoreInfo::getCurrentCoreId()
    {
#ifdef __WIIU__
        u32 core = OSGetCoreId();
        for (s32 i = 0; i < CoreId::cUndef; i++)
        {
            if (cPlatformCore[i] == core)
                return CoreId(i);
        }
        return CoreId::cUndef;
#else
        return CoreId::cMain;
#endif
    }

    u32 CoreInfo::getPlatformMask(CoreId id)
    {
        return (s32) id < CoreId::cUndef ? 1u << cPlatformCore[(s32) id] : 0;
    }

} // namespace pead
