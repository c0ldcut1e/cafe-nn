#include "Wipe.h"
#include <cstring>
#include <nn/crypto/CtrMode.h>

namespace nn::crypto
{

    namespace detail
    {

        template<size_t BlockSize>
        bool CtrMode<BlockSize>::Initialize(const BlockCipher *cipher, const void *iv, size_t ivSize)
        {
            if (!cipher || cipher->GetBlockSize() != BlockSize || ivSize != BlockSize)
                return false;
            m_cipher = cipher;
            memcpy(m_counter, iv, BlockSize);
            m_keyStreamUsed = BlockSize;
            return true;
        }

        template<size_t BlockSize>
        void CtrMode<BlockSize>::Finalize()
        {
            Wipe(m_counter, BlockSize);
            Wipe(m_keyStream, BlockSize);
            m_keyStreamUsed = BlockSize;
            m_cipher        = nullptr;
        }

        template<size_t BlockSize>
        void CtrMode<BlockSize>::IncrementCounter(qUnsignedInt8 *counter)
        {
            for (size_t i = BlockSize; i-- > 0;)
            {
                if (++counter[i] != 0)
                    break;
            }
        }

        // We keep the unused tail of the last key stream block, so callers can feed any split of the input and get the same output
        template<size_t BlockSize>
        void CtrMode<BlockSize>::ProcessBlocks(qUnsignedInt8 *dst, const qUnsignedInt8 *src, size_t size)
        {
            for (size_t i = 0; i < size; i++)
            {
                if (m_keyStreamUsed == BlockSize)
                {
                    m_cipher->Encrypt(m_keyStream, m_counter);
                    IncrementCounter(m_counter);
                    m_keyStreamUsed = 0;
                }
                dst[i] = src[i] ^ m_keyStream[m_keyStreamUsed++];
            }
        }

        template class CtrMode<16>;

    } // namespace detail

} // namespace nn::crypto
