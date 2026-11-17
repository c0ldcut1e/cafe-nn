/**
 * @file
 * @brief This is SHA-1 as FIPS 180-4 defines it.
 */

#pragma once

#include <cstddef>
#include <nn/crypto/ShaBlock512BitContext.h>
#include <nn/crypto/types.h>

namespace nn::crypto
{

    /**
     * @brief This is SHA-1 as FIPS 180-4 defines it.
     */
    class Sha1Context : public ShaBlock512BitContext
    {
    public:
        static constexpr size_t HashSize = 20;

        Sha1Context()
        {
            Initialize();
        }

        void Initialize() override;
        void Finalize() override;
        size_t GetHashSize() const override
        {
            return HashSize;
        }
        void GetHash(void *out) override;

        /**
         * @brief Resumes a hash from a saved intermediate state, such as one a save file stores to avoid rehashing a large prefix.
         *
         * @param state The five chaining words, in host order.
         * @param length How many bytes the state covers. Anything past the last whole block is dropped.
         */
        void InitializeWithContext(const void *state, qUnsignedInt64 length);

    protected:
        void ProcessBlock() override;

    private:
        qUnsignedInt32 m_state[5]      = {};
        qUnsignedInt8 m_hash[HashSize] = {};
    };

    /**
     * @brief Hashes a buffer with SHA-1 in one call.
     *
     * @param out Receives 20 bytes.
     * @param data The input. It may be null when size is 0.
     * @param size How many bytes data holds.
     */
    void CalculateSha1(void *out, const void *data, size_t size);

} // namespace nn::crypto
