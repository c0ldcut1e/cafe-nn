/**
 * @file
 * @brief This is pead's pseudo-random generator, a xorshift128 seeded the same way as the console library, so a seed gives the same sequence on every platform.
 */

#pragma once

#include <pead/peadTypes.h>

namespace pead
{
    class Random
    {
    public:
        Random()
        {
            init();
        }
        explicit Random(u32 seed)
        {
            init(seed);
        }

        /**
         * @brief This seeds from the tick clock, so two generators made at different times differ.
         */
        void init();
        void init(u32 seed);
        void init(u32 seed0, u32 seed1, u32 seed2, u32 seed3);

        u32 getU32();
        u64 getU64();

        /**
         * @brief This returns a number in [0, ceil).
         *
         * @param ceil The exclusive upper bound.
         * @return The number, or 0 when ceil is 0.
         */
        u32 getU32(u32 ceil)
        {
            return (u32) (((u64) getU32() * ceil) >> 32);
        }
        s32 getS32Range(s32 a, s32 b)
        {
            return (s32) getU32((u32) (b - a)) + a;
        }
        f32 getF32()
        {
            return (f32) (getU32() >> 8) * (1.0f / 16777216.0f);
        }
        f64 getF64()
        {
            return (f64) (getU64() >> 11) * (1.0 / 9007199254740992.0);
        }
        bool getBool()
        {
            return (getU32() & 0x80000000) != 0;
        }

        void getContext(u32 *s0, u32 *s1, u32 *s2, u32 *s3) const
        {
            *s0 = m_x;
            *s1 = m_y;
            *s2 = m_z;
            *s3 = m_w;
        }

    private:
        u32 m_x;
        u32 m_y;
        u32 m_z;
        u32 m_w;
    };

} // namespace pead
