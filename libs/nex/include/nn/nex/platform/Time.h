/**
 * @file
 * @brief This returns milliseconds from a monotonic clock, so only differences between two calls mean anything.
 */

#pragma once

#include <nn/nex/types.h>

namespace nn::nex
{

    using TimeValue = qUnsignedInt64;

    class Time
    {
    public:
        /**
         * @brief This returns milliseconds from a monotonic clock, so only differences between two calls mean anything.
         */
        static TimeValue GetTime();
        static TimeValue ConvertTimeoutToDeadline(qUnsignedInt32 timeout)
        {
            return GetTime() + timeout;
        }
    };

} // namespace nn::nex
