#include <chrono>
#include <cstdint>
#include <nn/nex/platform/RandomNumberGenerator.h>
#include <nn/nex/platform/Time.h>

namespace nn::nex
{

    // We seed from the clock and the stack address since connection signatures only need to be unpredictable to other players
    RandomNumberGenerator::RandomNumberGenerator()
    {
        qInt32 local;
        m_state = Time::GetTime() ^ ((qUnsignedInt64) (uintptr_t) &local << 16) ^ 0x9E3779B97F4A7C15ull;
#ifndef __WIIU__
        m_state ^= (qUnsignedInt64) std::chrono::system_clock::now().time_since_epoch().count() << 20;
#endif
    }

    qUnsignedInt32 RandomNumberGenerator::GetRandomNumber()
    {
        m_state ^= m_state << 13;
        m_state ^= m_state >> 7;
        m_state ^= m_state << 17;
        return (qUnsignedInt32) (m_state >> 16);
    }

    void RandomNumberGenerator::Fill(void *out, size_t size)
    {
        auto *p = (qUnsignedInt8 *) out;
        for (size_t i = 0; i < size; i++) p[i] = (qUnsignedInt8) GetRandomNumber();
    }

} // namespace nn::nex
