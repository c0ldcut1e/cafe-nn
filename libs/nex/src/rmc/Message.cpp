#include <nn/nex/rmc/Message.h>
#include <nn/nex/types.h>

namespace nn::nex
{

    size_t Message::BeginStructure(qUnsignedInt8 version)
    {
        WriteU8(version);
        size_t start = GetSize();
        WriteU32(0);
        return start;
    }

    void Message::EndStructure(size_t start)
    {
        PatchU32(start, (qUnsignedInt32) (GetSize() - start - 4));
    }

    bool Message::BeginExtractStructure(size_t *end, qUnsignedInt8 *version)
    {
        qUnsignedInt8 v     = ReadU8();
        qUnsignedInt32 size = ReadU32();
        if (!IsValid() || size > GetRemaining())
            return false;
        if (version)
            *version = v;
        *end = GetCursor() + size;
        return true;
    }

} // namespace nn::nex
