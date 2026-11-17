/**
 * @file
 * @brief These are pia's clock types. A Time is a point on the system tick clock and a TimeSpan is a distance between two of them.
 */

#pragma once

#include <compare>
#include <nn/pia/types.h>

namespace nn::pia::common
{
    class TimeSpan
    {
    public:
        constexpr TimeSpan() : m_ticks(0)
        {}
        constexpr explicit TimeSpan(s64 ticks) : m_ticks(ticks)
        {}

        static s64 GetTicksPerMSec();
        static s64 GetTicksPerSec();

        static TimeSpan FromMilliSeconds(s64 ms)
        {
            return TimeSpan(ms * GetTicksPerMSec());
        }
        static TimeSpan FromSeconds(s64 s)
        {
            return TimeSpan(s * GetTicksPerSec());
        }

        constexpr s64 GetTicks() const
        {
            return m_ticks;
        }
        s64 GetMilliSeconds() const
        {
            return m_ticks / GetTicksPerMSec();
        }
        s64 GetSeconds() const
        {
            return m_ticks / GetTicksPerSec();
        }

        constexpr TimeSpan operator+(TimeSpan other) const
        {
            return TimeSpan(m_ticks + other.m_ticks);
        }
        constexpr TimeSpan operator-(TimeSpan other) const
        {
            return TimeSpan(m_ticks - other.m_ticks);
        }
        constexpr auto operator<=>(const TimeSpan &other) const = default;

    private:
        s64 m_ticks;
    };

    class Time
    {
    public:
        constexpr Time() : m_ticks(0)
        {}
        constexpr explicit Time(u64 ticks) : m_ticks(ticks)
        {}

        void SetNow();
        static Time GetNow();

        constexpr u64 GetTicks() const
        {
            return m_ticks;
        }
        constexpr bool IsInvalid() const
        {
            return m_ticks == 0;
        }

        constexpr Time operator+(TimeSpan span) const
        {
            return Time(m_ticks + (u64) span.GetTicks());
        }
        constexpr Time operator-(TimeSpan span) const
        {
            return Time(m_ticks - (u64) span.GetTicks());
        }
        constexpr TimeSpan operator-(const Time &other) const
        {
            return TimeSpan((s64) (m_ticks - other.m_ticks));
        }
        constexpr auto operator<=>(const Time &other) const = default;

    private:
        u64 m_ticks;
    };
} // namespace nn::pia::common
