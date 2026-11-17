/**
 * @file
 * @brief These are pead's basic types: the sized integers, byte order, ticks and CPU cores. Every other pead header builds on them.
 */

#pragma once

#include <cstddef>
#include <cstdint>

namespace pead
{
    using u8  = uint8_t;
    using u16 = uint16_t;
    using u32 = uint32_t;
    using u64 = uint64_t;
    using s8  = int8_t;
    using s16 = int16_t;
    using s32 = int32_t;
    using s64 = int64_t;
    using f32 = float;
    using f64 = double;

    /**
     * @brief This names a byte order and swaps values between it and the host's.
     */
    class Endian
    {
    public:
        enum Types
        {
            cBig    = 0,
            cLittle = 1,
        };

        static Types getHostEndian()
        {
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
            return cBig;
#else
            return cLittle;
#endif
        }

        static u16 swapU16(u16 v)
        {
            return (u16) ((v >> 8) | (v << 8));
        }
        static u32 swapU32(u32 v)
        {
            return (v >> 24) | ((v >> 8) & 0xFF00) | ((v << 8) & 0xFF0000) | (v << 24);
        }
        static u64 swapU64(u64 v)
        {
            return ((u64) swapU32((u32) v) << 32) | swapU32((u32) (v >> 32));
        }

        /**
         * @brief This converts between the host order and type, which is the same operation in both directions.
         *
         * @param type The order the value is stored in, or should be stored in.
         * @param v The value in host order or in type's order.
         * @return v unchanged when type is the host's order, otherwise v byte-swapped.
         */
        static u16 convertU16(Types type, u16 v)
        {
            return type == getHostEndian() ? v : swapU16(v);
        }
        static u32 convertU32(Types type, u32 v)
        {
            return type == getHostEndian() ? v : swapU32(v);
        }
        static u64 convertU64(Types type, u64 v)
        {
            return type == getHostEndian() ? v : swapU64(v);
        }
    };

    /**
     * @brief This is a span of time in ticks, the console's timer unit. Host builds count nanoseconds instead, so always convert through the to and from functions.
     */
    class TickSpan
    {
    public:
        TickSpan() : m_span(0)
        {}
        explicit TickSpan(s64 ticks) : m_span(ticks)
        {}

        static s64 getFrequency();

        static TickSpan fromNanoSeconds(s64 ns);
        static TickSpan fromMicroSeconds(s64 us)
        {
            return fromNanoSeconds(us * 1000);
        }
        static TickSpan fromMilliSeconds(s64 ms)
        {
            return fromNanoSeconds(ms * 1000000);
        }
        static TickSpan fromSeconds(s64 s)
        {
            return fromNanoSeconds(s * 1000000000);
        }

        s64 toTicks() const
        {
            return m_span;
        }
        s64 toNanoSeconds() const;
        s64 toMicroSeconds() const
        {
            return toNanoSeconds() / 1000;
        }
        s64 toMilliSeconds() const
        {
            return toNanoSeconds() / 1000000;
        }
        s64 toSeconds() const
        {
            return toNanoSeconds() / 1000000000;
        }

        TickSpan operator+(TickSpan other) const
        {
            return TickSpan(m_span + other.m_span);
        }
        TickSpan operator-(TickSpan other) const
        {
            return TickSpan(m_span - other.m_span);
        }
        bool operator<(TickSpan other) const
        {
            return m_span < other.m_span;
        }

    private:
        s64 m_span;
    };

    /**
     * @brief This is a point on the monotonic tick clock.
     */
    class TickTime
    {
    public:
        TickTime() : m_tick(now())
        {}

        static s64 now();

        void setNow()
        {
            m_tick = now();
        }
        TickSpan diffToNow() const
        {
            return TickSpan(now() - m_tick);
        }
        s64 toTicks() const
        {
            return m_tick;
        }

    private:
        s64 m_tick;
    };

    /**
     * @brief This names one of the console's three CPU cores. Host builds map them onto whatever cores the machine has.
     */
    class CoreId
    {
    public:
        enum ValueType
        {
            cMain  = 0,
            cSub1  = 1,
            cSub2  = 2,
            cUndef = 3,
        };

        CoreId() : m_value(cUndef)
        {}
        CoreId(ValueType v) : m_value(v)
        {}
        explicit CoreId(s32 v) : m_value((ValueType) v)
        {}

        operator s32() const
        {
            return m_value;
        }

    private:
        ValueType m_value;
    };

    /**
     * @brief This is a set of cores, one bit per CoreId, that a thread may run on.
     */
    class CoreIdMask
    {
    public:
        CoreIdMask() : m_mask(0)
        {}
        explicit CoreIdMask(u32 mask) : m_mask(mask)
        {}
        CoreIdMask(CoreId id) : m_mask(1u << (s32) id)
        {}

        static CoreIdMask all()
        {
            return CoreIdMask(7);
        }

        void set(CoreId id)
        {
            m_mask |= 1u << (s32) id;
        }
        void reset(CoreId id)
        {
            m_mask &= ~(1u << (s32) id);
        }
        bool isOn(CoreId id) const
        {
            return (m_mask & (1u << (s32) id)) != 0;
        }
        u32 getMask() const
        {
            return m_mask;
        }

    private:
        u32 m_mask;
    };

    class CoreInfo
    {
    public:
        /**
         * @brief This reads the core count and main core from the system. Call it once before using the other functions.
         */
        static void configure();

        static u32 getNumCores()
        {
            return s_numCores;
        }
        static CoreId getMainCoreId()
        {
            return s_mainCoreId;
        }
        static CoreId getCurrentCoreId();

        /**
         * @brief This turns a core into the affinity bit the system thread API wants.
         *
         * @param id The core, which is mapped to a hardware core first.
         * @return The OS affinity mask with only that core's bit set.
         */
        static u32 getPlatformMask(CoreId id);

    private:
        static u32 s_numCores;
        static CoreId s_mainCoreId;
    };

} // namespace pead
