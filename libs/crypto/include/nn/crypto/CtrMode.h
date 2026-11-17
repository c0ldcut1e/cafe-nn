/**
 * @file
 * @brief This is the counter mode state CtrEncryptor and CtrDecryptor share.
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
         * @brief This is counter mode as SP 800-38A defines it, with the whole block as a big endian counter.
         */
        template<size_t BlockSize>
        class CtrMode
        {
        public:
            bool Initialize(const BlockCipher *cipher, const void *iv, size_t ivSize);
            void Finalize();
            void ProcessBlocks(qUnsignedInt8 *dst, const qUnsignedInt8 *src, size_t size);

            static void IncrementCounter(qUnsignedInt8 *counter);

        private:
            const BlockCipher *m_cipher          = nullptr;
            qUnsignedInt8 m_counter[BlockSize]   = {};
            qUnsignedInt8 m_keyStream[BlockSize] = {};
            size_t m_keyStreamUsed               = BlockSize;
        };
    } // namespace detail

} // namespace nn::crypto
