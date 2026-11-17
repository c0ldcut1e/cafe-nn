/**
 * @file
 * @brief These are pead's strings. A SafeString only points at characters someone else owns, and a BufferedSafeString writes into a fixed buffer, so neither one allocates or overruns.
 */

#pragma once

#include <cstdarg>
#include <pead/peadTypes.h>

namespace pead
{
    class Heap;

    template<typename T>
    class SafeStringBase
    {
    public:
        static const T cNullChar;
        static const T cLineBreakChar;
        static const SafeStringBase cEmptyString;

        // We stop measuring at this many characters, so a string missing its terminator can't run off forever.
        static constexpr s32 cMaximumLength = 0x80000;

        /**
         * @brief This walks the characters up to the terminator. The parsers use it to step through a number.
         */
        class iterator
        {
        public:
            explicit iterator(const SafeStringBase *string, s32 index = 0) : m_string(string), m_index(index)
            {}

            T operator*() const
            {
                return m_string->at(m_index);
            }
            iterator &operator++()
            {
                m_index++;
                return *this;
            }
            s32 getIndex() const
            {
                return m_index;
            }
            bool isEnd() const
            {
                return m_string->at(m_index) == cNullChar;
            }

        private:
            const SafeStringBase *m_string;
            s32 m_index;
        };

        SafeStringBase() : m_stringTop(&cNullChar)
        {}
        SafeStringBase(const T *str) : m_stringTop(str ? str : &cNullChar)
        {}
        SafeStringBase(const SafeStringBase &other) : m_stringTop(other.m_stringTop)
        {}
        virtual ~SafeStringBase() = default;

        SafeStringBase &operator=(const SafeStringBase &other)
        {
            m_stringTop = other.m_stringTop;
            return *this;
        }

        const T *cstr() const
        {
            assureTerminationImpl_();
            return m_stringTop;
        }
        iterator begin() const
        {
            return iterator(this, 0);
        }

        T at(s32 index) const;
        T operator[](s32 index) const
        {
            return at(index);
        }

        s32 calcLength() const;
        bool isEmpty() const
        {
            return cstr()[0] == cNullChar;
        }
        bool isEqual(const SafeStringBase &other) const;
        bool operator==(const SafeStringBase &other) const
        {
            return isEqual(other);
        }
        bool operator!=(const SafeStringBase &other) const
        {
            return !isEqual(other);
        }

        /**
         * @brief This compares character values one by one, like strcmp.
         *
         * @param other The string to compare against.
         * @return A negative number, zero or a positive number when this sorts before, equal to or after other.
         */
        s32 comparewith(const SafeStringBase &other) const;

        /**
         * @brief This finds the first occurrence of token.
         *
         * @param token The text to look for.
         * @return The index of the first match, or -1.
         */
        s32 findIndex(const SafeStringBase &token) const;
        s32 rfindIndex(const SafeStringBase &token) const;
        bool include(T c) const;
        bool include(const SafeStringBase &token) const
        {
            return findIndex(token) != -1;
        }
        bool startsWith(const SafeStringBase &prefix) const;
        bool endsWith(const SafeStringBase &suffix) const;

        /**
         * @brief This returns the tail of the string, sharing its characters.
         *
         * @param at The index the tail starts at. It is clamped to the length.
         * @return The tail, valid as long as this string is.
         */
        SafeStringBase getPart(s32 at) const;

    protected:
        virtual void assureTerminationImpl_() const
        {}

        const T *m_stringTop;
    };

    template<typename T>
    class BufferedSafeStringBase : public SafeStringBase<T>
    {
    public:
        using SafeStringBase<T>::cNullChar;

        /**
         * @brief This wraps a buffer and terminates it as an empty string.
         *
         * @param buffer The buffer, which has to outlive the string.
         * @param size The buffer size in characters, including the terminator.
         */
        BufferedSafeStringBase(T *buffer, s32 size);
        BufferedSafeStringBase(const BufferedSafeStringBase &) = delete;

        BufferedSafeStringBase &operator=(const SafeStringBase<T> &other)
        {
            copy(other);
            return *this;
        }

        T *getBuffer()
        {
            return (T *) this->m_stringTop;
        }
        s32 getBufferSize() const
        {
            return m_bufferSize;
        }

        /**
         * @brief This copies src in, cutting it off at the buffer size.
         *
         * @param src The text to copy. It may overlap the buffer.
         * @param copyLength How many characters to copy, or -1 for all of them.
         * @return The resulting length.
         */
        s32 copy(const SafeStringBase<T> &src, s32 copyLength = -1);
        s32 copyAt(s32 at, const SafeStringBase<T> &src, s32 copyLength = -1);
        s32 append(const SafeStringBase<T> &src, s32 appendLength = -1);
        s32 append(T c);
        s32 prepend(const SafeStringBase<T> &src, s32 prependLength = -1);

        /**
         * @brief This formats into the buffer like snprintf. The char16_t version takes a char16_t format and reads %s arguments as char16_t strings.
         *
         * @param format The printf-style format.
         * @return The resulting length, which is cut off at the buffer size.
         */
        s32 format(const T *format, ...);
        s32 formatV(const T *format, va_list args);
        s32 appendWithFormat(const T *format, ...);
        s32 appendWithFormatV(const T *format, va_list args);

        void clear()
        {
            getBuffer()[0] = cNullChar;
        }

        /**
         * @brief This cuts the string to trimLength characters.
         *
         * @param trimLength The new length. Longer than the string means no change.
         * @return The resulting length.
         */
        s32 trim(s32 trimLength);
        s32 trimMatchedString(const SafeStringBase<T> &suffix);
        s32 removeSuffix(const SafeStringBase<T> &suffix)
        {
            return trimMatchedString(suffix);
        }

        /**
         * @brief This replaces every occurrence of oldStr with newStr.
         *
         * @param oldStr The text to find. An empty string replaces nothing.
         * @param newStr The replacement.
         * @return How many occurrences were replaced, including any cut off at the end of the buffer.
         */
        s32 replaceString(const SafeStringBase<T> &oldStr, const SafeStringBase<T> &newStr);

    protected:
        void assureTerminationImpl_() const override;

        s32 m_bufferSize;
    };

    template<typename T, s32 N>
    class FixedSafeStringBase : public BufferedSafeStringBase<T>
    {
    public:
        FixedSafeStringBase() : BufferedSafeStringBase<T>(m_buffer, N)
        {}
        explicit FixedSafeStringBase(const SafeStringBase<T> &str) : BufferedSafeStringBase<T>(m_buffer, N)
        {
            this->copy(str);
        }
        FixedSafeStringBase(const FixedSafeStringBase &other) : BufferedSafeStringBase<T>(m_buffer, N)
        {
            this->copy(other);
        }
        FixedSafeStringBase &operator=(const SafeStringBase<T> &other)
        {
            this->copy(other);
            return *this;
        }
        FixedSafeStringBase &operator=(const FixedSafeStringBase &other)
        {
            this->copy(other);
            return *this;
        }

    private:
        T m_buffer[N];
    };

    /**
     * @brief This is a BufferedSafeString whose buffer comes from a heap, sized to fit the string it was made from.
     */
    template<typename T>
    class HeapSafeStringBase : public BufferedSafeStringBase<T>
    {
    public:
        /**
         * @brief This allocates a buffer one character longer than str and copies str into it.
         *
         * @param heap The heap to allocate from, or null for the current one.
         * @param str The text to copy.
         * @param alignment The buffer alignment.
         */
        HeapSafeStringBase(Heap *heap, const SafeStringBase<T> &str, s32 alignment = 4);
        ~HeapSafeStringBase() override;
    };

    using SafeString          = SafeStringBase<char>;
    using WSafeString         = SafeStringBase<char16_t>;
    using BufferedSafeString  = BufferedSafeStringBase<char>;
    using WBufferedSafeString = BufferedSafeStringBase<char16_t>;
    using HeapSafeString      = HeapSafeStringBase<char>;
    using WHeapSafeString     = HeapSafeStringBase<char16_t>;
    template<s32 N>
    using FixedSafeString = FixedSafeStringBase<char, N>;
    template<s32 N>
    using WFixedSafeString = FixedSafeStringBase<char16_t, N>;

    /**
     * @brief This replaces oldStr with newStr while copying src into dst. dst may be the same buffer as src.
     *
     * @param dst The output buffer.
     * @param length Receives the output length.
     * @param dstSize The size of dst in characters, including the terminator.
     * @param src The input text.
     * @param srcLength The length of src.
     * @param oldStr The text to find.
     * @param newStr The replacement.
     * @param isOverflow Receives whether the output was cut off. It may be null.
     * @return How many occurrences were replaced.
     */
    template<typename T>
    s32 replaceStringImpl_(T *dst, s32 *length, s32 dstSize, const T *src, s32 srcLength, const SafeStringBase<T> &oldStr,
                           const SafeStringBase<T> &newStr, bool *isOverflow);

} // namespace pead
