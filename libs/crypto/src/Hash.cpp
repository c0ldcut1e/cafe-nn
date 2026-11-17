#include <cstring>
#include <nn/crypto.h>

namespace nn::crypto
{

    namespace
    {

        inline qUnsignedInt32 Rotl(qUnsignedInt32 x, qUnsignedInt32 n)
        {
            return (x << n) | (x >> (32 - n));
        }

        // Both hashes read and write their words byte by byte, so they come out the same on the big endian Wii U and little endian PCs
        inline qUnsignedInt32 LoadBig(const qUnsignedInt8 *p)
        {
            return (qUnsignedInt32) p[0] << 24 | (qUnsignedInt32) p[1] << 16 | (qUnsignedInt32) p[2] << 8 | p[3];
        }

        inline void StoreBig(qUnsignedInt8 *p, qUnsignedInt32 v)
        {
            p[0] = (qUnsignedInt8) (v >> 24);
            p[1] = (qUnsignedInt8) (v >> 16);
            p[2] = (qUnsignedInt8) (v >> 8);
            p[3] = (qUnsignedInt8) v;
        }

        inline qUnsignedInt32 LoadLittle(const qUnsignedInt8 *p)
        {
            return (qUnsignedInt32) p[3] << 24 | (qUnsignedInt32) p[2] << 16 | (qUnsignedInt32) p[1] << 8 | p[0];
        }

        inline void StoreLittle(qUnsignedInt8 *p, qUnsignedInt32 v)
        {
            p[0] = (qUnsignedInt8) v;
            p[1] = (qUnsignedInt8) (v >> 8);
            p[2] = (qUnsignedInt8) (v >> 16);
            p[3] = (qUnsignedInt8) (v >> 24);
        }

        constexpr qUnsignedInt32 kMd5Shift[64] = {7,  12, 17, 22, 7,  12, 17, 22, 7,  12, 17, 22, 7,  12, 17, 22, 5,  9,  14, 20, 5,  9,
                                                  14, 20, 5,  9,  14, 20, 5,  9,  14, 20, 4,  11, 16, 23, 4,  11, 16, 23, 4,  11, 16, 23,
                                                  4,  11, 16, 23, 6,  10, 15, 21, 6,  10, 15, 21, 6,  10, 15, 21, 6,  10, 15, 21};

        constexpr qUnsignedInt32 kMd5Sine[64] = {
                0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501, 0x698098d8, 0x8b44f7af, 0xffff5bb1,
                0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821, 0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453,
                0xd8a1e681, 0xe7d3fbc8, 0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a, 0xfffa3942,
                0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70, 0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
                0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665, 0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d,
                0x85845dd1, 0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391};

    } // namespace

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

    void Md5Context::Initialize()
    {
        m_state[0]  = 0x67452301;
        m_state[1]  = 0xefcdab89;
        m_state[2]  = 0x98badcfe;
        m_state[3]  = 0x10325476;
        m_length    = 0;
        m_blockUsed = 0;
        m_finalized = false;
    }

    void Md5Context::Update(const void *data, size_t size)
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

    void Md5Context::ProcessBlock()
    {
        qUnsignedInt32 m[16];
        for (qInt32 i = 0; i < 16; i++) m[i] = LoadLittle(m_block + i * 4);

        qUnsignedInt32 a = m_state[0], b = m_state[1], c = m_state[2], d = m_state[3];
        for (qUnsignedInt32 i = 0; i < 64; i++)
        {
            qUnsignedInt32 f, g;
            if (i < 16)
            {
                f = (b & c) | (~b & d);
                g = i;
            }
            else if (i < 32)
            {
                f = (d & b) | (~d & c);
                g = (5 * i + 1) % 16;
            }
            else if (i < 48)
            {
                f = b ^ c ^ d;
                g = (3 * i + 5) % 16;
            }
            else
            {
                f = c ^ (b | ~d);
                g = (7 * i) % 16;
            }
            qUnsignedInt32 t = d;
            d                = c;
            c                = b;
            b                = b + Rotl(a + f + kMd5Sine[i] + m[g], kMd5Shift[i]);
            a                = t;
        }
        m_state[0] += a;
        m_state[1] += b;
        m_state[2] += c;
        m_state[3] += d;
    }

    void Md5Context::Finalize()
    {
        if (m_finalized)
            return;
        qUnsignedInt64 bits    = m_length * 8;
        m_block[m_blockUsed++] = 0x80;
        if (m_blockUsed > BlockSize - 8)
        {
            memset(m_block + m_blockUsed, 0, BlockSize - m_blockUsed);
            ProcessBlock();
            m_blockUsed = 0;
        }
        memset(m_block + m_blockUsed, 0, BlockSize - 8 - m_blockUsed);
        StoreLittle(m_block + 56, (qUnsignedInt32) bits);
        StoreLittle(m_block + 60, (qUnsignedInt32) (bits >> 32));
        ProcessBlock();
        m_blockUsed = 0;
        for (qInt32 i = 0; i < 4; i++) StoreLittle(m_hash + i * 4, m_state[i]);
        m_finalized = true;
    }

    void Md5Context::GetHash(void *out)
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

    void CalculateMd5(void *out, const void *data, size_t size)
    {
        Md5Context context;
        context.Update(data, size);
        context.GetHash(out);
    }

} // namespace nn::crypto
