#include <nn/crypto/Decryptor.h>

namespace nn::crypto
{

    size_t Decryptor::Decrypt(void *dst, size_t dstSize, const void *src, size_t srcSize, const void *iv, size_t ivSize, const BlockCipher *cipher)
    {
        if (!Initialize(cipher, iv, ivSize))
            return 0;
        size_t written = Update(dst, dstSize, src, srcSize);
        if (written != srcSize)
        {
            Finalize();
            return 0;
        }
        written += UpdateFinal(static_cast<qUnsignedInt8 *>(dst) + written, dstSize - written);
        Finalize();
        return written;
    }

} // namespace nn::crypto
