/**
 * @file
 * @brief This is MD5 as RFC 1321 defines it.
 */

#pragma once

#include <cstddef>
#include <nn/crypto/HashContextBase.h>
#include <nn/crypto/types.h>

namespace nn::crypto
{

    /**
     * @brief This is MD5 as RFC 1321 defines it.
     */
    class Md5Context : public HashContextBase
    {
    public:
        static constexpr size_t HashSize  = 16;
        static constexpr size_t BlockSize = 64;

        Md5Context()
        {
            Initialize();
        }

        void Initialize() override;
        void Update(const void *data, size_t size) override;
        void Finalize() override;
        size_t GetHashSize() const override
        {
            return HashSize;
        }
        void GetHash(void *out) override;

        /**
         * @brief Gets how many bytes were fed in so far.
         *
         * @return The input length in bytes.
         */
        qUnsignedInt64 GetHashLength() const
        {
            return m_length;
        }

    protected:
        void ProcessBlock() override;

    private:
        qUnsignedInt32 m_state[4]        = {};
        qUnsignedInt8 m_block[BlockSize] = {};
        size_t m_blockUsed               = 0;
        qUnsignedInt64 m_length          = 0;
        qUnsignedInt8 m_hash[HashSize]   = {};
        bool m_finalized                 = false;
    };

    /**
     * @brief Hashes a buffer with MD5 in one call.
     *
     * @param out Receives 16 bytes.
     * @param data The input. It may be null when size is 0.
     * @param size How many bytes data holds.
     */
    void CalculateMd5(void *out, const void *data, size_t size);

} // namespace nn::crypto
