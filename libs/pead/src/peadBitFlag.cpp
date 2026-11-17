#include <pead/peadBitFlag.h>

namespace pead
{

    s32 BitFlagUtil::countOnBit(u32 x)
    {
        x = x - ((x >> 1) & 0x55555555);
        x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
        x = (x + (x >> 4)) & 0x0F0F0F0F;
        return (s32) ((x * 0x01010101) >> 24);
    }

    s32 BitFlagUtil::findOnBitFromRight(u32 x, s32 num)
    {
        for (s32 bit = 0; bit < 32; bit++)
        {
            if ((x & (1u << bit)) != 0 && --num == 0)
                return bit;
        }
        return -1;
    }

} // namespace pead
