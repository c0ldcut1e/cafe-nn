#include <nn/crypto/CtrEncryptor.h>

namespace nn::crypto
{

    template<size_t BlockSize>
    size_t CtrEncryptor<BlockSize>::Update(void *dst, size_t dstSize, const void *src, size_t srcSize)
    {
        if (dstSize < srcSize)
            return 0;
        m_mode.ProcessBlocks(static_cast<qUnsignedInt8 *>(dst), static_cast<const qUnsignedInt8 *>(src), srcSize);
        return srcSize;
    }

    template class CtrEncryptor<16>;

} // namespace nn::crypto
