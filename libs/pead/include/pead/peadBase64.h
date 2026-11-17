/**
 * @file
 * @brief This is Base64 with either the standard alphabet or the URL-safe one.
 */

#pragma once

#include <pead/peadTypes.h>

namespace pead
{
    class Base64
    {
    public:
        static u32 calcEncodedSize(u32 srcSize)
        {
            return (srcSize + 2) / 3 * 4;
        }

        /**
         * @brief This encodes src and terminates the output.
         *
         * @param dst The output, at least calcEncodedSize(srcSize) + 1 bytes.
         * @param src The bytes to encode.
         * @param srcSize The size of src in bytes.
         * @param urlSafe True for '-' and '_' in place of '+' and '/'.
         */
        static void encode(char *dst, const void *src, u32 srcSize, bool urlSafe);

        /**
         * @brief This decodes either alphabet and accepts missing padding.
         *
         * @param dst Receives the decoded bytes. It is not terminated.
         * @param dstSize The size of dst in bytes.
         * @param src The text to decode. It doesn't need a terminator.
         * @param srcSize The length of src in characters.
         * @param decodedSize Receives the decoded size. It may be null.
         * @return False when src has a character outside both alphabets or dst is too small.
         */
        static bool decode(void *dst, u32 dstSize, const char *src, u32 srcSize, u32 *decodedSize);
    };

} // namespace pead
