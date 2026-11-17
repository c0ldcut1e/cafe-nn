/**
 * @file
 * @brief This is a thin non-blocking UDP socket, since every NEX transport runs over one datagram socket.
 */

#pragma once

#include <nn/nex/platform/InetAddress.h>
#include <nn/nex/types.h>

namespace nn::nex
{

    /**
     * @brief This is a thin non-blocking UDP socket, since every NEX transport runs over one datagram socket.
     */
    class SocketDriver
    {
    public:
        SocketDriver() = default;
        ~SocketDriver()
        {
            Close();
        }
        SocketDriver(const SocketDriver &)            = delete;
        SocketDriver &operator=(const SocketDriver &) = delete;

        qResult Open(qUnsignedInt16 localPort = 0);
        void Close();
        bool IsOpen() const
        {
            return m_fd >= 0;
        }
        qResult SendTo(const InetAddress &to, const void *data, size_t size);

        /**
         * @brief This returns zero rather than blocking when nothing is queued.
         *
         * @param from Receives the sender's address.
         * @param data Receives the datagram.
         * @param capacity The size of data. A longer datagram is cut off.
         * @return The datagram size, or 0 when nothing is queued.
         */
        qInt32 RecvFrom(InetAddress *from, void *data, size_t capacity);
        InetAddress GetLocalAddress() const;

    private:
        // Windows sockets are pointer-sized handles, so we keep the descriptor in an intptr_t on every platform
        intptr_t m_fd = -1;
    };

} // namespace nn::nex
