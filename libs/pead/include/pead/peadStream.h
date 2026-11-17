/**
 * @file
 * @brief These are pead's streams. A StreamSrc moves bytes and a StreamFormat decides how values become bytes, so the same ReadStream reads binary or text from a buffer or anything else.
 */

#pragma once

#include <pead/peadSafeString.h>

namespace pead
{
    /**
     * @brief This is where a stream's bytes come from or go to.
     */
    class StreamSrc
    {
    public:
        virtual ~StreamSrc() = default;

        /**
         * @brief This reads up to size bytes.
         *
         * @param dst Receives the bytes. It needs room for size bytes.
         * @param size How many bytes to read.
         * @return How many bytes were read, which is less than size at the end.
         */
        virtual u32 read(void *dst, u32 size)        = 0;
        virtual u32 write(const void *src, u32 size) = 0;
        virtual u32 skip(s32 offset)                 = 0;
        virtual void rewind()                        = 0;
        virtual bool isEOF()                         = 0;
        virtual bool flush()
        {
            return true;
        }
    };

    /**
     * @brief This reads from and writes to a fixed buffer.
     */
    class RamStreamSrc : public StreamSrc
    {
    public:
        RamStreamSrc(void *buffer, u32 size) : m_start((u8 *) buffer), m_size(size), m_position(0)
        {}

        u32 read(void *dst, u32 size) override;
        u32 write(const void *src, u32 size) override;
        u32 skip(s32 offset) override;
        void rewind() override
        {
            m_position = 0;
        }
        bool isEOF() override
        {
            return m_position >= m_size;
        }
        u32 getPosition() const
        {
            return m_position;
        }

    private:
        u8 *m_start;
        u32 m_size;
        u32 m_position;
    };

    class StreamFormat
    {
    public:
        virtual ~StreamFormat() = default;

        virtual u8 readU8(StreamSrc *src, Endian::Types endian)       = 0;
        virtual u16 readU16(StreamSrc *src, Endian::Types endian)     = 0;
        virtual u32 readU32(StreamSrc *src, Endian::Types endian)     = 0;
        virtual u64 readU64(StreamSrc *src, Endian::Types endian)     = 0;
        virtual s8 readS8(StreamSrc *src, Endian::Types endian)       = 0;
        virtual s16 readS16(StreamSrc *src, Endian::Types endian)     = 0;
        virtual s32 readS32(StreamSrc *src, Endian::Types endian)     = 0;
        virtual s64 readS64(StreamSrc *src, Endian::Types endian)     = 0;
        virtual f32 readF32(StreamSrc *src, Endian::Types endian)     = 0;
        virtual f64 readF64(StreamSrc *src, Endian::Types endian)     = 0;
        virtual void readBit(StreamSrc *src, void *dst, u32 bitCount) = 0;

        /**
         * @brief This reads a string. The binary format reads exactly size bytes and the text format reads one token.
         *
         * @param src Where the bytes come from.
         * @param dst Receives the string.
         * @param size The byte count for the binary format.
         */
        virtual void readString(StreamSrc *src, BufferedSafeString *dst, u32 size) = 0;
        virtual u32 readMemBlock(StreamSrc *src, void *dst, u32 size)              = 0;

        virtual void writeU8(StreamSrc *src, Endian::Types endian, u8 value)      = 0;
        virtual void writeU16(StreamSrc *src, Endian::Types endian, u16 value)    = 0;
        virtual void writeU32(StreamSrc *src, Endian::Types endian, u32 value)    = 0;
        virtual void writeU64(StreamSrc *src, Endian::Types endian, u64 value)    = 0;
        virtual void writeS8(StreamSrc *src, Endian::Types endian, s8 value)      = 0;
        virtual void writeS16(StreamSrc *src, Endian::Types endian, s16 value)    = 0;
        virtual void writeS32(StreamSrc *src, Endian::Types endian, s32 value)    = 0;
        virtual void writeS64(StreamSrc *src, Endian::Types endian, s64 value)    = 0;
        virtual void writeF32(StreamSrc *src, Endian::Types endian, f32 value)    = 0;
        virtual void writeF64(StreamSrc *src, Endian::Types endian, f64 value)    = 0;
        virtual void writeBit(StreamSrc *src, const void *data, u32 bitCount)     = 0;
        virtual void writeString(StreamSrc *src, const SafeString &str, u32 size) = 0;
        virtual void writeMemBlock(StreamSrc *src, const void *data, u32 size)    = 0;
        virtual void writeDecorationText(StreamSrc *src, const SafeString &text)  = 0;
        virtual void writeNullChar(StreamSrc *src)                                = 0;
        virtual void skip(StreamSrc *src, u32 size)                               = 0;
        virtual void flush(StreamSrc *src)                                        = 0;
        virtual void rewind(StreamSrc *src)                                       = 0;
    };

    /**
     * @brief This reads and writes values as raw bytes in the stream's byte order.
     */
    class BinaryStreamFormat : public StreamFormat
    {
    public:
        u8 readU8(StreamSrc *src, Endian::Types endian) override;
        u16 readU16(StreamSrc *src, Endian::Types endian) override;
        u32 readU32(StreamSrc *src, Endian::Types endian) override;
        u64 readU64(StreamSrc *src, Endian::Types endian) override;
        s8 readS8(StreamSrc *src, Endian::Types endian) override;
        s16 readS16(StreamSrc *src, Endian::Types endian) override;
        s32 readS32(StreamSrc *src, Endian::Types endian) override;
        s64 readS64(StreamSrc *src, Endian::Types endian) override;
        f32 readF32(StreamSrc *src, Endian::Types endian) override;
        f64 readF64(StreamSrc *src, Endian::Types endian) override;
        void readBit(StreamSrc *src, void *dst, u32 bitCount) override;
        void readString(StreamSrc *src, BufferedSafeString *dst, u32 size) override;
        u32 readMemBlock(StreamSrc *src, void *dst, u32 size) override;
        void writeU8(StreamSrc *src, Endian::Types endian, u8 value) override;
        void writeU16(StreamSrc *src, Endian::Types endian, u16 value) override;
        void writeU32(StreamSrc *src, Endian::Types endian, u32 value) override;
        void writeU64(StreamSrc *src, Endian::Types endian, u64 value) override;
        void writeS8(StreamSrc *src, Endian::Types endian, s8 value) override;
        void writeS16(StreamSrc *src, Endian::Types endian, s16 value) override;
        void writeS32(StreamSrc *src, Endian::Types endian, s32 value) override;
        void writeS64(StreamSrc *src, Endian::Types endian, s64 value) override;
        void writeF32(StreamSrc *src, Endian::Types endian, f32 value) override;
        void writeF64(StreamSrc *src, Endian::Types endian, f64 value) override;
        void writeBit(StreamSrc *src, const void *data, u32 bitCount) override;
        void writeString(StreamSrc *src, const SafeString &str, u32 size) override;
        void writeMemBlock(StreamSrc *src, const void *data, u32 size) override;
        void writeDecorationText(StreamSrc *, const SafeString &) override
        {}
        void writeNullChar(StreamSrc *) override
        {}
        void skip(StreamSrc *src, u32 size) override;
        void flush(StreamSrc *src) override;
        void rewind(StreamSrc *src) override;
    };

    /**
     * @brief This reads and writes values as text separated by spaces and line breaks. Hexadecimal input with a 0x prefix is accepted.
     */
    class TextStreamFormat : public StreamFormat
    {
    public:
        u8 readU8(StreamSrc *src, Endian::Types endian) override;
        u16 readU16(StreamSrc *src, Endian::Types endian) override;
        u32 readU32(StreamSrc *src, Endian::Types endian) override;
        u64 readU64(StreamSrc *src, Endian::Types endian) override;
        s8 readS8(StreamSrc *src, Endian::Types endian) override;
        s16 readS16(StreamSrc *src, Endian::Types endian) override;
        s32 readS32(StreamSrc *src, Endian::Types endian) override;
        s64 readS64(StreamSrc *src, Endian::Types endian) override;
        f32 readF32(StreamSrc *src, Endian::Types endian) override;
        f64 readF64(StreamSrc *src, Endian::Types endian) override;
        void readBit(StreamSrc *src, void *dst, u32 bitCount) override;
        void readString(StreamSrc *src, BufferedSafeString *dst, u32 size) override;
        u32 readMemBlock(StreamSrc *src, void *dst, u32 size) override;
        void writeU8(StreamSrc *src, Endian::Types endian, u8 value) override;
        void writeU16(StreamSrc *src, Endian::Types endian, u16 value) override;
        void writeU32(StreamSrc *src, Endian::Types endian, u32 value) override;
        void writeU64(StreamSrc *src, Endian::Types endian, u64 value) override;
        void writeS8(StreamSrc *src, Endian::Types endian, s8 value) override;
        void writeS16(StreamSrc *src, Endian::Types endian, s16 value) override;
        void writeS32(StreamSrc *src, Endian::Types endian, s32 value) override;
        void writeS64(StreamSrc *src, Endian::Types endian, s64 value) override;
        void writeF32(StreamSrc *src, Endian::Types endian, f32 value) override;
        void writeF64(StreamSrc *src, Endian::Types endian, f64 value) override;
        void writeBit(StreamSrc *src, const void *data, u32 bitCount) override;
        void writeString(StreamSrc *src, const SafeString &str, u32 size) override;
        void writeMemBlock(StreamSrc *src, const void *data, u32 size) override;
        void writeDecorationText(StreamSrc *src, const SafeString &text) override;
        void writeNullChar(StreamSrc *src) override;
        void skip(StreamSrc *src, u32 size) override;
        void flush(StreamSrc *src) override;
        void rewind(StreamSrc *src) override;

    private:
        // This reads the next token into buffer, skipping leading whitespace, and returns its length.
        s32 readToken_(StreamSrc *src, char *buffer, s32 size);
        void writeToken_(StreamSrc *src, const char *text, s32 length);
    };

    class Stream
    {
    public:
        enum Modes
        {
            cBinary = 0,
            cText   = 1,
        };

        Stream() : m_format(nullptr), m_src(nullptr), m_endian(Endian::getHostEndian())
        {}
        explicit Stream(StreamSrc *src) : m_format(nullptr), m_src(src), m_endian(Endian::getHostEndian())
        {}
        virtual ~Stream() = default;

        /**
         * @brief This picks one of the built-in formats.
         */
        void setMode(Modes mode);
        void setUserFormat(StreamFormat *format)
        {
            m_format = format;
        }
        StreamSrc *getSrcStream() const
        {
            return m_src;
        }
        void setSrcStream(StreamSrc *src)
        {
            m_src = src;
        }
        Endian::Types getBinaryEndian() const
        {
            return m_endian;
        }
        void setBinaryEndian(Endian::Types endian)
        {
            m_endian = endian;
        }
        void skip(u32 size)
        {
            m_format->skip(m_src, size);
        }
        void rewind()
        {
            m_format->rewind(m_src);
        }
        bool isEOF() const
        {
            return m_src->isEOF();
        }

    protected:
        StreamFormat *m_format;
        StreamSrc *m_src;
        Endian::Types m_endian;
    };

    class ReadStream : public Stream
    {
    public:
        using Stream::Stream;

        u8 readU8();
        u16 readU16();
        u32 readU32();
        u64 readU64();
        s8 readS8();
        s16 readS16();
        s32 readS32();
        s64 readS64();
        f32 readF32();
        f64 readF64();
        void readBit(void *dst, u32 bitCount);
        void readString(BufferedSafeString *dst, u32 size);
        u32 readMemBlock(void *dst, u32 size);
    };

    class WriteStream : public ReadStream
    {
    public:
        using ReadStream::ReadStream;

        void writeU8(u8 value);
        void writeU16(u16 value);
        void writeU32(u32 value);
        void writeU64(u64 value);
        void writeS8(s8 value);
        void writeS16(s16 value);
        void writeS32(s32 value);
        void writeS64(s64 value);
        void writeF32(f32 value);
        void writeF64(f64 value);
        void writeBit(const void *data, u32 bitCount);
        void writeString(const SafeString &str, u32 size);
        void writeMemBlock(const void *data, u32 size);
        void writeComment(const SafeString &comment);
        void writeLineComment(const SafeString &comment);
        void writeDecorationText(const SafeString &text);
        void writeNullChar();
        void flush();
    };

    /**
     * @brief This is a WriteStream that owns a RamStreamSrc, for writing into a buffer in one object.
     */
    class RamWriteStream : public WriteStream
    {
    public:
        RamWriteStream(void *buffer, u32 size, Modes mode);
        u32 getPosition() const
        {
            return m_ramSrc.getPosition();
        }

    private:
        RamStreamSrc m_ramSrc;
    };

    class RamReadStream : public ReadStream
    {
    public:
        RamReadStream(const void *buffer, u32 size, Modes mode);
        u32 getPosition() const
        {
            return m_ramSrc.getPosition();
        }

    private:
        RamStreamSrc m_ramSrc;
    };

} // namespace pead
