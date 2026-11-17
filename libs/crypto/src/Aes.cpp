#include <cstring>
#include <nn/crypto/Aes.h>

namespace nn::crypto
{

    namespace
    {

        constexpr qUnsignedInt8 Xtime(qUnsignedInt8 x)
        {
            return (qUnsignedInt8) ((x << 1) ^ ((x & 0x80) ? 0x1b : 0));
        }

        constexpr qUnsignedInt8 Multiply(qUnsignedInt8 a, qUnsignedInt8 b)
        {
            qUnsignedInt8 r = 0;
            while (b)
            {
                if (b & 1)
                    r ^= a;
                a = Xtime(a);
                b >>= 1;
            }
            return r;
        }

        struct Tables
        {
            qUnsignedInt8 sbox[256]    = {};
            qUnsignedInt8 invSbox[256] = {};
        };

        // We build the S-boxes from their definition, the GF(2^8) inverse followed by the affine map, instead of pasting the tables
        constexpr Tables MakeTables()
        {
            Tables t;
            for (qInt32 i = 0; i < 256; i++)
            {
                qUnsignedInt8 inverse = 0;
                if (i != 0)
                {
                    // x^254 is the inverse of x in GF(2^8)
                    qUnsignedInt8 x = (qUnsignedInt8) i, r = 1;
                    for (qInt32 e = 254; e; e >>= 1)
                    {
                        if (e & 1)
                            r = Multiply(r, x);
                        x = Multiply(x, x);
                    }
                    inverse = r;
                }
                qUnsignedInt8 s = inverse;
                for (qInt32 k = 1; k <= 4; k++) s ^= (qUnsignedInt8) ((inverse << k) | (inverse >> (8 - k)));
                s ^= 0x63;
                t.sbox[i]    = s;
                t.invSbox[s] = (qUnsignedInt8) i;
            }
            return t;
        }

        constexpr Tables kTables = MakeTables();

        void AddRoundKey(qUnsignedInt8 *state, const qUnsignedInt8 *key)
        {
            for (qInt32 i = 0; i < 16; i++) state[i] ^= key[i];
        }

        // The state is column major, so byte r + 4c is row r of column c and row r shifts left by r columns
        void ShiftRows(qUnsignedInt8 *s)
        {
            qUnsignedInt8 t[16];
            for (qInt32 c = 0; c < 4; c++)
                for (qInt32 r = 0; r < 4; r++) t[r + 4 * c] = s[r + 4 * ((c + r) % 4)];
            memcpy(s, t, 16);
        }

        void InvShiftRows(qUnsignedInt8 *s)
        {
            qUnsignedInt8 t[16];
            for (qInt32 c = 0; c < 4; c++)
                for (qInt32 r = 0; r < 4; r++) t[r + 4 * ((c + r) % 4)] = s[r + 4 * c];
            memcpy(s, t, 16);
        }

    } // namespace

    namespace detail
    {

        template<size_t KeySize>
        void AesImpl<KeySize>::MixColumns(qUnsignedInt8 *s)
        {
            for (qInt32 c = 0; c < 4; c++)
            {
                qUnsignedInt8 *col = s + 4 * c;
                qUnsignedInt8 a0 = col[0], a1 = col[1], a2 = col[2], a3 = col[3];
                col[0] = (qUnsignedInt8) (Xtime(a0) ^ Xtime(a1) ^ a1 ^ a2 ^ a3);
                col[1] = (qUnsignedInt8) (a0 ^ Xtime(a1) ^ Xtime(a2) ^ a2 ^ a3);
                col[2] = (qUnsignedInt8) (a0 ^ a1 ^ Xtime(a2) ^ Xtime(a3) ^ a3);
                col[3] = (qUnsignedInt8) (Xtime(a0) ^ a0 ^ a1 ^ a2 ^ Xtime(a3));
            }
        }

        template<size_t KeySize>
        void AesImpl<KeySize>::InvMixColumns(qUnsignedInt8 *s)
        {
            for (qInt32 c = 0; c < 4; c++)
            {
                qUnsignedInt8 *col = s + 4 * c;
                qUnsignedInt8 a0 = col[0], a1 = col[1], a2 = col[2], a3 = col[3];
                col[0] = Multiply(a0, 14) ^ Multiply(a1, 11) ^ Multiply(a2, 13) ^ Multiply(a3, 9);
                col[1] = Multiply(a0, 9) ^ Multiply(a1, 14) ^ Multiply(a2, 11) ^ Multiply(a3, 13);
                col[2] = Multiply(a0, 13) ^ Multiply(a1, 9) ^ Multiply(a2, 14) ^ Multiply(a3, 11);
                col[3] = Multiply(a0, 11) ^ Multiply(a1, 13) ^ Multiply(a2, 9) ^ Multiply(a3, 14);
            }
        }

        template<size_t KeySize>
        void AesImpl<KeySize>::SetKey(const qUnsignedInt8 *key)
        {
            constexpr size_t nk    = KeySize / 4;
            constexpr size_t words = (Rounds + 1) * 4;
            memcpy(m_roundKeys, key, KeySize);
            qUnsignedInt8 rcon = 1;
            for (size_t i = nk; i < words; i++)
            {
                qUnsignedInt8 t[4];
                memcpy(t, m_roundKeys + (i - 1) * 4, 4);
                if (i % nk == 0)
                {
                    qUnsignedInt8 first = t[0];
                    t[0]                = (qUnsignedInt8) (kTables.sbox[t[1]] ^ rcon);
                    t[1]                = kTables.sbox[t[2]];
                    t[2]                = kTables.sbox[t[3]];
                    t[3]                = kTables.sbox[first];
                    rcon                = Xtime(rcon);
                }
                else if (nk > 6 && i % nk == 4)
                {
                    for (qInt32 k = 0; k < 4; k++) t[k] = kTables.sbox[t[k]];
                }
                for (qInt32 k = 0; k < 4; k++) m_roundKeys[i * 4 + k] = m_roundKeys[(i - nk) * 4 + k] ^ t[k];
            }
        }

        template<size_t KeySize>
        void AesImpl<KeySize>::Encrypt(qUnsignedInt8 *dst, const qUnsignedInt8 *src) const
        {
            qUnsignedInt8 s[16];
            memcpy(s, src, 16);
            AddRoundKey(s, m_roundKeys);
            for (size_t round = 1; round <= Rounds; round++)
            {
                for (qInt32 i = 0; i < 16; i++) s[i] = kTables.sbox[s[i]];
                ShiftRows(s);
                if (round != Rounds)
                    MixColumns(s);
                AddRoundKey(s, m_roundKeys + round * 16);
            }
            memcpy(dst, s, 16);
        }

        template<size_t KeySize>
        void AesImpl<KeySize>::Decrypt(qUnsignedInt8 *dst, const qUnsignedInt8 *src) const
        {
            qUnsignedInt8 s[16];
            memcpy(s, src, 16);
            AddRoundKey(s, m_roundKeys + Rounds * 16);
            for (size_t round = Rounds; round >= 1; round--)
            {
                InvShiftRows(s);
                for (qInt32 i = 0; i < 16; i++) s[i] = kTables.invSbox[s[i]];
                AddRoundKey(s, m_roundKeys + (round - 1) * 16);
                if (round != 1)
                    InvMixColumns(s);
            }
            memcpy(dst, s, 16);
        }

        template class AesImpl<16>;
        template class AesImpl<24>;
        template class AesImpl<32>;

    } // namespace detail

    // We wipe the expanded key so it doesn't linger in freed memory
    template<size_t KeySize>
    Aes<KeySize>::~Aes()
    {
        volatile qUnsignedInt8 *p = reinterpret_cast<volatile qUnsignedInt8 *>(&m_impl);
        for (size_t i = 0; i < sizeof m_impl; i++) p[i] = 0;
    }

    template<size_t KeySize>
    bool Aes<KeySize>::SetKey(const void *key, size_t size)
    {
        if (size != KeySize)
            return false;
        m_impl.SetKey(static_cast<const qUnsignedInt8 *>(key));
        return true;
    }

    template<size_t KeySize>
    void Aes<KeySize>::Encrypt(void *dst, const void *src) const
    {
        m_impl.Encrypt(static_cast<qUnsignedInt8 *>(dst), static_cast<const qUnsignedInt8 *>(src));
    }

    template<size_t KeySize>
    void Aes<KeySize>::Decrypt(void *dst, const void *src) const
    {
        m_impl.Decrypt(static_cast<qUnsignedInt8 *>(dst), static_cast<const qUnsignedInt8 *>(src));
    }

    template class Aes<16>;
    template class Aes<24>;
    template class Aes<32>;

} // namespace nn::crypto
