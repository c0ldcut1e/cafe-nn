#include <cstdio>
#include <nn/pia/common/InetAddress.h>

namespace nn::pia::common
{
    bool InetAddress::IsPrivate() const
    {
        return (m_address >> 24) == 10 || (m_address >> 20) == 0xac1 || (m_address >> 16) == 0xc0a8;
    }

    Result InetAddress::Serialize(u8 *buffer, u32 size) const
    {
        if (!buffer || size < SerializedSize)
            return ResultInvalidArgument();
        buffer[0] = (u8) (m_address >> 24);
        buffer[1] = (u8) (m_address >> 16);
        buffer[2] = (u8) (m_address >> 8);
        buffer[3] = (u8) m_address;
        buffer[4] = (u8) (m_port >> 8);
        buffer[5] = (u8) m_port;
        return ResultSuccess();
    }

    Result InetAddress::Deserialize(const u8 *buffer, u32 size)
    {
        if (!buffer || size < SerializedSize)
            return ResultInvalidArgument();
        m_address = ((u32) buffer[0] << 24) | ((u32) buffer[1] << 16) | ((u32) buffer[2] << 8) | buffer[3];
        m_port    = (u16) (((u32) buffer[4] << 8) | buffer[5]);
        return ResultSuccess();
    }

    void InetAddress::ToString(char *buffer, u32 size) const
    {
        if (!buffer || size == 0)
            return;
        snprintf(buffer, size, "%u.%u.%u.%u:%u", (unsigned) (m_address >> 24), (unsigned) ((m_address >> 16) & 0xff),
                 (unsigned) ((m_address >> 8) & 0xff), (unsigned) (m_address & 0xff), (unsigned) m_port);
    }
} // namespace nn::pia::common
