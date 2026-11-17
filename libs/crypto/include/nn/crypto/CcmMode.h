/**
 * @file
 * @brief This is the CCM state CcmEncryptor and CcmDecryptor share.
 */

#pragma once

#include <cstddef>
#include <nn/crypto/BlockCipher.h>
#include <nn/crypto/types.h>

namespace nn::crypto
{

    namespace detail
    {
        /**
         * @brief This is the CBC-MAC and counter logic of CCM as SP 800-38C defines it, which both directions share.
         */
        class CcmMode
        {
        public:
            static constexpr size_t BlockSize = 16;

            bool Initialize(const BlockCipher *cipher, const void *nonce, size_t nonceSize, qUnsignedInt64 adataSize, qUnsignedInt64 dataSize,
                            size_t macSize);
            void Finalize();

            void UpdateAdata(const void *data, size_t size);
            void UpdateAdataFinal();

            // encrypt says whether src is plaintext, since the MAC always covers the plaintext
            size_t UpdateData(void *dst, size_t dstSize, const void *src, size_t srcSize, bool encrypt);
            void UpdateDataFinal();
            bool GenerateMac(void *mac, size_t macSize);

            size_t GetIvSize() const
            {
                return m_nonceSize;
            }
            size_t GetMacSize() const
            {
                return m_macSize;
            }

        private:
            enum class State
            {
                None,
                Adata,
                Data,
                Done,
            };

            void Absorb(const qUnsignedInt8 *data, size_t size);
            void Pad();

            const BlockCipher *m_cipher          = nullptr;
            State m_state                        = State::None;
            qUnsignedInt8 m_mac[BlockSize]       = {};
            size_t m_macUsed                     = 0;
            qUnsignedInt8 m_counter[BlockSize]   = {};
            qUnsignedInt8 m_keyStream[BlockSize] = {};
            size_t m_keyStreamUsed               = BlockSize;
            qUnsignedInt8 m_tagMask[BlockSize]   = {};
            qUnsignedInt64 m_adataLeft           = 0;
            qUnsignedInt64 m_dataLeft            = 0;
            size_t m_nonceSize                   = 0;
            size_t m_macSize                     = 0;
        };
    } // namespace detail

} // namespace nn::crypto
