/**
 * @file
 * @brief This is GenerateRandomBytes and the seed source behind it.
 */

#pragma once

#include <cstddef>
#include <nn/crypto/types.h>

namespace nn::crypto
{

    /**
     * @brief Fills a buffer with random bytes. On Linux and Windows these come from the OS. On Wii U we stretch a seed from the system
     * clocks through a SHA-1 based generator.
     *
     * @param out Receives size random bytes.
     * @param size How many bytes to write.
     */
    void GenerateRandomBytes(void *out, size_t size);

    namespace detail
    {
        void GenerateRandomBytesImpl(void *out, size_t size);

        /**
         * @brief Collects seed material for the generator.
         *
         * @param out Receives the seed.
         * @param size How many seed bytes to collect.
         */
        void GetSeed(qUnsignedInt8 *out, size_t size);
    } // namespace detail

} // namespace nn::crypto
