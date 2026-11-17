/**
 * @file
 * @brief This is the little-endian byte buffer that every NEX message is read from and written to.
 */

#pragma once

#include <nn/nex/types.h>

namespace nn::nex
{

    /**
     * @brief Reads and writes advance one cursor, and every multi-byte value is little-endian as on the wire.
     */
    class ByteStream
    {
    public:
        virtual ~ByteStream() = default;
        ByteStream()          = default;
        ByteStream(const void *data, size_t size) : m_data((const qByte *) data, (const qByte *) data + size)
        {}

        void Write(const void *data, size_t size)
        {
            m_data.insert(m_data.end(), (const qByte *) data, (const qByte *) data + size);
        }
        void WriteU8(qUnsignedInt8 v)
        {
            m_data.push_back(v);
        }
        void WriteU16(qUnsignedInt16 v);
        void WriteU32(qUnsignedInt32 v);
        void WriteU64(qUnsignedInt64 v);
        void WriteBool(bool v)
        {
            WriteU8(v ? 1 : 0);
        }
        void WriteString(const String &s);
        void WriteBuffer(const Buffer &b);
        void WriteQBuffer(const qBuffer &b);
        void WriteDateTime(const DateTime &d)
        {
            WriteU64(d.GetValue());
        }

        /**
         * @brief Reads past the end return zeros and latch the error, so callers check IsValid once after a whole structure.
         *
         * @param out Receives size bytes. It is zero-filled when the stream runs out.
         * @param size How many bytes to read.
         * @return Whether the stream had enough bytes.
         */
        bool Read(void *out, size_t size);
        qUnsignedInt8 ReadU8();
        qUnsignedInt16 ReadU16();
        qUnsignedInt32 ReadU32();
        qUnsignedInt64 ReadU64();
        bool ReadBool()
        {
            return ReadU8() != 0;
        }
        String ReadString();
        Buffer ReadBuffer();
        qBuffer ReadQBuffer();
        DateTime ReadDateTime()
        {
            return DateTime(ReadU64());
        }

        bool IsValid() const
        {
            return !m_overrun;
        }
        size_t GetRemaining() const
        {
            return m_data.size() - m_cursor;
        }
        size_t GetSize() const
        {
            return m_data.size();
        }
        const qByte *GetData() const
        {
            return m_data.data();
        }
        const qByte *GetCursorPtr() const
        {
            return m_data.data() + m_cursor;
        }
        size_t GetCursor() const
        {
            return m_cursor;
        }
        void SetCursor(size_t cursor)
        {
            m_cursor = cursor <= m_data.size() ? cursor : m_data.size();
        }

        /**
         * @brief Structure headers carry their content size, which we only know after writing the content.
         *
         * @param offset The offset BeginStructure returned.
         * @param v The value to write at offset.
         */
        void PatchU32(size_t offset, qUnsignedInt32 v);
        void SetLength(size_t length)
        {
            m_data.resize(length);
        }
        void Clear()
        {
            m_data.clear();
            m_cursor  = 0;
            m_overrun = false;
        }

    private:
        std::vector<qByte> m_data;
        size_t m_cursor = 0;
        bool m_overrun  = false;
    };

} // namespace nn::nex
