/**
 * @file
 * @brief This is a cipher mode that encrypts, with a one shot helper on top of the streaming calls.
 */

#pragma once

#include <cstddef>
#include <nn/crypto/BlockCipher.h>
#include <nn/crypto/CipherMode.h>
#include <nn/crypto/types.h>

namespace nn::crypto
{

    /**
     * @brief This is a cipher mode that encrypts, with a one shot helper on top of the streaming calls.
     */
    class Encryptor : public CipherMode
    {
    public:
        /**
         * @brief Processes more input. A stream mode writes as many bytes as it reads.
         *
         * @param dst Receives the output.
         * @param dstSize Room in dst, which has to be at least srcSize.
         * @param src The input. It may be the same buffer as dst.
         * @param srcSize How many bytes src holds.
         * @return How many bytes were written, or 0 when dst is too small.
         */
        virtual size_t Update(void *dst, size_t dstSize, const void *src, size_t srcSize) = 0;

        /**
         * @brief Flushes whatever the mode held back.
         *
         * @param dst Receives the output.
         * @param dstSize Room in dst.
         * @return How many bytes were written.
         */
        virtual size_t UpdateFinal(void *dst, size_t dstSize) = 0;

        /**
         * @brief Runs Initialize, Update, UpdateFinal and Finalize in one go.
         *
         * @return How many bytes were written, or 0 when anything didn't fit.
         */
        size_t Encrypt(void *dst, size_t dstSize, const void *src, size_t srcSize, const void *iv, size_t ivSize, const BlockCipher *cipher);
    };

} // namespace nn::crypto
