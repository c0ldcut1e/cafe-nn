/**
 * @file
 * @brief This is a cipher mode that decrypts.
 */

#pragma once

#include <cstddef>
#include <nn/crypto/BlockCipher.h>
#include <nn/crypto/CipherMode.h>
#include <nn/crypto/Encryptor.h>
#include <nn/crypto/types.h>

namespace nn::crypto
{

    /**
     * @brief This is a cipher mode that decrypts. The streaming calls mean the same as Encryptor's.
     */
    class Decryptor : public CipherMode
    {
    public:
        virtual size_t Update(void *dst, size_t dstSize, const void *src, size_t srcSize) = 0;
        virtual size_t UpdateFinal(void *dst, size_t dstSize)                             = 0;

        /**
         * @brief Runs Initialize, Update, UpdateFinal and Finalize in one go.
         *
         * @return How many bytes were written, or 0 when anything didn't fit.
         */
        size_t Decrypt(void *dst, size_t dstSize, const void *src, size_t srcSize, const void *iv, size_t ivSize, const BlockCipher *cipher);
    };

} // namespace nn::crypto
