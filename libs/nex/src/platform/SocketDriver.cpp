#include <nn/nex/platform/InetAddress.h>
#include <nn/nex/platform/SocketDriver.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#ifdef __WIIU__
#include <coreinit/time.h>
#endif

namespace nn::nex
{

    namespace
    {
#ifdef _WIN32
        using Socket = SOCKET;
#else
        using Socket = qInt32;
#endif
    } // namespace

    qResult SocketDriver::Open(qUnsignedInt16 localPort)
    {
        Close();
#ifdef _WIN32
        // Winsock has to be started once per process before any socket call, and later calls only bump a counter
        static const bool started = []
        {
            WSADATA data;
            return WSAStartup(MAKEWORD(2, 2), &data) == 0;
        }();
        if (!started)
            return qResult(QResult::Transport_IOError);
#endif
        m_fd = (intptr_t) socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (m_fd < 0)
            return qResult(QResult::Transport_IOError);
        sockaddr_in local     = {};
        local.sin_family      = AF_INET;
        local.sin_port        = htons(localPort);
        local.sin_addr.s_addr = htonl(INADDR_ANY);
        if (bind((Socket) m_fd, (sockaddr *) &local, sizeof(local)) < 0)
        {
            Close();
            return qResult(QResult::Transport_IOError);
        }
#if defined(__WIIU__)
        qInt32 nonBlocking = 1;
        setsockopt(m_fd, SOL_SOCKET, SO_NONBLOCK, &nonBlocking, sizeof(nonBlocking));
#elif defined(_WIN32)
        u_long nonBlocking = 1;
        ioctlsocket((SOCKET) m_fd, FIONBIO, &nonBlocking);
#else
        fcntl(m_fd, F_SETFL, fcntl(m_fd, F_GETFL, 0) | O_NONBLOCK);
#endif
        return true;
    }

    void SocketDriver::Close()
    {
        if (m_fd >= 0)
        {
#ifdef _WIN32
            closesocket((SOCKET) m_fd);
#else
            close((qInt32) m_fd);
#endif
        }
        m_fd = -1;
    }

    qResult SocketDriver::SendTo(const InetAddress &to, const void *data, size_t size)
    {
        sockaddr_in addr     = {};
        addr.sin_family      = AF_INET;
        addr.sin_port        = htons(to.GetPortNumber());
        addr.sin_addr.s_addr = htonl(to.GetAddress());
        if (sendto((Socket) m_fd, (const char *) data, (qInt32) size, 0, (sockaddr *) &addr, sizeof(addr)) < 0)
            return qResult(QResult::Transport_IOError);
        return true;
    }

    qInt32 SocketDriver::RecvFrom(InetAddress *from, void *data, size_t capacity)
    {
        sockaddr_in addr = {};
        socklen_t len    = sizeof(addr);
        qInt32 n         = (qInt32) recvfrom((Socket) m_fd, (char *) data, (qInt32) capacity, 0, (sockaddr *) &addr, &len);
        if (n <= 0)
            return 0;
        if (from)
            *from = InetAddress(ntohl(addr.sin_addr.s_addr), ntohs(addr.sin_port));
        return n;
    }

    InetAddress SocketDriver::GetLocalAddress() const
    {
        sockaddr_in addr = {};
        socklen_t len    = sizeof(addr);
        getsockname((Socket) m_fd, (sockaddr *) &addr, &len);
        return InetAddress(ntohl(addr.sin_addr.s_addr), ntohs(addr.sin_port));
    }

} // namespace nn::nex
