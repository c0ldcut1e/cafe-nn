/**
 * @file
 * @brief This is pia's result code. Zero means success and every other value names a failure.
 */

#pragma once

#include <nn/pia/types.h>

namespace nn::pia
{
    class Result
    {
    public:
        static constexpr u32 SuccessValue         = 0;
        static constexpr u32 InvalidArgumentValue = 0x10c07;
        static constexpr u32 InvalidStateValue    = 0x10c08;
        static constexpr u32 NexErrorValue        = 0xe437;
        static constexpr u32 TransportErrorValue  = 0xe46c;

        // We return this from GetErrorCode for any value that has no entry of its own.
        static constexpr u32 UnknownErrorCode = 0x120160;

        constexpr Result() : m_value(SuccessValue)
        {}
        constexpr explicit Result(u32 value) : m_value(value)
        {}

        constexpr bool IsSuccess() const
        {
            return m_value == SuccessValue;
        }
        constexpr bool IsFailure() const
        {
            return m_value != SuccessValue;
        }
        constexpr u32 GetValue() const
        {
            return m_value;
        }
        constexpr bool operator==(const Result &other) const
        {
            return m_value == other.m_value;
        }

        /**
         * @brief Maps this result to the error code the game shows to the player.
         *
         * @return 0 on success. A NEX failure gives the code stored with SetNexNetworkErrorCode when one was stored.
         */
        u32 GetErrorCode() const;

        /**
         * @brief Stores the NEX error code that GetErrorCode reports for NexErrorValue. Zero clears it.
         *
         * @param code The code NEX reported for the failed call.
         */
        static void SetNexNetworkErrorCode(u32 code);

        /**
         * @brief Stores the code that GetErrorCode reports for TransportErrorValue. Zero clears it.
         *
         * @param code The code the transport reported.
         */
        static void SetTransportErrorCode(u32 code);

    private:
        u32 m_value;
    };

    constexpr Result ResultSuccess()
    {
        return Result(Result::SuccessValue);
    }
    constexpr Result ResultInvalidArgument()
    {
        return Result(Result::InvalidArgumentValue);
    }
    constexpr Result ResultInvalidState()
    {
        return Result(Result::InvalidStateValue);
    }
} // namespace nn::pia
