/**
 * @file
 * @brief This is the interface every hash context shares, so code that only needs "some hash" can take any of them.
 */

#pragma once

#include <cstddef>
#include <nn/crypto/types.h>

namespace nn::crypto
{

    /**
     * @brief This is the interface every hash context shares, so code that only needs "some hash" can take any of them.
     */
    class HashContextBase
    {
    public:
        virtual ~HashContextBase() = default;

        /**
         * @brief Starts a new hash, dropping anything that was fed in before.
         */
        virtual void Initialize() = 0;

        /**
         * @brief Feeds more input into the hash. Calls after Finalize are ignored until the next Initialize.
         *
         * @param data The next part of the input, which can be split at any byte.
         * @param size How many bytes data holds.
         */
        virtual void Update(const void *data, size_t size) = 0;

        /**
         * @brief Pads the input and computes the digest. Calling it twice is harmless.
         */
        virtual void Finalize() = 0;

        /**
         * @brief Gets the digest size in bytes.
         *
         * @return 20 for SHA-1, 16 for MD5.
         */
        virtual size_t GetHashSize() const = 0;

        /**
         * @brief Finalizes if that hasn't happened yet and copies the digest out.
         *
         * @param out Receives GetHashSize() bytes.
         */
        virtual void GetHash(void *out) = 0;

    protected:
        virtual void ProcessBlock() = 0;
    };

} // namespace nn::crypto
