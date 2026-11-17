#include <cstring>
#include <nn/nex/crypto/MD5.h>
#include <nn/nex/types.h>

namespace nn::nex
{

    namespace
    {

        constexpr qUnsignedInt32 kShift[64] = {7,  12, 17, 22, 7,  12, 17, 22, 7,  12, 17, 22, 7,  12, 17, 22, 5,  9,  14, 20, 5,  9,
                                               14, 20, 5,  9,  14, 20, 5,  9,  14, 20, 4,  11, 16, 23, 4,  11, 16, 23, 4,  11, 16, 23,
                                               4,  11, 16, 23, 6,  10, 15, 21, 6,  10, 15, 21, 6,  10, 15, 21, 6,  10, 15, 21};

        constexpr qUnsignedInt32 kSine[64] = {
                0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501, 0x698098d8, 0x8b44f7af, 0xffff5bb1,
                0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821, 0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453,
                0xd8a1e681, 0xe7d3fbc8, 0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a, 0xfffa3942,
                0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70, 0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
                0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665, 0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d,
                0x85845dd1, 0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391};

        inline qUnsignedInt32 Rotl(qUnsignedInt32 x, qUnsignedInt32 n)
        {
            return (x << n) | (x >> (32 - n));
        }

    } // namespace

    void MD5::Reset()
    {
        m_state[0] = 0x67452301;
        m_state[1] = 0xefcdab89;
        m_state[2] = 0x98badcfe;
        m_state[3] = 0x10325476;
        m_length   = 0;
    }

    // MD5 reads its words little endian, so we assemble them byte by byte to stay correct on the big endian Wii U
    void MD5::Block(const qUnsignedInt8 *p)
    {
        qUnsignedInt32 w[16];
        for (qInt32 i = 0; i < 16; i++) w[i] = p[i * 4] | (p[i * 4 + 1] << 8) | (p[i * 4 + 2] << 16) | ((qUnsignedInt32) p[i * 4 + 3] << 24);

        qUnsignedInt32 a = m_state[0], b = m_state[1], c = m_state[2], d = m_state[3];
        for (qInt32 i = 0; i < 64; i++)
        {
            qUnsignedInt32 f;
            qInt32 g;
            if (i < 16)
            {
                f = (b & c) | (~b & d);
                g = i;
            }
            else if (i < 32)
            {
                f = (d & b) | (~d & c);
                g = (5 * i + 1) & 15;
            }
            else if (i < 48)
            {
                f = b ^ c ^ d;
                g = (3 * i + 5) & 15;
            }
            else
            {
                f = c ^ (b | ~d);
                g = (7 * i) & 15;
            }
            qUnsignedInt32 next = b + Rotl(a + f + kSine[i] + w[g], kShift[i]);
            a                   = d;
            d                   = c;
            c                   = b;
            b                   = next;
        }
        m_state[0] += a;
        m_state[1] += b;
        m_state[2] += c;
        m_state[3] += d;
    }

    void MD5::Update(const void *data, size_t size)
    {
        const auto *p = (const qUnsignedInt8 *) data;
        size_t used   = m_length & 63;
        m_length += size;
        if (used)
        {
            size_t take = 64 - used < size ? 64 - used : size;
            memcpy(m_pending + used, p, take);
            p += take;
            size -= take;
            if (used + take < 64)
                return;
            Block(m_pending);
        }
        for (; size >= 64; p += 64, size -= 64) Block(p);
        memcpy(m_pending, p, size);
    }

    void MD5::Final(qUnsignedInt8 out[16])
    {
        qUnsignedInt64 bits   = m_length * 8;
        qUnsignedInt8 pad[72] = {0x80};
        size_t used           = m_length & 63;
        size_t padLen         = (used < 56 ? 56 : 120) - used;
        for (qInt32 i = 0; i < 8; i++) pad[padLen + i] = (qUnsignedInt8) (bits >> (8 * i));
        Update(pad, padLen + 8);
        for (qInt32 i = 0; i < 4; i++)
            for (qInt32 j = 0; j < 4; j++) out[i * 4 + j] = (qUnsignedInt8) (m_state[i] >> (8 * j));
    }

    void MD5::Hash(const void *data, size_t size, qUnsignedInt8 out[16])
    {
        MD5 md5;
        md5.Update(data, size);
        md5.Final(out);
    }

    void HmacMD5(const void *key, size_t keySize, const void *data, size_t size, qUnsignedInt8 out[16])
    {
        qUnsignedInt8 block[64] = {};
        if (keySize > 64)
            MD5::Hash(key, keySize, block);
        else
            memcpy(block, key, keySize);

        qUnsignedInt8 ipad[64], opad[64];
        for (qInt32 i = 0; i < 64; i++)
        {
            ipad[i] = block[i] ^ 0x36;
            opad[i] = block[i] ^ 0x5c;
        }

        qUnsignedInt8 inner[16];
        MD5 md5;
        md5.Update(ipad, 64);
        md5.Update(data, size);
        md5.Final(inner);

        md5.Reset();
        md5.Update(opad, 64);
        md5.Update(inner, 16);
        md5.Final(out);
    }

} // namespace nn::nex
