#include "ByteOrder.h"
#include <cstring>
#include <nn/crypto/Sha1Context.h>

namespace nn::crypto
{

    using namespace detail;

    void Sha1Context::Initialize()
    {
        m_state[0]  = 0x67452301;
        m_state[1]  = 0xefcdab89;
        m_state[2]  = 0x98badcfe;
        m_state[3]  = 0x10325476;
        m_state[4]  = 0xc3d2e1f0;
        m_length    = 0;
        m_blockUsed = 0;
        m_finalized = false;
    }

    void Sha1Context::InitializeWithContext(const void *state, qUnsignedInt64 length)
    {
        memcpy(m_state, state, sizeof m_state);
        ShaBlock512BitContext::InitializeWithContext(length);
    }

    void Sha1Context::ProcessBlock()
    {
        qUnsignedInt32 w[80];
        for (qInt32 i = 0; i < 16; i++) w[i] = LoadBig(m_block + i * 4);
        for (qInt32 i = 16; i < 80; i++) w[i] = Rotl(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);

        qUnsignedInt32 a = m_state[0], b = m_state[1], c = m_state[2], d = m_state[3], e = m_state[4];
        for (qInt32 i = 0; i < 80; i++)
        {
            qUnsignedInt32 f, k;
            if (i < 20)
            {
                f = (b & c) | (~b & d);
                k = 0x5a827999;
            }
            else if (i < 40)
            {
                f = b ^ c ^ d;
                k = 0x6ed9eba1;
            }
            else if (i < 60)
            {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8f1bbcdc;
            }
            else
            {
                f = b ^ c ^ d;
                k = 0xca62c1d6;
            }
            qUnsignedInt32 t = Rotl(a, 5) + f + e + k + w[i];
            e                = d;
            d                = c;
            c                = Rotl(b, 30);
            b                = a;
            a                = t;
        }
        m_state[0] += a;
        m_state[1] += b;
        m_state[2] += c;
        m_state[3] += d;
        m_state[4] += e;
    }

    void Sha1Context::Finalize()
    {
        if (m_finalized)
            return;
        AddPadding();
        for (qInt32 i = 0; i < 5; i++) StoreBig(m_hash + i * 4, m_state[i]);
        m_finalized = true;
    }

    void Sha1Context::GetHash(void *out)
    {
        Finalize();
        memcpy(out, m_hash, HashSize);
    }

    void CalculateSha1(void *out, const void *data, size_t size)
    {
        Sha1Context context;
        context.Update(data, size);
        context.GetHash(out);
    }

} // namespace nn::crypto
