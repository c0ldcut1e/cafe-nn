#include "ByteOrder.h"
#include <cstring>
#include <nn/crypto/ShaBlock512BitContext.h>

namespace nn::crypto
{

    using namespace detail;

    void ShaBlock512BitContext::Update(const void *data, size_t size)
    {
        if (m_finalized)
            return;
        const qUnsignedInt8 *p = static_cast<const qUnsignedInt8 *>(data);
        m_length += size;
        while (size > 0)
        {
            size_t n = BlockSize - m_blockUsed;
            if (n > size)
                n = size;
            memcpy(m_block + m_blockUsed, p, n);
            m_blockUsed += n;
            p += n;
            size -= n;
            if (m_blockUsed == BlockSize)
            {
                ProcessBlock();
                m_blockUsed = 0;
            }
        }
    }

    void ShaBlock512BitContext::AddPadding()
    {
        qUnsignedInt64 bits    = m_length * 8;
        m_block[m_blockUsed++] = 0x80;
        if (m_blockUsed > BlockSize - 8)
        {
            memset(m_block + m_blockUsed, 0, BlockSize - m_blockUsed);
            ProcessBlock();
            m_blockUsed = 0;
        }
        memset(m_block + m_blockUsed, 0, BlockSize - 8 - m_blockUsed);
        StoreBig(m_block + 56, (qUnsignedInt32) (bits >> 32));
        StoreBig(m_block + 60, (qUnsignedInt32) bits);
        ProcessBlock();
        m_blockUsed = 0;
    }

    void ShaBlock512BitContext::InitializeWithContext(qUnsignedInt64 length)
    {
        m_length    = length - length % BlockSize;
        m_blockUsed = 0;
        m_finalized = false;
    }

} // namespace nn::crypto
