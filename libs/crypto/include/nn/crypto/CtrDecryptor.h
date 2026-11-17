/**
 * @file
 * @brief This is CTR mode decryption, which is the same operation as encryption.
 */

#pragma once

#include <cstddef>
#include <nn/crypto/BlockCipher.h>
#include <nn/crypto/CtrMode.h>
#include <nn/crypto/Decryptor.h>
#include <nn/crypto/types.h>

namespace nn::crypto
{

    /**
     * @brief This is CTR mode decryption, which is the same operation as encryption.
     */
    template<size_t BlockSize>
    class CtrDecryptor : public Decryptor
    {
    public:
        ~CtrDecryptor() override
        {
            Finalize();
        }

        size_t GetIvSize() const override
        {
            return BlockSize;
        }
        size_t GetUnitSize() const override
        {
            return BlockSize;
        }
        bool Initialize(const BlockCipher *cipher, const void *iv, size_t ivSize) override
        {
            return m_mode.Initialize(cipher, iv, ivSize);
        }
        void Finalize() override
        {
            m_mode.Finalize();
        }
        size_t Update(void *dst, size_t dstSize, const void *src, size_t srcSize) override;
        size_t UpdateFinal(void *, size_t) override
        {
            return 0;
        }

    private:
        detail::CtrMode<BlockSize> m_mode;
    };

} // namespace nn::crypto
