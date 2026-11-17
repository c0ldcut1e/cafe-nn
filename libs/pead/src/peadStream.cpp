#include <cstdio>
#include <cstring>
#include <pead/peadStream.h>
#include <pead/peadStringUtil.h>

namespace pead
{

    namespace
    {

        BinaryStreamFormat s_binaryFormat;
        TextStreamFormat s_textFormat;

        constexpr s32 cTokenSize = 64;

        bool IsSpace(char c)
        {
            return c == ' ' || c == '\t' || c == '\n' || c == '\r';
        }

        s32 HexDigit(char c)
        {
            if (c >= '0' && c <= '9')
                return c - '0';
            if (c >= 'a' && c <= 'f')
                return c - 'a' + 10;
            if (c >= 'A' && c <= 'F')
                return c - 'A' + 10;
            return -1;
        }

        template<typename T>
        T ReadRaw(StreamSrc *src)
        {
            T value;
            memset(&value, 0, sizeof(value));
            src->read(&value, sizeof(value));
            return value;
        }

        template<typename T>
        void WriteRaw(StreamSrc *src, T value)
        {
            src->write(&value, sizeof(value));
        }

    } // namespace

    u32 RamStreamSrc::read(void *dst, u32 size)
    {
        u32 left = m_position < m_size ? m_size - m_position : 0;
        if (size > left)
            size = left;
        memcpy(dst, m_start + m_position, size);
        m_position += size;
        return size;
    }

    u32 RamStreamSrc::write(const void *src, u32 size)
    {
        u32 left = m_position < m_size ? m_size - m_position : 0;
        if (size > left)
            size = left;
        memcpy(m_start + m_position, src, size);
        m_position += size;
        return size;
    }

    u32 RamStreamSrc::skip(s32 offset)
    {
        s64 position = (s64) m_position + offset;
        if (position < 0)
            position = 0;
        if (position > (s64) m_size)
            position = m_size;
        u32 moved  = (u32) (position > m_position ? position - m_position : m_position - position);
        m_position = (u32) position;
        return moved;
    }

    u8 BinaryStreamFormat::readU8(StreamSrc *src, Endian::Types)
    {
        return ReadRaw<u8>(src);
    }

    u16 BinaryStreamFormat::readU16(StreamSrc *src, Endian::Types endian)
    {
        return Endian::convertU16(endian, ReadRaw<u16>(src));
    }

    u32 BinaryStreamFormat::readU32(StreamSrc *src, Endian::Types endian)
    {
        return Endian::convertU32(endian, ReadRaw<u32>(src));
    }

    u64 BinaryStreamFormat::readU64(StreamSrc *src, Endian::Types endian)
    {
        return Endian::convertU64(endian, ReadRaw<u64>(src));
    }

    s8 BinaryStreamFormat::readS8(StreamSrc *src, Endian::Types endian)
    {
        return (s8) readU8(src, endian);
    }

    s16 BinaryStreamFormat::readS16(StreamSrc *src, Endian::Types endian)
    {
        return (s16) readU16(src, endian);
    }

    s32 BinaryStreamFormat::readS32(StreamSrc *src, Endian::Types endian)
    {
        return (s32) readU32(src, endian);
    }

    s64 BinaryStreamFormat::readS64(StreamSrc *src, Endian::Types endian)
    {
        return (s64) readU64(src, endian);
    }

    f32 BinaryStreamFormat::readF32(StreamSrc *src, Endian::Types endian)
    {
        u32 bits = readU32(src, endian);
        f32 value;
        memcpy(&value, &bits, sizeof(value));
        return value;
    }

    f64 BinaryStreamFormat::readF64(StreamSrc *src, Endian::Types endian)
    {
        u64 bits = readU64(src, endian);
        f64 value;
        memcpy(&value, &bits, sizeof(value));
        return value;
    }

    void BinaryStreamFormat::readBit(StreamSrc *src, void *dst, u32 bitCount)
    {
        src->read(dst, (bitCount + 7) / 8);
    }

    void BinaryStreamFormat::readString(StreamSrc *src, BufferedSafeString *dst, u32 size)
    {
        // We always consume size bytes so the stream stays in step, even when dst keeps fewer of them.
        char *buffer = dst->getBuffer();
        u32 keep     = (s32) size < dst->getBufferSize() ? size : (u32) dst->getBufferSize() - 1;
        u32 got      = src->read(buffer, keep);
        buffer[got]  = '\0';
        if (size > keep)
            src->skip((s32) (size - keep));
    }

    u32 BinaryStreamFormat::readMemBlock(StreamSrc *src, void *dst, u32 size)
    {
        return src->read(dst, size);
    }

    void BinaryStreamFormat::writeU8(StreamSrc *src, Endian::Types, u8 value)
    {
        WriteRaw(src, value);
    }

    void BinaryStreamFormat::writeU16(StreamSrc *src, Endian::Types endian, u16 value)
    {
        WriteRaw(src, Endian::convertU16(endian, value));
    }

    void BinaryStreamFormat::writeU32(StreamSrc *src, Endian::Types endian, u32 value)
    {
        WriteRaw(src, Endian::convertU32(endian, value));
    }

    void BinaryStreamFormat::writeU64(StreamSrc *src, Endian::Types endian, u64 value)
    {
        WriteRaw(src, Endian::convertU64(endian, value));
    }

    void BinaryStreamFormat::writeS8(StreamSrc *src, Endian::Types endian, s8 value)
    {
        writeU8(src, endian, (u8) value);
    }

    void BinaryStreamFormat::writeS16(StreamSrc *src, Endian::Types endian, s16 value)
    {
        writeU16(src, endian, (u16) value);
    }

    void BinaryStreamFormat::writeS32(StreamSrc *src, Endian::Types endian, s32 value)
    {
        writeU32(src, endian, (u32) value);
    }

    void BinaryStreamFormat::writeS64(StreamSrc *src, Endian::Types endian, s64 value)
    {
        writeU64(src, endian, (u64) value);
    }

    void BinaryStreamFormat::writeF32(StreamSrc *src, Endian::Types endian, f32 value)
    {
        u32 bits;
        memcpy(&bits, &value, sizeof(bits));
        writeU32(src, endian, bits);
    }

    void BinaryStreamFormat::writeF64(StreamSrc *src, Endian::Types endian, f64 value)
    {
        u64 bits;
        memcpy(&bits, &value, sizeof(bits));
        writeU64(src, endian, bits);
    }

    void BinaryStreamFormat::writeBit(StreamSrc *src, const void *data, u32 bitCount)
    {
        src->write(data, (bitCount + 7) / 8);
    }

    void BinaryStreamFormat::writeString(StreamSrc *src, const SafeString &str, u32 size)
    {
        u32 length = (u32) str.calcLength();
        if (length > size)
            length = size;
        src->write(str.cstr(), length);
        for (u32 i = length; i < size; i++) WriteRaw<u8>(src, 0);
    }

    void BinaryStreamFormat::writeMemBlock(StreamSrc *src, const void *data, u32 size)
    {
        src->write(data, size);
    }

    void BinaryStreamFormat::skip(StreamSrc *src, u32 size)
    {
        src->skip((s32) size);
    }

    void BinaryStreamFormat::flush(StreamSrc *src)
    {
        src->flush();
    }

    void BinaryStreamFormat::rewind(StreamSrc *src)
    {
        src->rewind();
    }

    s32 TextStreamFormat::readToken_(StreamSrc *src, char *buffer, s32 size)
    {
        char c = 0;
        for (;;)
        {
            if (src->read(&c, 1) != 1)
            {
                buffer[0] = '\0';
                return 0;
            }
            if (c == '#')
            {
                while (src->read(&c, 1) == 1 && c != '\n')
                {
                }
                continue;
            }
            if (!IsSpace(c))
                break;
        }

        s32 length  = 0;
        bool quoted = c == '"';
        if (!quoted && length + 1 < size)
            buffer[length++] = c;
        while (src->read(&c, 1) == 1)
        {
            if (quoted ? c == '"' : IsSpace(c))
                break;
            if (quoted && c == '\\' && src->read(&c, 1) != 1)
                break;
            if (length + 1 < size)
                buffer[length++] = c;
        }
        buffer[length] = '\0';
        return length;
    }

    void TextStreamFormat::writeToken_(StreamSrc *src, const char *text, s32 length)
    {
        src->write(text, (u32) length);
        src->write(" ", 1);
    }

#define PEAD_TEXT_READ_INT(name, type, parser)                                                                                                       \
    type TextStreamFormat::name(StreamSrc *src, Endian::Types)                                                                                       \
    {                                                                                                                                                \
        char token[cTokenSize];                                                                                                                      \
        readToken_(src, token, cTokenSize);                                                                                                          \
        type value = 0;                                                                                                                              \
        StringUtil::parser(&value, token, StringUtil::cBaseAuto);                                                                                    \
        return value;                                                                                                                                \
    }

    PEAD_TEXT_READ_INT(readU8, u8, tryParseU8)
    PEAD_TEXT_READ_INT(readU16, u16, tryParseU16)
    PEAD_TEXT_READ_INT(readU32, u32, tryParseU32)
    PEAD_TEXT_READ_INT(readU64, u64, tryParseU64)
    PEAD_TEXT_READ_INT(readS8, s8, tryParseS8)
    PEAD_TEXT_READ_INT(readS16, s16, tryParseS16)
    PEAD_TEXT_READ_INT(readS32, s32, tryParseS32)
    PEAD_TEXT_READ_INT(readS64, s64, tryParseS64)

#undef PEAD_TEXT_READ_INT

    f32 TextStreamFormat::readF32(StreamSrc *src, Endian::Types)
    {
        char token[cTokenSize];
        readToken_(src, token, cTokenSize);
        f32 value = 0;
        StringUtil::tryParseF32(&value, token);
        return value;
    }

    f64 TextStreamFormat::readF64(StreamSrc *src, Endian::Types)
    {
        char token[cTokenSize];
        readToken_(src, token, cTokenSize);
        f64 value = 0;
        StringUtil::tryParseF64(&value, token);
        return value;
    }

    void TextStreamFormat::readBit(StreamSrc *src, void *dst, u32 bitCount)
    {
        u8 *bytes = (u8 *) dst;
        memset(bytes, 0, (bitCount + 7) / 8);
        char token[cTokenSize * 4];
        s32 length = readToken_(src, token, (s32) sizeof(token));
        for (u32 i = 0; i < bitCount && (s32) i < length; i++)
        {
            if (token[i] == '1')
                bytes[i / 8] |= (u8) (1u << (i % 8));
        }
    }

    void TextStreamFormat::readString(StreamSrc *src, BufferedSafeString *dst, u32)
    {
        readToken_(src, dst->getBuffer(), dst->getBufferSize());
    }

    u32 TextStreamFormat::readMemBlock(StreamSrc *src, void *dst, u32 size)
    {
        u8 *bytes = (u8 *) dst;
        u32 count = 0;
        char c    = 0;
        while (src->read(&c, 1) == 1 && IsSpace(c))
        {
        }
        s32 high = HexDigit(c);
        while (high >= 0 && count < size)
        {
            if (src->read(&c, 1) != 1 || HexDigit(c) < 0)
                break;
            bytes[count++] = (u8) ((high << 4) | HexDigit(c));
            if (src->read(&c, 1) != 1)
                break;
            high = HexDigit(c);
        }
        return count;
    }

#define PEAD_TEXT_WRITE(name, type, cast, fmt)                                                                                                       \
    void TextStreamFormat::name(StreamSrc *src, Endian::Types, type value)                                                                           \
    {                                                                                                                                                \
        char token[cTokenSize];                                                                                                                      \
        s32 length = StringUtil::snprintf(token, cTokenSize, fmt, (cast) value);                                                                     \
        writeToken_(src, token, length);                                                                                                             \
    }

    PEAD_TEXT_WRITE(writeU8, u8, unsigned, "%u")
    PEAD_TEXT_WRITE(writeU16, u16, unsigned, "%u")
    PEAD_TEXT_WRITE(writeU32, u32, unsigned, "%u")
    PEAD_TEXT_WRITE(writeU64, u64, unsigned long long, "%llu")
    PEAD_TEXT_WRITE(writeS8, s8, int, "%d")
    PEAD_TEXT_WRITE(writeS16, s16, int, "%d")
    PEAD_TEXT_WRITE(writeS32, s32, int, "%d")
    PEAD_TEXT_WRITE(writeS64, s64, long long, "%lld")
    PEAD_TEXT_WRITE(writeF32, f32, double, "%.9g")
    PEAD_TEXT_WRITE(writeF64, f64, double, "%.17g")

#undef PEAD_TEXT_WRITE

    void TextStreamFormat::writeBit(StreamSrc *src, const void *data, u32 bitCount)
    {
        const u8 *bytes = (const u8 *) data;
        for (u32 i = 0; i < bitCount; i++) src->write((bytes[i / 8] >> (i % 8)) & 1 ? "1" : "0", 1);
        src->write(" ", 1);
    }

    void TextStreamFormat::writeString(StreamSrc *src, const SafeString &str, u32)
    {
        // We quote strings so ones holding spaces read back as one token.
        const char *s = str.cstr();
        src->write("\"", 1);
        for (; *s; s++)
        {
            if (*s == '"' || *s == '\\')
                src->write("\\", 1);
            src->write(s, 1);
        }
        src->write("\" ", 2);
    }

    void TextStreamFormat::writeMemBlock(StreamSrc *src, const void *data, u32 size)
    {
        static const char cHex[] = "0123456789ABCDEF";
        const u8 *bytes          = (const u8 *) data;
        for (u32 i = 0; i < size; i++)
        {
            char pair[2] = {cHex[bytes[i] >> 4], cHex[bytes[i] & 0xF]};
            src->write(pair, 2);
        }
        src->write(" ", 1);
    }

    void TextStreamFormat::writeDecorationText(StreamSrc *src, const SafeString &text)
    {
        src->write(text.cstr(), (u32) text.calcLength());
    }

    void TextStreamFormat::writeNullChar(StreamSrc *src)
    {
        src->write("", 1);
    }

    void TextStreamFormat::skip(StreamSrc *src, u32 size)
    {
        char token[cTokenSize];
        for (u32 i = 0; i < size; i++) readToken_(src, token, cTokenSize);
    }

    void TextStreamFormat::flush(StreamSrc *src)
    {
        src->flush();
    }

    void TextStreamFormat::rewind(StreamSrc *src)
    {
        src->rewind();
    }

    void Stream::setMode(Modes mode)
    {
        m_format = mode == cText ? (StreamFormat *) &s_textFormat : (StreamFormat *) &s_binaryFormat;
    }

    u8 ReadStream::readU8()
    {
        return m_format->readU8(m_src, m_endian);
    }

    u16 ReadStream::readU16()
    {
        return m_format->readU16(m_src, m_endian);
    }

    u32 ReadStream::readU32()
    {
        return m_format->readU32(m_src, m_endian);
    }

    u64 ReadStream::readU64()
    {
        return m_format->readU64(m_src, m_endian);
    }

    s8 ReadStream::readS8()
    {
        return m_format->readS8(m_src, m_endian);
    }

    s16 ReadStream::readS16()
    {
        return m_format->readS16(m_src, m_endian);
    }

    s32 ReadStream::readS32()
    {
        return m_format->readS32(m_src, m_endian);
    }

    s64 ReadStream::readS64()
    {
        return m_format->readS64(m_src, m_endian);
    }

    f32 ReadStream::readF32()
    {
        return m_format->readF32(m_src, m_endian);
    }

    f64 ReadStream::readF64()
    {
        return m_format->readF64(m_src, m_endian);
    }

    void ReadStream::readBit(void *dst, u32 bitCount)
    {
        m_format->readBit(m_src, dst, bitCount);
    }

    void ReadStream::readString(BufferedSafeString *dst, u32 size)
    {
        m_format->readString(m_src, dst, size);
    }

    u32 ReadStream::readMemBlock(void *dst, u32 size)
    {
        return m_format->readMemBlock(m_src, dst, size);
    }

    void WriteStream::writeU8(u8 value)
    {
        m_format->writeU8(m_src, m_endian, value);
    }

    void WriteStream::writeU16(u16 value)
    {
        m_format->writeU16(m_src, m_endian, value);
    }

    void WriteStream::writeU32(u32 value)
    {
        m_format->writeU32(m_src, m_endian, value);
    }

    void WriteStream::writeU64(u64 value)
    {
        m_format->writeU64(m_src, m_endian, value);
    }

    void WriteStream::writeS8(s8 value)
    {
        m_format->writeS8(m_src, m_endian, value);
    }

    void WriteStream::writeS16(s16 value)
    {
        m_format->writeS16(m_src, m_endian, value);
    }

    void WriteStream::writeS32(s32 value)
    {
        m_format->writeS32(m_src, m_endian, value);
    }

    void WriteStream::writeS64(s64 value)
    {
        m_format->writeS64(m_src, m_endian, value);
    }

    void WriteStream::writeF32(f32 value)
    {
        m_format->writeF32(m_src, m_endian, value);
    }

    void WriteStream::writeF64(f64 value)
    {
        m_format->writeF64(m_src, m_endian, value);
    }

    void WriteStream::writeBit(const void *data, u32 bitCount)
    {
        m_format->writeBit(m_src, data, bitCount);
    }

    void WriteStream::writeString(const SafeString &str, u32 size)
    {
        m_format->writeString(m_src, str, size);
    }

    void WriteStream::writeMemBlock(const void *data, u32 size)
    {
        m_format->writeMemBlock(m_src, data, size);
    }

    void WriteStream::writeComment(const SafeString &comment)
    {
        FixedSafeString<256> text;
        text.format("# %s\n", comment.cstr());
        m_format->writeDecorationText(m_src, text);
    }

    void WriteStream::writeLineComment(const SafeString &comment)
    {
        FixedSafeString<256> text;
        text.format("\n# %s\n", comment.cstr());
        m_format->writeDecorationText(m_src, text);
    }

    void WriteStream::writeDecorationText(const SafeString &text)
    {
        m_format->writeDecorationText(m_src, text);
    }

    void WriteStream::writeNullChar()
    {
        m_format->writeNullChar(m_src);
    }

    void WriteStream::flush()
    {
        m_format->flush(m_src);
    }

    RamWriteStream::RamWriteStream(void *buffer, u32 size, Modes mode) : m_ramSrc(buffer, size)
    {
        m_src = &m_ramSrc;
        setMode(mode);
    }

    RamReadStream::RamReadStream(const void *buffer, u32 size, Modes mode) : m_ramSrc((void *) buffer, size)
    {
        m_src = &m_ramSrc;
        setMode(mode);
    }

} // namespace pead
