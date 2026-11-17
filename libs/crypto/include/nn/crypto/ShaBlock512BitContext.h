/**
 * @file
 * @brief This holds the 64 byte block buffering and big endian length padding that SHA-1 uses.
 */

#pragma once

#include <cstddef>
#include <nn/crypto/HashContextBase.h>
#include <nn/crypto/types.h>

namespace nn::crypto
{

    /**
     * @brief This holds the 64 byte block buffering and big endian length padding that SHA-1 uses.
     */
    class ShaBlock512BitContext : public HashContextBase
    {
    public:
        void Update(const void *data, size_t size) override;

    protected:
        static constexpr size_t BlockSize = 64;

        void AddPadding();

        /**
         * @brief Resets the buffering to resume a hash whose earlier blocks were already processed.
         *
         * @param length How many bytes the saved state covers. We only keep whole blocks, so the remainder must be fed in again.
         */
        void InitializeWithContext(qUnsignedInt64 length);

        qUnsignedInt8 m_block[BlockSize] = {};
        size_t m_blockUsed               = 0;
        qUnsignedInt64 m_length          = 0;
        bool m_finalized                 = false;
    };

} // namespace nn::crypto
