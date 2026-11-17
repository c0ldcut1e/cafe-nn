#include <cstring>
#include <nn/nex/crypto/RC4.h>
#include <nn/nex/types.h>

namespace nn::nex
{

    void RC4::SetKey(const void *key, size_t size)
    {
        const auto *k = (const qUnsignedInt8 *) key;
        for (qInt32 i = 0; i < 256; i++) m_s[i] = (qUnsignedInt8) i;
        qUnsignedInt8 j = 0;
        for (qInt32 i = 0; i < 256; i++)
        {
            j               = (qUnsignedInt8) (j + m_s[i] + k[i % size]);
            qUnsignedInt8 t = m_s[i];
            m_s[i]          = m_s[j];
            m_s[j]          = t;
        }
        m_i = m_j = 0;
    }

    void RC4::Process(void *data, size_t size)
    {
        auto *p = (qUnsignedInt8 *) data;
        for (size_t n = 0; n < size; n++)
        {
            m_i++;
            m_j             = (qUnsignedInt8) (m_j + m_s[m_i]);
            qUnsignedInt8 t = m_s[m_i];
            m_s[m_i]        = m_s[m_j];
            m_s[m_j]        = t;
            p[n] ^= m_s[(qUnsignedInt8) (m_s[m_i] + m_s[m_j])];
        }
    }

} // namespace nn::nex
