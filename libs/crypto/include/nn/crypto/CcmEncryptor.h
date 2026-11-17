/**
 * @file
 * @brief This is CCM encryption over a 16 byte block cipher.
 */

#pragma once

#include <cstddef>
#include <nn/crypto/AuthenticatedEncryptor.h>
#include <nn/crypto/BlockCipher.h>
#include <nn/crypto/CcmMode.h>
#include <nn/crypto/types.h>

namespace nn::crypto
{

    /**
     * @brief This is CCM encryption over a 16 byte block cipher.
     */
    class CcmEncryptor : public AuthenticatedEncryptor
    {
    public:
        ~CcmEncryptor() override
        {
            Finalize();
        }

        /**
         * @brief Binds a cipher and nonce. CCM needs every length up front because they go into the first MAC block.
         *
         * @param cipher A cipher with 16 byte blocks that outlives this mode.
         * @param nonce The nonce, which must never repeat under one key.
         * @param nonceSize 7 to 13 bytes. Shorter nonces allow longer messages.
         * @param adataSize The exact associated data length that will follow.
         * @param pdataSize The exact plaintext length that will follow.
         * @param macSize An even tag length from 4 to 16.
         * @return false when any size is out of range.
         */
        bool Initialize(const BlockCipher *cipher, const void *nonce, size_t nonceSize, qUnsignedInt64 adataSize, qUnsignedInt64 pdataSize,
                        size_t macSize)
        {
            return m_mode.Initialize(cipher, nonce, nonceSize, adataSize, pdataSize, macSize);
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
        size_t UpdatePdata(void *dst, size_t dstSize, const void *src, size_t srcSize) override
        {
            return m_mode.UpdateData(dst, dstSize, src, srcSize, true);
        }
        size_t UpdatePdataFinal(void *, size_t) override
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
