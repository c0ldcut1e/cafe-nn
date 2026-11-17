/**
 * @file
 * @brief This is CCM decryption over a 16 byte block cipher.
 */

#pragma once

#include <cstddef>
#include <nn/crypto/AuthenticatedDecryptor.h>
#include <nn/crypto/BlockCipher.h>
#include <nn/crypto/CcmEncryptor.h>
#include <nn/crypto/CcmMode.h>
#include <nn/crypto/types.h>

namespace nn::crypto
{

    /**
     * @brief This is CCM decryption over a 16 byte block cipher. Initialize takes the same arguments as CcmEncryptor's, with the
     * ciphertext length in place of the plaintext length.
     */
    class CcmDecryptor : public AuthenticatedDecryptor
    {
    public:
        ~CcmDecryptor() override
        {
            Finalize();
        }

        bool Initialize(const BlockCipher *cipher, const void *nonce, size_t nonceSize, qUnsignedInt64 adataSize, qUnsignedInt64 cdataSize,
                        size_t macSize)
        {
            return m_mode.Initialize(cipher, nonce, nonceSize, adataSize, cdataSize, macSize);
        }

        size_t GetIvSize() const override
        {
            return m_mode.GetIvSize();
        }
        size_t GetUnitSize() const override
        {
            return detail::CcmMode::BlockSize;
        }
        size_t GetMacSize() const override
        {
            return m_mode.GetMacSize();
        }
        void Finalize() override
        {
            m_mode.Finalize();
        }
        void UpdateAdata(const void *data, size_t size) override
        {
            m_mode.UpdateAdata(data, size);
        }
        void UpdateAdataFinal() override
        {
            m_mode.UpdateAdataFinal();
        }
        size_t UpdateCdata(void *dst, size_t dstSize, const void *src, size_t srcSize) override
        {
            return m_mode.UpdateData(dst, dstSize, src, srcSize, false);
        }
        size_t UpdateCdataFinal(void *, size_t) override
        {
            m_mode.UpdateDataFinal();
            return 0;
        }
        bool GenerateMac(void *mac, size_t macSize) override
        {
            return m_mode.GenerateMac(mac, macSize);
        }

    private:
        detail::CcmMode m_mode;
    };

} // namespace nn::crypto
