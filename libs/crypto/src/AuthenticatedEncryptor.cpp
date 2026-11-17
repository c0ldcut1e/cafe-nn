#include <nn/crypto/AuthenticatedEncryptor.h>

namespace nn::crypto
{

    bool AuthenticatedEncryptor::EncryptAndGenerate(void *dst, void *mac, size_t dstSize, const void *src, size_t srcSize, const void *adata,
                                                    size_t adataSize, size_t macSize)
    {
        UpdateAdata(adata, adataSize);
        UpdateAdataFinal();
        if (UpdatePdata(dst, dstSize, src, srcSize) != srcSize)
            return false;
        UpdatePdataFinal(static_cast<qUnsignedInt8 *>(dst) + srcSize, dstSize - srcSize);
        return GenerateMac(mac, macSize);
    }

} // namespace nn::crypto
