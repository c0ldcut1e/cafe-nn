#include "Wipe.h"
#include <nn/crypto/AuthenticatedDecryptor.h>

namespace nn::crypto
{

    using namespace detail;

    bool AuthenticatedDecryptor::DecryptAndVerify(void *dst, size_t dstSize, const void *src, size_t srcSize, const void *adata, size_t adataSize,
                                                  const void *mac, size_t macSize)
    {
        UpdateAdata(adata, adataSize);
        UpdateAdataFinal();
        bool ok = UpdateCdata(dst, dstSize, src, srcSize) == srcSize;
        if (ok)
            UpdateCdataFinal(static_cast<qUnsignedInt8 *>(dst) + srcSize, dstSize - srcSize);

        qUnsignedInt8 expected[16];
        ok = ok && macSize <= sizeof expected && GenerateMac(expected, macSize);

        // We compare every byte even after a mismatch so the timing doesn't say how much of a forged tag was right
        qUnsignedInt8 diff       = ok ? 0 : 1;
        const qUnsignedInt8 *got = static_cast<const qUnsignedInt8 *>(mac);
        for (size_t i = 0; ok && i < macSize; i++) diff |= expected[i] ^ got[i];
        Wipe(expected, sizeof expected);

        if (diff != 0)
        {
            Wipe(dst, srcSize <= dstSize ? srcSize : dstSize);
            return false;
        }
        return true;
    }

} // namespace nn::crypto
