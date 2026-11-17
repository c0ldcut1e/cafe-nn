#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <pead/peadStringUtil.h>

#include "peadSjisTable.inc"

namespace pead
{

    namespace
    {

        s32 FindPair(const SjisPair *table, s32 count, u16 key)
        {
            s32 lo = 0;
            s32 hi = count - 1;
            while (lo <= hi)
            {
                s32 mid = (lo + hi) / 2;
                if (table[mid].key == key)
                    return table[mid].value;
                if (table[mid].key < key)
                    lo = mid + 1;
                else
                    hi = mid - 1;
            }
            return -1;
        }

        s32 TextLength(const char *src, s32 srcLength)
        {
            return srcLength >= 0 ? srcLength : (s32) strlen(src);
        }

        s32 TextLength(const char16_t *src, s32 srcLength)
        {
            if (srcLength >= 0)
                return srcLength;
            s32 length = 0;
            while (src[length]) length++;
            return length;
        }

        // This reads one code point from UTF-16, joining a surrogate pair. A lone surrogate comes back as U+FFFD.
        u32 ReadUtf16(const char16_t *src, s32 length, s32 *i)
        {
            u32 c = src[(*i)++];
            if (c >= 0xD800 && c < 0xDC00 && *i < length && src[*i] >= 0xDC00 && src[*i] < 0xE000)
                return 0x10000 + ((c - 0xD800) << 10) + (src[(*i)++] - 0xDC00);
            if (c >= 0xD800 && c < 0xE000)
                return 0xFFFD;
            return c;
        }

        // This reads one code point from UTF-8. A malformed sequence consumes one byte and comes back as U+FFFD.
        u32 ReadUtf8(const char *src, s32 length, s32 *i)
        {
            const u8 *s = (const u8 *) src;
            u32 c       = s[(*i)++];
            s32 extra;
            u32 min;
            if (c < 0x80)
                return c;
            if ((c & 0xE0) == 0xC0)
            {
                extra = 1;
                min   = 0x80;
                c &= 0x1F;
            }
            else if ((c & 0xF0) == 0xE0)
            {
                extra = 2;
                min   = 0x800;
                c &= 0x0F;
            }
            else if ((c & 0xF8) == 0xF0)
            {
                extra = 3;
                min   = 0x10000;
                c &= 0x07;
            }
            else
            {
                return 0xFFFD;
            }
            if (*i + extra > length)
                return 0xFFFD;
            for (s32 k = 0; k < extra; k++)
            {
                if ((s[*i + k] & 0xC0) != 0x80)
                    return 0xFFFD;
                c = (c << 6) | (s[*i + k] & 0x3F);
            }
            *i += extra;
            return c < min || c > 0x10FFFF || (c >= 0xD800 && c < 0xE000) ? 0xFFFD : c;
        }

        s32 EncodeSjis(u32 c, char *out)
        {
            if (c < 0x80)
            {
                out[0] = (char) c;
                return 1;
            }
            if (c >= 0xFF61 && c <= 0xFF9F)
            {
                out[0] = (char) (c - 0xFF61 + 0xA1);
                return 1;
            }
            s32 code = c <= 0xFFFF ? FindPair(cUnicodeToSjis, (s32) (sizeof(cUnicodeToSjis) / sizeof(SjisPair)), (u16) c) : -1;
            if (code < 0)
            {
                out[0] = '?';
                return 1;
            }
            out[0] = (char) (code >> 8);
            out[1] = (char) code;
            return 2;
        }

        s32 Utf16Units(u32 c)
        {
            return c >= 0x10000 ? 2 : 1;
        }

        void WriteUtf16(u32 c, char16_t *out)
        {
            if (c >= 0x10000)
            {
                c -= 0x10000;
                out[0] = (char16_t) (0xD800 + (c >> 10));
                out[1] = (char16_t) (0xDC00 + (c & 0x3FF));
            }
            else
            {
                out[0] = (char16_t) c;
            }
        }

        bool IsSpace(s32 c)
        {
            return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
        }

        s32 DigitValue(s32 c)
        {
            if (c >= '0' && c <= '9')
                return c - '0';
            if (c >= 'a' && c <= 'f')
                return c - 'a' + 10;
            if (c >= 'A' && c <= 'F')
                return c - 'A' + 10;
            return 99;
        }

        // This reads the whole string as a sign and a magnitude, so each typed parser only has to check its range.
        bool ParseInteger(const SafeString &str, StringUtil::CardinalNumber base, bool *negative, u64 *magnitude)
        {
            const char *s = str.cstr();
            while (IsSpace(*s)) s++;
            *negative = false;
            if (*s == '+' || *s == '-')
                *negative = *s++ == '-';

            u32 radix = base == StringUtil::cBase16 ? 16 : 10;
            if ((base == StringUtil::cBaseAuto || base == StringUtil::cBase16) && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
            {
                radix = 16;
                s += 2;
            }

            u64 value   = 0;
            bool digits = false;
            for (; DigitValue(*s) < (s32) radix; s++)
            {
                u64 next = value * radix + (u64) DigitValue(*s);
                if (value > (~(u64) 0 - (u64) DigitValue(*s)) / radix)
                    return false;
                value  = next;
                digits = true;
            }
            while (IsSpace(*s)) s++;
            if (!digits || *s != '\0')
                return false;
            *magnitude = value;
            return true;
        }

        template<typename T>
        bool ParseSigned(T *value, const SafeString &str, StringUtil::CardinalNumber base, s64 min, s64 max)
        {
            bool negative;
            u64 magnitude;
            if (!ParseInteger(str, base, &negative, &magnitude))
                return false;
            if (negative ? magnitude > (u64) max + 1 : magnitude > (u64) max)
                return false;
            *value = negative ? (T) (min == INT64_MIN && magnitude == (u64) INT64_MAX + 1 ? INT64_MIN : -(s64) magnitude) : (T) magnitude;
            return true;
        }

        template<typename T>
        bool ParseUnsigned(T *value, const SafeString &str, StringUtil::CardinalNumber base, u64 max)
        {
            bool negative;
            u64 magnitude;
            if (!ParseInteger(str, base, &negative, &magnitude) || (negative && magnitude != 0) || magnitude > max)
                return false;
            *value = (T) magnitude;
            return true;
        }

        template<typename T>
        bool ParseFloat(T *value, const SafeString &str)
        {
            const char *s = str.cstr();
            char *end;
            f64 result = strtod(s, &end);
            if (end == s)
                return false;
            while (IsSpace(*end)) end++;
            if (*end != '\0')
                return false;
            *value = (T) result;
            return true;
        }

        // This collects the output of vsw16printf, dropping what does not fit while still counting it.
        struct W16Writer
        {
            char16_t *dst;
            u32 size;
            u32 length;

            void put(char16_t c)
            {
                if (length + 1 < size)
                    dst[length] = c;
                length++;
            }
            void pad(char16_t c, s32 count)
            {
                for (s32 i = 0; i < count; i++) put(c);
            }
        };

    } // namespace

    s32 StringUtil::compareChar16Pair(const Char16Pair &pair, const char16_t &key)
    {
        return (s32) pair.key - (s32) key;
    }

    char16_t StringUtil::replace(char16_t c, const Buffer<const Char16Pair> &table)
    {
        s32 index = table.binarySearch(c, compareChar16Pair);
        return index < 0 ? c : table[index].value;
    }

    char16_t StringUtil::toLowerCapital(char16_t c)
    {
        if ((c >= u'A' && c <= u'Z') || (c >= 0xC0 && c <= 0xDE && c != 0xD7) || (c >= 0x391 && c <= 0x3A9 && c != 0x3A2) ||
            (c >= 0x410 && c <= 0x42F) || (c >= 0xFF21 && c <= 0xFF3A))
            return (char16_t) (c + 0x20);
        if (c >= 0x400 && c <= 0x40F)
            return (char16_t) (c + 0x50);
        return c;
    }

    char16_t StringUtil::toUpperCapital(char16_t c)
    {
        if ((c >= u'a' && c <= u'z') || (c >= 0xE0 && c <= 0xFE && c != 0xF7) || (c >= 0x3B1 && c <= 0x3C9 && c != 0x3C2) ||
            (c >= 0x430 && c <= 0x44F) || (c >= 0xFF41 && c <= 0xFF5A))
            return (char16_t) (c - 0x20);
        if (c >= 0x450 && c <= 0x45F)
            return (char16_t) (c - 0x50);
        return c;
    }

    void StringUtil::toLowerCapitalFirstCharactor(WBufferedSafeString *str)
    {
        if (!str->isEmpty())
            str->getBuffer()[0] = toLowerCapital(str->getBuffer()[0]);
    }

    void StringUtil::toUpperCapitalFirstCharactor(WBufferedSafeString *str)
    {
        if (!str->isEmpty())
            str->getBuffer()[0] = toUpperCapital(str->getBuffer()[0]);
    }

    s32 StringUtil::convertUtf16ToUtf8(char *dst, u32 dstSize, const char16_t *src, s32 srcLength)
    {
        if (dstSize == 0)
            return 0;
        s32 length = TextLength(src, srcLength);
        u32 out    = 0;
        for (s32 i = 0; i < length;)
        {
            u32 c = ReadUtf16(src, length, &i);
            u8 bytes[4];
            u32 n;
            if (c < 0x80)
            {
                bytes[0] = (u8) c;
                n        = 1;
            }
            else if (c < 0x800)
            {
                bytes[0] = (u8) (0xC0 | (c >> 6));
                bytes[1] = (u8) (0x80 | (c & 0x3F));
                n        = 2;
            }
            else if (c < 0x10000)
            {
                bytes[0] = (u8) (0xE0 | (c >> 12));
                bytes[1] = (u8) (0x80 | ((c >> 6) & 0x3F));
                bytes[2] = (u8) (0x80 | (c & 0x3F));
                n        = 3;
            }
            else
            {
                bytes[0] = (u8) (0xF0 | (c >> 18));
                bytes[1] = (u8) (0x80 | ((c >> 12) & 0x3F));
                bytes[2] = (u8) (0x80 | ((c >> 6) & 0x3F));
                bytes[3] = (u8) (0x80 | (c & 0x3F));
                n        = 4;
            }
            if (out + n >= dstSize)
                break;
            memcpy(dst + out, bytes, n);
            out += n;
        }
        dst[out] = '\0';
        return (s32) out;
    }

    s32 StringUtil::convertUtf16ToSjis(char *dst, u32 dstSize, const char16_t *src, s32 srcLength)
    {
        if (dstSize == 0)
            return 0;
        s32 length = TextLength(src, srcLength);
        u32 out    = 0;
        for (s32 i = 0; i < length;)
        {
            char bytes[2];
            u32 n = (u32) EncodeSjis(ReadUtf16(src, length, &i), bytes);
            if (out + n >= dstSize)
                break;
            memcpy(dst + out, bytes, n);
            out += n;
        }
        dst[out] = '\0';
        return (s32) out;
    }

    s32 StringUtil::convertUtf8ToSjis(char *dst, u32 dstSize, const char *src, s32 srcLength)
    {
        if (dstSize == 0)
            return 0;
        s32 length = TextLength(src, srcLength);
        u32 out    = 0;
        for (s32 i = 0; i < length;)
        {
            char bytes[2];
            u32 n = (u32) EncodeSjis(ReadUtf8(src, length, &i), bytes);
            if (out + n >= dstSize)
                break;
            memcpy(dst + out, bytes, n);
            out += n;
        }
        dst[out] = '\0';
        return (s32) out;
    }

    s32 StringUtil::convertSjisToUtf16(char16_t *dst, u32 dstSize, const char *src, s32 srcLength)
    {
        if (dstSize == 0)
            return 0;
        const u8 *s = (const u8 *) src;
        s32 length  = TextLength(src, srcLength);
        u32 out     = 0;
        for (s32 i = 0; i < length && out + 1 < dstSize;)
        {
            u32 b = s[i++];
            u32 c;
            if (b < 0x80)
                c = b;
            else if (b >= 0xA1 && b <= 0xDF)
                c = 0xFF61 + (b - 0xA1);
            else if (i < length)
            {
                s32 found = FindPair(cSjisToUnicode, (s32) (sizeof(cSjisToUnicode) / sizeof(SjisPair)), (u16) ((b << 8) | s[i]));
                i++;
                c = found < 0 ? 0xFFFD : (u32) found;
            }
            else
                c = 0xFFFD;
            dst[out++] = (char16_t) c;
        }
        dst[out] = 0;
        return (s32) out;
    }

    s32 StringUtil::convertUtf8ToUtf16(char16_t *dst, u32 dstSize, const char *src, s32 srcLength)
    {
        if (dstSize == 0)
            return 0;
        s32 length = TextLength(src, srcLength);
        u32 out    = 0;
        for (s32 i = 0; i < length;)
        {
            u32 c = ReadUtf8(src, length, &i);
            u32 n = (u32) Utf16Units(c);
            if (out + n >= dstSize)
                break;
            WriteUtf16(c, dst + out);
            out += n;
        }
        dst[out] = 0;
        return (s32) out;
    }

    bool StringUtil::tryParseS8(s8 *value, const SafeString &str, CardinalNumber base)
    {
        return ParseSigned(value, str, base, INT8_MIN, INT8_MAX);
    }

    bool StringUtil::tryParseS16(s16 *value, const SafeString &str, CardinalNumber base)
    {
        return ParseSigned(value, str, base, INT16_MIN, INT16_MAX);
    }

    bool StringUtil::tryParseS32(s32 *value, const SafeString &str, CardinalNumber base)
    {
        return ParseSigned(value, str, base, INT32_MIN, INT32_MAX);
    }

    bool StringUtil::tryParseS64(s64 *value, const SafeString &str, CardinalNumber base)
    {
        return ParseSigned(value, str, base, INT64_MIN, INT64_MAX);
    }

    bool StringUtil::tryParseU8(u8 *value, const SafeString &str, CardinalNumber base)
    {
        return ParseUnsigned(value, str, base, UINT8_MAX);
    }

    bool StringUtil::tryParseU16(u16 *value, const SafeString &str, CardinalNumber base)
    {
        return ParseUnsigned(value, str, base, UINT16_MAX);
    }

    bool StringUtil::tryParseU32(u32 *value, const SafeString &str, CardinalNumber base)
    {
        return ParseUnsigned(value, str, base, UINT32_MAX);
    }

    bool StringUtil::tryParseU64(u64 *value, const SafeString &str, CardinalNumber base)
    {
        return ParseUnsigned(value, str, base, UINT64_MAX);
    }

    bool StringUtil::tryParseF32(f32 *value, const SafeString &str)
    {
        return ParseFloat(value, str);
    }

    bool StringUtil::tryParseF64(f64 *value, const SafeString &str)
    {
        return ParseFloat(value, str);
    }

    s32 StringUtil::parseS32(const SafeString &str, CardinalNumber base)
    {
        s32 value = 0;
        tryParseS32(&value, str, base);
        return value;
    }

    s32 StringUtil::vsnprintf(char *dst, u32 dstSize, const char *format, va_list args)
    {
        if (dstSize == 0)
            return 0;
        s32 length = ::vsnprintf(dst, dstSize, format, args);
        if (length < 0)
        {
            dst[0] = '\0';
            return 0;
        }
        dst[dstSize - 1] = '\0';
        return length < (s32) dstSize ? length : (s32) dstSize - 1;
    }

    s32 StringUtil::snprintf(char *dst, u32 dstSize, const char *format, ...)
    {
        va_list args;
        va_start(args, format);
        s32 length = vsnprintf(dst, dstSize, format, args);
        va_end(args);
        return length;
    }

    s32 StringUtil::vsw16printf(char16_t *dst, u32 dstSize, const char16_t *format, va_list args)
    {
        if (dstSize == 0)
            return 0;
        W16Writer w = {dst, dstSize, 0};
        va_list ap;
        va_copy(ap, args);

        for (const char16_t *f = format; *f; f++)
        {
            if (*f != u'%')
            {
                w.put(*f);
                continue;
            }

            // We rebuild each conversion as a narrow spec, so the number formats come straight from the C library.
            char spec[32];
            u32 specLength     = 0;
            spec[specLength++] = '%';
            bool leftAlign     = false;
            s32 width          = 0;
            s32 precision      = -1;
            f++;
            while (*f == u'-' || *f == u'+' || *f == u' ' || *f == u'#' || *f == u'0')
            {
                leftAlign = leftAlign || *f == u'-';
                if (specLength < 8)
                    spec[specLength++] = (char) *f;
                f++;
            }
            if (*f == u'*')
            {
                width = va_arg(ap, int);
                if (width < 0)
                {
                    leftAlign = true;
                    width     = -width;
                }
                f++;
            }
            else
            {
                while (*f >= u'0' && *f <= u'9') width = width * 10 + (*f++ - u'0');
            }
            if (*f == u'.')
            {
                f++;
                precision = 0;
                if (*f == u'*')
                {
                    precision = va_arg(ap, int);
                    f++;
                }
                else
                {
                    while (*f >= u'0' && *f <= u'9') precision = precision * 10 + (*f++ - u'0');
                }
            }

            s32 shortCount = 0;
            s32 longCount  = 0;
            bool sizeT     = false;
            for (;; f++)
            {
                if (*f == u'h')
                    shortCount++;
                else if (*f == u'l')
                    longCount++;
                else if (*f == u'z')
                    sizeT = true;
                else
                    break;
            }

            char16_t conv = *f;
            if (conv == 0)
                break;
            if (conv == u'%')
            {
                w.put(u'%');
                continue;
            }
            if (conv == u'n')
                continue;

            if (conv == u's' || conv == u'c')
            {
                char16_t single[2]   = {0, 0};
                const char16_t *wide = nullptr;
                const char *narrow   = nullptr;
                if (conv == u'c')
                {
                    single[0] = (char16_t) va_arg(ap, int);
                    wide      = single;
                }
                else if (shortCount > 0)
                {
                    narrow = va_arg(ap, const char *);
                    if (!narrow)
                        narrow = "(null)";
                }
                else
                {
                    wide = va_arg(ap, const char16_t *);
                    if (!wide)
                        wide = u"(null)";
                }
                s32 length = conv == u'c' ? 1 : narrow ? (s32) strlen(narrow) : TextLength(wide, -1);
                if (conv == u's' && precision >= 0 && precision < length)
                    length = precision;
                if (!leftAlign)
                    w.pad(u' ', width - length);
                for (s32 i = 0; i < length; i++) w.put(narrow ? (char16_t) (u8) narrow[i] : wide[i]);
                if (leftAlign)
                    w.pad(u' ', width - length);
                continue;
            }

            if (width > 0)
                specLength += (u32)::snprintf(spec + specLength, sizeof(spec) - specLength, "%d", (int) width);
            if (precision >= 0)
                specLength += (u32)::snprintf(spec + specLength, sizeof(spec) - specLength, ".%d", (int) precision);

            char text[512];
            bool isInteger = conv == u'd' || conv == u'i' || conv == u'u' || conv == u'x' || conv == u'X' || conv == u'o';
            bool isFloat =
                    conv == u'f' || conv == u'F' || conv == u'e' || conv == u'E' || conv == u'g' || conv == u'G' || conv == u'a' || conv == u'A';
            if (isInteger)
            {
                bool isSigned      = conv == u'd' || conv == u'i';
                spec[specLength++] = 'j';
                spec[specLength++] = (char) conv;
                spec[specLength]   = '\0';
                if (isSigned)
                {
                    intmax_t v = longCount >= 2   ? (intmax_t) va_arg(ap, long long)
                                 : longCount == 1 ? (intmax_t) va_arg(ap, long)
                                 : sizeT          ? (intmax_t) va_arg(ap, ptrdiff_t)
                                                  : (intmax_t) va_arg(ap, int);
                    if (shortCount == 1)
                        v = (short) v;
                    else if (shortCount >= 2)
                        v = (signed char) v;
                    ::snprintf(text, sizeof(text), spec, v);
                }
                else
                {
                    uintmax_t v = longCount >= 2   ? (uintmax_t) va_arg(ap, unsigned long long)
                                  : longCount == 1 ? (uintmax_t) va_arg(ap, unsigned long)
                                  : sizeT          ? (uintmax_t) va_arg(ap, size_t)
                                                   : (uintmax_t) va_arg(ap, unsigned int);
                    if (shortCount == 1)
                        v = (unsigned short) v;
                    else if (shortCount >= 2)
                        v = (unsigned char) v;
                    ::snprintf(text, sizeof(text), spec, v);
                }
            }
            else if (isFloat)
            {
                spec[specLength++] = (char) conv;
                spec[specLength]   = '\0';
                ::snprintf(text, sizeof(text), spec, va_arg(ap, double));
            }
            else if (conv == u'p')
            {
                spec[specLength++] = 'p';
                spec[specLength]   = '\0';
                ::snprintf(text, sizeof(text), spec, va_arg(ap, void *));
            }
            else
            {
                w.put(u'%');
                w.put(conv);
                continue;
            }
            for (const char *t = text; *t; t++) w.put((char16_t) (u8) *t);
        }
        va_end(ap);

        u32 end  = w.length < dstSize ? w.length : dstSize - 1;
        dst[end] = 0;
        return (s32) end;
    }

    s32 StringUtil::sw16printf(char16_t *dst, u32 dstSize, const char16_t *format, ...)
    {
        va_list args;
        va_start(args, format);
        s32 length = vsw16printf(dst, dstSize, format, args);
        va_end(args);
        return length;
    }

} // namespace pead
