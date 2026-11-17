#include <pead/peadRandom.h>

namespace pead
{

    void Random::init()
    {
        init((u32) TickTime::now());
    }

    void Random::init(u32 seed)
    {
        // Each word comes from the previous one through the Mersenne Twister seeding step, so even seed 0 gives a non-zero state.
        m_x = (seed ^ (seed >> 30)) * 0x6C078965 + 1;
        m_y = (m_x ^ (m_x >> 30)) * 0x6C078965 + 2;
        m_z = (m_y ^ (m_y >> 30)) * 0x6C078965 + 3;
        m_w = (m_z ^ (m_z >> 30)) * 0x6C078965 + 4;
    }

    void Random::init(u32 seed0, u32 seed1, u32 seed2, u32 seed3)
    {
        // An all-zero state would only ever produce zeros, so we fall back to seeding from 0.
        if ((seed0 | seed1 | seed2 | seed3) == 0)
        {
            init(0u);
            return;
        }
        m_x = seed0;
        m_y = seed1;
        m_z = seed2;
        m_w = seed3;
    }

    u32 Random::getU32()
    {
        u32 t = m_x ^ (m_x << 11);
        m_x   = m_y;
        m_y   = m_z;
        m_z   = m_w;
        m_w   = m_w ^ (m_w >> 19) ^ t ^ (t >> 8);
        return m_w;
    }

    u64 Random::getU64()
    {
        u64 hi = getU32();
        return (hi << 32) | getU32();
    }

} // namespace pead
