/**
 * @file
 * @brief These are bit helpers and a typed bit set.
 */

#pragma once

#include <pead/peadTypes.h>

namespace pead
{
    class BitFlagUtil
    {
    public:
        static s32 countOnBit(u32 x);

        /**
         * @brief This finds the nth set bit, counting from bit 0.
         *
         * @param x The bits to search.
         * @param num Which set bit to find, starting at 1.
         * @return The bit index, or -1 when x has fewer than num set bits.
         */
        static s32 findOnBitFromRight(u32 x, s32 num);
    };

    template<typename T>
    class BitFlag
    {
    public:
        BitFlag() : m_bits(0)
        {}
        explicit BitFlag(T bits) : m_bits(bits)
        {}

        void makeAllZero()
        {
            m_bits = 0;
        }
        void makeAllOne()
        {
            m_bits = (T) ~(T) 0;
        }
        void set(T mask)
        {
            m_bits |= mask;
        }
        void reset(T mask)
        {
            m_bits &= ~mask;
        }
        void toggle(T mask)
        {
            m_bits ^= mask;
        }
        void change(T mask, bool on)
        {
            on ? set(mask) : reset(mask);
        }
        bool isOn(T mask) const
        {
            return (m_bits & mask) != 0;
        }
        bool isOnAll(T mask) const
        {
            return (m_bits & mask) == mask;
        }
        bool isOff(T mask) const
        {
            return !isOn(mask);
        }
        void setBit(s32 bit)
        {
            set((T) ((T) 1 << bit));
        }
        void resetBit(s32 bit)
        {
            reset((T) ((T) 1 << bit));
        }
        bool isOnBit(s32 bit) const
        {
            return isOn((T) ((T) 1 << bit));
        }
        s32 countOnBit() const
        {
            return BitFlagUtil::countOnBit((u32) m_bits);
        }
        T getDirect() const
        {
            return m_bits;
        }
        void setDirect(T bits)
        {
            m_bits = bits;
        }

    private:
        T m_bits;
    };

    using BitFlag8  = BitFlag<u8>;
    using BitFlag16 = BitFlag<u16>;
    using BitFlag32 = BitFlag<u32>;

} // namespace pead
