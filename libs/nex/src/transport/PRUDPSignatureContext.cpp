#include <nn/nex/crypto/MD5.h>
#include <nn/nex/transport/PRUDPSignatureContext.h>
#include <nn/nex/types.h>

namespace nn::nex
{

    void PRUDPSignatureContext::SetAccessKey(const char *accessKey)
    {
        size_t len = strlen(accessKey);
        MD5::Hash(accessKey, len, accessKeyHash);
        accessKeySum = 0;
        for (size_t i = 0; i < len; i++) accessKeySum += (qUnsignedInt8) accessKey[i];
    }

} // namespace nn::nex
