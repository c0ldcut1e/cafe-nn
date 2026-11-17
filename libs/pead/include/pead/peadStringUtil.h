/**
 * @file
 * @brief These are pead's string helpers: number parsing, case mapping, UTF-8, UTF-16 and Shift JIS conversion, and printf for char and char16_t.
 */

#pragma once

#include <cstdarg>
#include <pead/peadBuffer.h>
#include <pead/peadSafeString.h>

namespace pead
{
    class StringUtil
    {
    public:
        enum CardinalNumber
        {
            cBaseAuto = -1,
            cBase10   = 10,
            cBase16   = 16,
        };

        /**
         * @brief This is one entry of a sorted character mapping table, such as the case tables.
         */
        struct Char16Pair
        {
            char16_t key;
            char16_t value;
        };

        static s32 compareChar16Pair(const Char16Pair &pair, const char16_t &key);

        /**
         * @brief This maps c through table.
         *
         * @param c The character to map.
         * @param table Pairs sorted by key.
         * @return The mapped character, or c when table has no entry for it.
         */
        static char16_t replace(char16_t c, const Buffer<const Char16Pair> &table);

        /**
         * @brief This lowers ASCII, Latin-1, Greek, Cyrillic and full-width Latin letters.
         */
        static char16_t toLowerCapital(char16_t c);
        static char16_t toUpperCapital(char16_t c);
        static void toLowerCapitalFirstCharactor(WBufferedSafeString *str);
        static void toUpperCapitalFirstCharactor(WBufferedSafeString *str);

        /**
         * @brief This converts UTF-16 to UTF-8, joining surrogate pairs. It stops before a character that would not fit, so the output is always terminated and never holds half a character.
         *
         * @param dst The output buffer.
         * @param dstSize The size of dst in bytes, including the terminator.
         * @param src The input text.
         * @param srcLength The length of src in char16_t units, or -1 when it is terminated.
         * @return The output length in bytes.
         */
        static s32 convertUtf16ToUtf8(char *dst, u32 dstSize, const char16_t *src, s32 srcLength);

        /**
         * @brief This converts UTF-16 to Shift JIS (code page 932). Characters it has no code for become '?'.
         *
         * @param dst The output buffer.
         * @param dstSize The size of dst in bytes, including the terminator.
         * @param src The input text.
         * @param srcLength The length of src in char16_t units, or -1 when it is terminated.
         * @return The output length in bytes.
         */
        static s32 convertUtf16ToSjis(char *dst, u32 dstSize, const char16_t *src, s32 srcLength);
        static s32 convertUtf8ToSjis(char *dst, u32 dstSize, const char *src, s32 srcLength);
        static s32 convertSjisToUtf16(char16_t *dst, u32 dstSize, const char *src, s32 srcLength);
        static s32 convertUtf8ToUtf16(char16_t *dst, u32 dstSize, const char *src, s32 srcLength);

        /**
         * @brief This parses a whole string as an integer. Leading and trailing spaces are allowed, and anything else makes it fail.
         *
         * @param value Receives the number. It is left alone on failure.
         * @param str The text, with an optional sign. With cBaseAuto a 0x prefix means hexadecimal.
         * @param base The base to read in.
         * @return False when the text is not a number or does not fit the type.
         */
        static bool tryParseS8(s8 *value, const SafeString &str, CardinalNumber base);
        static bool tryParseS16(s16 *value, const SafeString &str, CardinalNumber base);
        static bool tryParseS32(s32 *value, const SafeString &str, CardinalNumber base);
        static bool tryParseS64(s64 *value, const SafeString &str, CardinalNumber base);
        static bool tryParseU8(u8 *value, const SafeString &str, CardinalNumber base);
        static bool tryParseU16(u16 *value, const SafeString &str, CardinalNumber base);
        static bool tryParseU32(u32 *value, const SafeString &str, CardinalNumber base);
        static bool tryParseU64(u64 *value, const SafeString &str, CardinalNumber base);
        static bool tryParseF32(f32 *value, const SafeString &str);
        static bool tryParseF64(f64 *value, const SafeString &str);

        /**
         * @brief This parses like tryParseS32 but returns 0 when the text is not a number.
         */
        static s32 parseS32(const SafeString &str, CardinalNumber base);

        /**
         * @brief This is vsnprintf that always terminates dst.
         *
         * @param dst The output buffer.
         * @param dstSize The size of dst, including the terminator.
         * @param format The printf-style format.
         * @param args The arguments.
         * @return The length written, which is cut off at dstSize - 1.
         */
        static s32 vsnprintf(char *dst, u32 dstSize, const char *format, va_list args);
        static s32 snprintf(char *dst, u32 dstSize, const char *format, ...);

        /**
         * @brief This is printf for char16_t. %s and %c take char16_t, %hs takes a char string, and the number conversions match printf.
         *
         * @param dst The output buffer.
         * @param dstSize The size of dst in characters, including the terminator.
         * @param format The format.
         * @param args The arguments.
         * @return The length written, which is cut off at dstSize - 1.
         */
        static s32 vsw16printf(char16_t *dst, u32 dstSize, const char16_t *format, va_list args);
        static s32 sw16printf(char16_t *dst, u32 dstSize, const char16_t *format, ...);
    };

} // namespace pead
