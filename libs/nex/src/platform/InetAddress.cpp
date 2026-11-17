#include <cstdio>
#include <cstring>
#include <nn/nex/core/String.h>
#include <nn/nex/platform/InetAddress.h>

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

namespace nn::nex
{

    qResult InetAddress::SetAddress(const wchar_t *address)
    {
        return SetAddress(String(address).ToUtf8().c_str());
    }

    qResult InetAddress::SetAddress(const char *address)
    {
        in_addr in;
        if (inet_pton(AF_INET, address, &in) == 1)
        {
            m_address = ntohl(in.s_addr);
            return true;
        }
        hostent *host = gethostbyname(address);
        if (!host || host->h_addrtype != AF_INET || !host->h_addr_list[0])
            return qResult(QResult::Transport_DNSError);
        memcpy(&in, host->h_addr_list[0], 4);
        m_address = ntohl(in.s_addr);
        return true;
    }

    String InetAddress::GetAddressStr() const
    {
        char buf[16];
        snprintf(buf, sizeof(buf), "%u.%u.%u.%u", (unsigned) (m_address >> 24), (unsigned) (m_address >> 16) & 0xFF,
                 (unsigned) (m_address >> 8) & 0xFF, (unsigned) m_address & 0xFF);
        return String(buf);
    }

} // namespace nn::nex
