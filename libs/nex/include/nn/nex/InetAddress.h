/**
 * @file
 * @brief This is an IPv4 address and port as the socket layer and station URLs use them.
 */

#pragma once

#include <nn/nex/types.h>

namespace nn::nex
{

    /**
     * @brief This keeps the address in host byte order, so the socket layer is the only place that swaps it.
     */
    class InetAddress
    {
    public:
        InetAddress() = default;
        InetAddress(qUnsignedInt32 address, qUnsignedInt16 port) : m_address(address), m_port(port)
        {}
        InetAddress(const InetAddress &)            = default;
        InetAddress &operator=(const InetAddress &) = default;
        ~InetAddress()                              = default;

        qUnsignedInt32 GetAddress() const
        {
            return m_address;
        }
        void SetAddress(qUnsignedInt32 address)
        {
            m_address = address;
        }

        /**
         * @brief Host names are resolved here as well as dotted quads, since server URLs may hold either.
         *
         * @param address A dotted quad or a host name.
         * @return Whether the address could be parsed or resolved.
         */
        qResult SetAddress(const wchar_t *address);
        qResult SetAddress(const char *address);
        String GetAddressStr() const;
        qUnsignedInt16 GetPortNumber() const
        {
            return m_port;
        }
        void SetPortNumber(qUnsignedInt16 port)
        {
            m_port = port;
        }

        bool operator==(const InetAddress &o) const
        {
            return m_address == o.m_address && m_port == o.m_port;
        }

    private:
        qUnsignedInt32 m_address = 0;
        qUnsignedInt16 m_port    = 0;
    };

} // namespace nn::nex
