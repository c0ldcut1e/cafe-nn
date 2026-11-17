/**
 * @file
 * @brief This is the address one station can be reached at. It serializes exactly like the InetAddress it wraps.
 */

#pragma once

#include <nn/pia/common/InetAddress.h>

namespace nn::pia::common
{
    class StationAddress
    {
    public:
        static constexpr u32 SerializedSize = InetAddress::SerializedSize;

        StationAddress() = default;
        explicit StationAddress(const InetAddress &address) : m_inetAddress(address)
        {}

        const InetAddress &GetInetAddress() const
        {
            return m_inetAddress;
        }
        void SetInetAddress(const InetAddress &address)
        {
            m_inetAddress = address;
        }
        bool IsValid() const
        {
            return m_inetAddress.IsValid();
        }

        Result Serialize(u8 *buffer, u32 size) const
        {
            return m_inetAddress.Serialize(buffer, size);
        }
        Result Deserialize(const u8 *buffer, u32 size)
        {
            return m_inetAddress.Deserialize(buffer, size);
        }

        /**
         * @brief Orders addresses by their keys compared as signed numbers, so addresses from 128.0.0.0 up sort before the rest.
         *
         * @return Negative, zero or positive like strcmp.
         */
        s32 Compare(const StationAddress &other) const
        {
            s64 a = (s64) m_inetAddress.GetKey();
            s64 b = (s64) other.m_inetAddress.GetKey();
            return a < b ? -1 : a > b ? 1 : 0;
        }
        bool operator<(const StationAddress &other) const
        {
            return Compare(other) < 0;
        }
        bool operator==(const StationAddress &other) const
        {
            return m_inetAddress == other.m_inetAddress;
        }

    private:
        InetAddress m_inetAddress;
    };
} // namespace nn::pia::common
