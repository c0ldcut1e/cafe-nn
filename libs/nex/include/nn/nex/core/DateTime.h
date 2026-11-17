/**
 * @file
 * @brief This packs the date into the same bit fields the servers use: year, month, day, hour, minute and second from high to low.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <list>
#include <nn/nex/types.h>
#include <string>
#include <vector>

namespace nn::nex
{

    /**
     * @brief This packs the date into the same bit fields the servers use: year, month, day, hour, minute and second from high to low.
     */
    class DateTime
    {
    public:
        DateTime() : m_value(0)
        {}
        explicit DateTime(qUnsignedInt64 value) : m_value(value)
        {}
        DateTime(qUnsignedInt16 year, qUnsignedInt8 month, qUnsignedInt8 day, qUnsignedInt8 hour, qUnsignedInt8 minute, qUnsignedInt8 second)
            : m_value(((qUnsignedInt64) year << 26) | ((qUnsignedInt64) month << 22) | ((qUnsignedInt64) day << 17) | ((qUnsignedInt64) hour << 12) |
                      ((qUnsignedInt64) minute << 6) | second)
        {}

        unsigned GetYear() const
        {
            return (unsigned) (m_value >> 26);
        }
        unsigned GetMonth() const
        {
            return (unsigned) (m_value >> 22) & 0xF;
        }
        unsigned GetDay() const
        {
            return (unsigned) (m_value >> 17) & 0x1F;
        }
        unsigned GetHour() const
        {
            return (unsigned) (m_value >> 12) & 0x1F;
        }
        unsigned GetMinute() const
        {
            return (unsigned) (m_value >> 6) & 0x3F;
        }
        unsigned GetSecond() const
        {
            return (unsigned) m_value & 0x3F;
        }
        qUnsignedInt64 GetValue() const
        {
            return m_value;
        }
        bool IsNever() const
        {
            return m_value == 0;
        }
        bool IsValid() const
        {
            return GetMonth() >= 1 && GetMonth() <= 12 && GetDay() >= 1 && GetDay() <= 31;
        }
        qUnsignedInt64 ToEpochTime() const;

        static const DateTime Never;

    private:
        qUnsignedInt64 m_value;
    };

} // namespace nn::nex
