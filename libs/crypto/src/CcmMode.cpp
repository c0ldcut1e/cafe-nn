#include "Wipe.h"
#include <cstring>
#include <nn/crypto/CcmMode.h>
#include <nn/crypto/CtrMode.h>

namespace nn::crypto
{

    namespace detail
    {

        bool CcmMode::Initialize(const BlockCipher *cipher, const void *nonce, size_t nonceSize, qUnsignedInt64 adataSize, qUnsignedInt64 dataSize,
                                 size_t macSize)
        {
            Finalize();
            if (!cipher || cipher->GetBlockSize() != BlockSize || nonceSize < 7 || nonceSize > 13 || macSize < 4 || macSize > 16 || macSize % 2 != 0)
                return false;

            // L is how many bytes the counter and the message length get, and the length has to fit in it
            size_t lengthSize = 15 - nonceSize;
            if (lengthSize < 8 && dataSize >> (lengthSize * 8) != 0)
                return false;

            m_cipher    = cipher;
            m_nonceSize = nonceSize;
            m_macSize   = macSize;
            m_adataLeft = adataSize;
            m_dataLeft  = dataSize;

            m_counter[0] = (qUnsignedInt8) (lengthSize - 1);
            memcpy(m_counter + 1, nonce, nonceSize);
            m_cipher->Encrypt(m_tagMask, m_counter);
            m_counter[BlockSize - 1] = 1;

            qUnsignedInt8 b0[BlockSize];
            b0[0] = (qUnsignedInt8) ((adataSize ? 0x40 : 0) | ((macSize - 2) / 2) << 3 | (lengthSize - 1));
            memcpy(b0 + 1, nonce, nonceSize);
            for (size_t i = 0; i < lengthSize; i++) b0[BlockSize - 1 - i] = i < 8 ? (qUnsignedInt8) (dataSize >> (i * 8)) : 0;
            m_cipher->Encrypt(m_mac, b0);
            m_macUsed = 0;

            if (adataSize)
            {
                qUnsignedInt8 header[10];
                size_t headerSize;
                if (adataSize < 0xff00)
                {
                    headerSize = 2;
                }
                else if (adataSize <= 0xffffffff)
                {
                    header[0]  = 0xff;
                    header[1]  = 0xfe;
                    headerSize = 6;
                }
                else
                {
                    header[0]  = 0xff;
                    header[1]  = 0xff;
                    headerSize = 10;
                }
                size_t lengthBytes = headerSize == 2 ? 2 : headerSize - 2;
                for (size_t i = 0; i < lengthBytes; i++) header[headerSize - 1 - i] = (qUnsignedInt8) (adataSize >> (i * 8));
                Absorb(header, headerSize);
            }
            m_state = State::Adata;
            return true;
        }

        void CcmMode::Finalize()
        {
            Wipe(m_mac, BlockSize);
            Wipe(m_counter, BlockSize);
            Wipe(m_keyStream, BlockSize);
            Wipe(m_tagMask, BlockSize);
            m_macUsed       = 0;
            m_keyStreamUsed = BlockSize;
            m_cipher        = nullptr;
            m_state         = State::None;
        }

        void CcmMode::Absorb(const qUnsignedInt8 *data, size_t size)
        {
            for (size_t i = 0; i < size; i++)
            {
                m_mac[m_macUsed++] ^= data[i];
                if (m_macUsed == BlockSize)
                {
                    m_cipher->Encrypt(m_mac, m_mac);
                    m_macUsed = 0;
                }
            }
        }

        void CcmMode::Pad()
        {
            if (m_macUsed)
            {
                m_cipher->Encrypt(m_mac, m_mac);
                m_macUsed = 0;
            }
        }

        // More data than Initialize promised poisons the state, since the first block already committed to the lengths
        void CcmMode::UpdateAdata(const void *data, size_t size)
        {
            if (m_state != State::Adata)
                return;
            if (size > m_adataLeft)
            {
                m_state = State::None;
                return;
            }
            Absorb(static_cast<const qUnsignedInt8 *>(data), size);
            m_adataLeft -= size;
        }

        void CcmMode::UpdateAdataFinal()
        {
            if (m_state != State::Adata)
                return;
            if (m_adataLeft)
            {
                m_state = State::None;
                return;
            }
            Pad();
            m_state = State::Data;
        }

        size_t CcmMode::UpdateData(void *dst, size_t dstSize, const void *src, size_t srcSize, bool encrypt)
        {
            if (m_state == State::Adata)
                UpdateAdataFinal();
            if (m_state != State::Data || dstSize < srcSize)
                return 0;
            if (srcSize > m_dataLeft)
            {
                m_state = State::None;
                return 0;
            }

            qUnsignedInt8 *out      = static_cast<qUnsignedInt8 *>(dst);
            const qUnsignedInt8 *in = static_cast<const qUnsignedInt8 *>(src);
            for (size_t i = 0; i < srcSize; i++)
            {
                if (m_keyStreamUsed == BlockSize)
                {
                    m_cipher->Encrypt(m_keyStream, m_counter);
                    CtrMode<BlockSize>::IncrementCounter(m_counter);
                    m_keyStreamUsed = 0;
                }
                qUnsignedInt8 plain = encrypt ? in[i] : (qUnsignedInt8) (in[i] ^ m_keyStream[m_keyStreamUsed]);
                out[i]              = encrypt ? (qUnsignedInt8) (in[i] ^ m_keyStream[m_keyStreamUsed]) : plain;
                m_keyStreamUsed++;
                Absorb(&plain, 1);
            }
            m_dataLeft -= srcSize;
            return srcSize;
        }

        void CcmMode::UpdateDataFinal()
        {
            if (m_state == State::Adata)
                UpdateAdataFinal();
            if (m_state != State::Data)
                return;
            if (m_dataLeft)
            {
                m_state = State::None;
                return;
            }
            Pad();
            m_state = State::Done;
        }

        bool CcmMode::GenerateMac(void *mac, size_t macSize)
        {
            if (m_state == State::Data || m_state == State::Adata)
                UpdateDataFinal();
            if (m_state != State::Done || macSize != m_macSize)
                return false;
            qUnsignedInt8 *out = static_cast<qUnsignedInt8 *>(mac);
            for (size_t i = 0; i < macSize; i++) out[i] = m_mac[i] ^ m_tagMask[i];
            return true;
        }

    } // namespace detail

} // namespace nn::crypto
