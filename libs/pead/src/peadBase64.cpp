#include <pead/peadBase64.h>

namespace pead
{

    namespace
    {

        constexpr char cTable[]        = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        constexpr char cUrlSafeTable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

        // This accepts both alphabets so decode doesn't need to be told which one was used.
        s32 DecodeChar(char c)
        {
            if (c >= 'A' && c <= 'Z')
                return c - 'A';
            if (c >= 'a' && c <= 'z')
                return c - 'a' + 26;
            if (c >= '0' && c <= '9')
                return c - '0' + 52;
            if (c == '+' || c == '-')
                return 62;
            if (c == '/' || c == '_')
                return 63;
            return -1;
        }

    } // namespace

    void Base64::encode(char *dst, const void *src, u32 srcSize, bool urlSafe)
    {
        const char *table = urlSafe ? cUrlSafeTable : cTable;
        const u8 *in      = (const u8 *) src;
        u32 o             = 0;
        for (u32 i = 0; i < srcSize; i += 3)
        {
            u32 left = srcSize - i;
            u32 v    = (u32) in[i] << 16;
            if (left > 1)
                v |= (u32) in[i + 1] << 8;
            if (left > 2)
                v |= in[i + 2];
            dst[o++] = table[(v >> 18) & 0x3F];
            dst[o++] = table[(v >> 12) & 0x3F];
            dst[o++] = left > 1 ? table[(v >> 6) & 0x3F] : '=';
            dst[o++] = left > 2 ? table[v & 0x3F] : '=';
        }
        dst[o] = '\0';
    }

    bool Base64::decode(void *dst, u32 dstSize, const char *src, u32 srcSize, u32 *decodedSize)
    {
        u8 *out   = (u8 *) dst;
        u32 o     = 0;
        u32 acc   = 0;
        s32 bits  = 0;
        bool done = false;
        for (u32 i = 0; i < srcSize; i++)
        {
            char c = src[i];
            if (c == '=')
            {
                done = true;
                continue;
            }
            s32 v = DecodeChar(c);
            if (v < 0 || done)
                return false;
            acc = (acc << 6) | (u32) v;
            bits += 6;
            if (bits >= 8)
            {
                bits -= 8;
                if (o >= dstSize)
                    return false;
                out[o++] = (u8) (acc >> bits);
            }
        }
        if (decodedSize)
            *decodedSize = o;
        return true;
    }

} // namespace pead
