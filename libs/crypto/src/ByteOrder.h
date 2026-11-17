/**
 * @file
 * @brief These read and write 32 bit words in a fixed byte order, so the hashes come out the same on the big endian Wii U and little endian PCs.
 */

#pragma once

#include <nn/crypto/types.h>

namespace nn::crypto::detail
{

    inline qUnsignedInt32 Rotl(qUnsignedInt32 x, qUnsignedInt32 n)
    {
        return (x << n) | (x >> (32 - n));
    }

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

} // namespace nn::crypto::detail
