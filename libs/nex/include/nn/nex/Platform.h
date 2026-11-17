/**
 * @file
 * @brief These are the pieces that differ between the console and a host build: the clock, the UDP socket and the random number source.
 */

#pragma once

#include <nn/nex/InetAddress.h>

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
         * @param capacity The size of data.
         * @return The datagram size, or 0 when nothing is queued.
         */
        qInt32 RecvFrom(InetAddress *from, void *data, size_t capacity);
        InetAddress GetLocalAddress() const;

    private:
        // Windows sockets are pointer-sized handles, so we keep the descriptor in an intptr_t on every platform
        intptr_t m_fd = -1;
    };

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
