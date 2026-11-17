/**
 * @file
 * @brief These are the pieces that differ between the console and a host build: the clock, the UDP socket and the random number source.
 */

#pragma once

#include <nn/nex/types.h>

namespace nn::nex
{

    class RandomNumberGenerator
    {
    public:
        RandomNumberGenerator();
        qUnsignedInt32 GetRandomNumber();
        void Fill(void *out, size_t size);

    private:
        qUnsignedInt64 m_state;
    };

} // namespace nn::nex
