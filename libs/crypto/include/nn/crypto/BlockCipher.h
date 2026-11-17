/**
 * @file
 * @brief This is a block cipher that the cipher modes drive one block at a time.
 */

#pragma once

#include <cstddef>
#include <nn/crypto/types.h>

namespace nn::crypto
{

    /**
     * @brief This is a block cipher that the cipher modes drive one block at a time.
     */
    class BlockCipher
    {
    public:
        virtual ~BlockCipher() = default;

        virtual size_t GetBlockSize() const = 0;
        virtual size_t GetKeySize() const   = 0;

        /**
         * @brief Expands a key for both directions.
         *
         * @param key The raw key. We copy what we need, so it can be freed afterwards.
         * @param size Must equal GetKeySize().
         * @return false when the size is wrong, which leaves the old key in place.
         */
        virtual bool SetKey(const void *key, size_t size) = 0;

        /**
         * @brief Encrypts one block. dst and src may be the same buffer.
         *
         * @param dst Receives GetBlockSize() bytes.
         * @param src GetBlockSize() bytes of plaintext.
         */
        virtual void Encrypt(void *dst, const void *src) const = 0;

        /**
         * @brief Decrypts one block. dst and src may be the same buffer.
         *
         * @param dst Receives GetBlockSize() bytes.
         * @param src GetBlockSize() bytes of ciphertext.
         */
        virtual void Decrypt(void *dst, const void *src) const = 0;
    };

} // namespace nn::crypto
