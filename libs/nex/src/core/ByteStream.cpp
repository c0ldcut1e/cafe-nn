#include <nn/nex/ByteStream.h>

namespace nn::nex
{

    void ByteStream::WriteU16(qUnsignedInt16 v)
    {
        WriteU8((qUnsignedInt8) v);
        WriteU8((qUnsignedInt8) (v >> 8));
    }

    void ByteStream::WriteU32(qUnsignedInt32 v)
    {
        WriteU16((qUnsignedInt16) v);
        WriteU16((qUnsignedInt16) (v >> 16));
    }

    void ByteStream::WriteU64(qUnsignedInt64 v)
    {
        WriteU32((qUnsignedInt32) v);
        WriteU32((qUnsignedInt32) (v >> 32));
    }

    // The length counts the terminating zero because the servers reject strings without it
    void ByteStream::WriteString(const String &s)
    {
        std::string utf8 = s.ToUtf8();
        WriteU16((qUnsignedInt16) (utf8.size() + 1));
        Write(utf8.c_str(), utf8.size() + 1);
    }

    void ByteStream::WriteBuffer(const Buffer &b)
    {
        WriteU32((qUnsignedInt32) b.GetContentSize());
        Write(b.GetContentPtr(), b.GetContentSize());
    }

    void ByteStream::WriteQBuffer(const qBuffer &b)
    {
        WriteU16((qUnsignedInt16) b.GetContentSize());
        Write(b.GetContentPtr(), b.GetContentSize());
    }

    void ByteStream::PatchU32(size_t offset, qUnsignedInt32 v)
    {
        for (qInt32 i = 0; i < 4; i++) m_data[offset + i] = (qByte) (v >> (i * 8));
    }

    bool ByteStream::Read(void *out, size_t size)
    {
        if (m_overrun || GetRemaining() < size)
        {
            m_overrun = true;
            memset(out, 0, size);
            return false;
        }
        memcpy(out, m_data.data() + m_cursor, size);
        m_cursor += size;
        return true;
    }

    qUnsignedInt8 ByteStream::ReadU8()
    {
        qUnsignedInt8 v;
        Read(&v, 1);
        return v;
    }

    qUnsignedInt16 ByteStream::ReadU16()
    {
        qUnsignedInt8 b[2];
        Read(b, 2);
        return (qUnsignedInt16) (b[0] | (b[1] << 8));
    }

    qUnsignedInt32 ByteStream::ReadU32()
    {
        qUnsignedInt32 lo = ReadU16();
        return lo | ((qUnsignedInt32) ReadU16() << 16);
    }

    qUnsignedInt64 ByteStream::ReadU64()
    {
        qUnsignedInt64 lo = ReadU32();
        return lo | ((qUnsignedInt64) ReadU32() << 32);
    }

    String ByteStream::ReadString()
    {
        qUnsignedInt16 len = ReadU16();
        std::string utf8(len, '\0');
        if (!Read(utf8.data(), len))
            return String();
        return String(utf8.c_str());
    }

    Buffer ByteStream::ReadBuffer()
    {
        qUnsignedInt32 len = ReadU32();
        if (len > GetRemaining())
        {
            m_overrun = true;
            return Buffer();
        }
        Buffer b(GetCursorPtr(), len);
        m_cursor += len;
        return b;
    }

    qBuffer ByteStream::ReadQBuffer()
    {
        qUnsignedInt16 len = ReadU16();
        if (len > GetRemaining())
        {
            m_overrun = true;
            return qBuffer();
        }
        qBuffer b(GetCursorPtr(), len);
        m_cursor += len;
        return b;
    }

} // namespace nn::nex
