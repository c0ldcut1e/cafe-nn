/**
 * @file
 * @brief This clears key material in a way the compiler can't optimise away.
 */

#pragma once

#include <nn/crypto/types.h>

namespace nn::crypto::detail
{

    inline void Wipe(void *p, size_t size)
    {
        volatile qUnsignedInt8 *b = static_cast<volatile qUnsignedInt8 *>(p);
        for (size_t i = 0; i < size; i++) b[i] = 0;
    }

} // namespace nn::crypto::detail
