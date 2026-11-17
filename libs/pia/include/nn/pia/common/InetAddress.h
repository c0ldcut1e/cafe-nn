/**
 * @file
 * @brief This is an IPv4 address and port in host byte order. On the wire it is six bytes, the address and then the port, both big-endian.
 */

#pragma once

#include <nn/pia/Result.h>

namespace nn::pia::common
{
    class InetAddress
    {
    public:
        static constexpr u32 SerializedSize = 6;

        constexpr InetAddress() : m_address(0), m_port(0)
        {}
        constexpr InetAddress(u32 address, u16 port) : m_address(address), m_port(port)
        {}

        void Set(u32 address, u16 port)
        {
            m_address = address;
            m_port    = port;
        }
        constexpr u32 GetAddress() const
        {
            return m_address;
        }
        constexpr u16 GetPort() const
        {
            return m_port;
        }
        void SetPort(u16 port)
        {
            m_port = port;
        }
        constexpr bool IsValid() const
        {
            return m_address != 0;
        }

        /**
         * @brief Tells whether the address is in 10/8, 172.16/12 or 192.168/16.
         */
        bool IsPrivate() const;

        /**
         * @brief Packs the address into the high 32 bits and the port into the low ones, so keys order by address first.
         */
        constexpr u64 GetKey() const
        {
            return ((u64) m_address << 32) | m_port;
        }

        Result Serialize(u8 *buffer, u32 size) const;
        Result Deserialize(const u8 *buffer, u32 size);

        /**
         * @brief Writes the address as "a.b.c.d:port".
         *
         * @param buffer Needs room for 22 characters including the terminator.
         */
        void ToString(char *buffer, u32 size) const;

        constexpr bool operator==(const InetAddress &other) const
        {
            return m_address == other.m_address && m_port == other.m_port;
        }

    private:
        u32 m_address;
        u16 m_port;
    };
} // namespace nn::pia::common
