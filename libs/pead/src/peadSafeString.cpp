#include <cstdlib>
#include <cstring>
#include <pead/peadHeapMgr.h>
#include <pead/peadSafeString.h>
#include <pead/peadStringUtil.h>

namespace pead
{

    template<>
    const char SafeStringBase<char>::cNullChar = '\0';
    template<>
    const char SafeStringBase<char>::cLineBreakChar = '\n';
    template<>
    const SafeStringBase<char> SafeStringBase<char>::cEmptyString("");
    template<>
    const char16_t SafeStringBase<char16_t>::cNullChar = u'\0';
    template<>
    const char16_t SafeStringBase<char16_t>::cLineBreakChar = u'\n';
    template<>
    const SafeStringBase<char16_t> SafeStringBase<char16_t>::cEmptyString(u"");

    namespace
    {

        s32 VFormat(char *dst, s32 size, const char *format, va_list args)
        {
            return StringUtil::vsnprintf(dst, (u32) size, format, args);
        }

        s32 VFormat(char16_t *dst, s32 size, const char16_t *format, va_list args)
        {
            return StringUtil::vsw16printf(dst, (u32) size, format, args);
        }

    } // namespace

    template<typename T>
    T SafeStringBase<T>::at(s32 index) const
    {
        const T *str = cstr();
        for (s32 i = 0; i < index; i++)
        {
            if (str[i] == cNullChar)
                return cNullChar;
        }
        return index < 0 ? cNullChar : str[index];
    }

    template<typename T>
    s32 SafeStringBase<T>::calcLength() const
    {
        const T *str = cstr();
        s32 length   = 0;
        while (length < cMaximumLength && str[length] != cNullChar) length++;
        return length;
    }

    template<typename T>
    bool SafeStringBase<T>::isEqual(const SafeStringBase &other) const
    {
        return comparewith(other) == 0;
    }

    template<typename T>
    s32 SafeStringBase<T>::comparewith(const SafeStringBase &other) const
    {
        const T *a = cstr();
        const T *b = other.cstr();
        if (a == b)
            return 0;
        for (s32 i = 0; i < cMaximumLength; i++)
        {
            if (a[i] != b[i])
                return a[i] < b[i] ? -1 : 1;
            if (a[i] == cNullChar)
                return 0;
        }
        return 0;
    }

    template<typename T>
    s32 SafeStringBase<T>::findIndex(const SafeStringBase &token) const
    {
        const T *str    = cstr();
        const T *tok    = token.cstr();
        s32 length      = calcLength();
        s32 tokenLength = token.calcLength();
        for (s32 i = 0; i + tokenLength <= length; i++)
        {
            if (memcmp(str + i, tok, (size_t) tokenLength * sizeof(T)) == 0)
                return i;
        }
        return -1;
    }

    template<typename T>
    s32 SafeStringBase<T>::rfindIndex(const SafeStringBase &token) const
    {
        const T *str    = cstr();
        const T *tok    = token.cstr();
        s32 length      = calcLength();
        s32 tokenLength = token.calcLength();
        for (s32 i = length - tokenLength; i >= 0; i--)
        {
            if (memcmp(str + i, tok, (size_t) tokenLength * sizeof(T)) == 0)
                return i;
        }
        return -1;
    }

    template<typename T>
    bool SafeStringBase<T>::include(T c) const
    {
        const T *str = cstr();
        for (s32 i = 0; i < cMaximumLength && str[i] != cNullChar; i++)
        {
            if (str[i] == c)
                return true;
        }
        return false;
    }

    template<typename T>
    bool SafeStringBase<T>::startsWith(const SafeStringBase &prefix) const
    {
        s32 prefixLength = prefix.calcLength();
        return calcLength() >= prefixLength && memcmp(cstr(), prefix.cstr(), (size_t) prefixLength * sizeof(T)) == 0;
    }

    template<typename T>
    bool SafeStringBase<T>::endsWith(const SafeStringBase &suffix) const
    {
        s32 length       = calcLength();
        s32 suffixLength = suffix.calcLength();
        return length >= suffixLength && memcmp(cstr() + length - suffixLength, suffix.cstr(), (size_t) suffixLength * sizeof(T)) == 0;
    }

    template<typename T>
    SafeStringBase<T> SafeStringBase<T>::getPart(s32 at) const
    {
        s32 length = calcLength();
        if (at < 0)
            at = 0;
        return SafeStringBase(cstr() + (at < length ? at : length));
    }

    template<typename T>
    BufferedSafeStringBase<T>::BufferedSafeStringBase(T *buffer, s32 size) : SafeStringBase<T>(buffer), m_bufferSize(size)
    {
        if (buffer && size > 0)
            buffer[0] = cNullChar;
    }

    template<typename T>
    void BufferedSafeStringBase<T>::assureTerminationImpl_() const
    {
        if (m_bufferSize > 0)
            ((T *) this->m_stringTop)[m_bufferSize - 1] = cNullChar;
    }

    template<typename T>
    s32 BufferedSafeStringBase<T>::copy(const SafeStringBase<T> &src, s32 copyLength)
    {
        return copyAt(0, src, copyLength);
    }

    template<typename T>
    s32 BufferedSafeStringBase<T>::copyAt(s32 at, const SafeStringBase<T> &src, s32 copyLength)
    {
        T *buffer  = getBuffer();
        s32 length = this->calcLength();
        if (at < 0)
            at = length + at + 1;
        if (at < 0 || at > length)
            at = length;
        if (copyLength < 0)
            copyLength = src.calcLength();
        if (at + copyLength >= m_bufferSize)
            copyLength = m_bufferSize - at - 1;
        if (copyLength < 0)
            copyLength = 0;
        memmove(buffer + at, src.cstr(), (size_t) copyLength * sizeof(T));
        // Copying into the middle keeps whatever followed the copied range.
        s32 end = at + copyLength;
        if (end >= length)
            buffer[end] = cNullChar;
        return end > length ? end : length;
    }

    template<typename T>
    s32 BufferedSafeStringBase<T>::append(const SafeStringBase<T> &src, s32 appendLength)
    {
        return copyAt(this->calcLength(), src, appendLength);
    }

    template<typename T>
    s32 BufferedSafeStringBase<T>::append(T c)
    {
        s32 length = this->calcLength();
        if (length + 1 >= m_bufferSize)
            return length;
        T *buffer          = getBuffer();
        buffer[length]     = c;
        buffer[length + 1] = cNullChar;
        return length + 1;
    }

    template<typename T>
    s32 BufferedSafeStringBase<T>::prepend(const SafeStringBase<T> &src, s32 prependLength)
    {
        T *buffer  = getBuffer();
        s32 length = this->calcLength();
        if (prependLength < 0)
            prependLength = src.calcLength();
        if (prependLength >= m_bufferSize)
            prependLength = m_bufferSize - 1;
        s32 keep = length;
        if (prependLength + keep >= m_bufferSize)
            keep = m_bufferSize - 1 - prependLength;
        memmove(buffer + prependLength, buffer, (size_t) keep * sizeof(T));
        memcpy(buffer, src.cstr(), (size_t) prependLength * sizeof(T));
        buffer[prependLength + keep] = cNullChar;
        return prependLength + keep;
    }

    template<typename T>
    s32 BufferedSafeStringBase<T>::format(const T *format, ...)
    {
        va_list args;
        va_start(args, format);
        s32 length = formatV(format, args);
        va_end(args);
        return length;
    }

    template<typename T>
    s32 BufferedSafeStringBase<T>::formatV(const T *format, va_list args)
    {
        return VFormat(getBuffer(), m_bufferSize, format, args);
    }

    template<typename T>
    s32 BufferedSafeStringBase<T>::appendWithFormat(const T *format, ...)
    {
        va_list args;
        va_start(args, format);
        s32 length = appendWithFormatV(format, args);
        va_end(args);
        return length;
    }

    template<typename T>
    s32 BufferedSafeStringBase<T>::appendWithFormatV(const T *format, va_list args)
    {
        s32 length = this->calcLength();
        if (length + 1 >= m_bufferSize)
            return length;
        return length + VFormat(getBuffer() + length, m_bufferSize - length, format, args);
    }

    template<typename T>
    s32 BufferedSafeStringBase<T>::trim(s32 trimLength)
    {
        s32 length = this->calcLength();
        if (trimLength < 0 || trimLength >= length)
            return length;
        getBuffer()[trimLength] = cNullChar;
        return trimLength;
    }

    template<typename T>
    s32 BufferedSafeStringBase<T>::trimMatchedString(const SafeStringBase<T> &suffix)
    {
        if (!this->endsWith(suffix))
            return this->calcLength();
        return trim(this->calcLength() - suffix.calcLength());
    }

    template<typename T>
    s32 BufferedSafeStringBase<T>::replaceString(const SafeStringBase<T> &oldStr, const SafeStringBase<T> &newStr)
    {
        s32 length = 0;
        return replaceStringImpl_(getBuffer(), &length, m_bufferSize, this->cstr(), this->calcLength(), oldStr, newStr, nullptr);
    }

    template<typename T>
    s32 replaceStringImpl_(T *dst, s32 *length, s32 dstSize, const T *src, s32 srcLength, const SafeStringBase<T> &oldStr,
                           const SafeStringBase<T> &newStr, bool *isOverflow)
    {
        s32 oldLength    = oldStr.calcLength();
        s32 newLength    = newStr.calcLength();
        const T *oldText = oldStr.cstr();
        const T *newText = newStr.cstr();
        bool overflow    = false;
        s32 count        = 0;

        // dst may alias src, so we build into a scratch copy first when they share memory.
        T *scratch = nullptr;
        if (dst == src && srcLength > 0)
        {
            scratch = (T *) malloc((size_t) srcLength * sizeof(T));
            if (!scratch)
            {
                *length = srcLength;
                return 0;
            }
            memcpy(scratch, src, (size_t) srcLength * sizeof(T));
            src = scratch;
        }

        s32 out = 0;
        s32 i   = 0;
        while (i < srcLength)
        {
            if (oldLength > 0 && i + oldLength <= srcLength && memcmp(src + i, oldText, (size_t) oldLength * sizeof(T)) == 0)
            {
                count++;
                s32 n = newLength;
                if (out + n >= dstSize)
                {
                    n        = dstSize - 1 - out;
                    overflow = true;
                }
                if (n > 0)
                    memcpy(dst + out, newText, (size_t) n * sizeof(T));
                out += n > 0 ? n : 0;
                i += oldLength;
                continue;
            }
            if (out + 1 >= dstSize)
            {
                overflow = true;
                i++;
                continue;
            }
            dst[out++] = src[i++];
        }
        if (dstSize > 0)
            dst[out] = SafeStringBase<T>::cNullChar;

        ::free(scratch);
        *length = out;
        if (isOverflow)
            *isOverflow = overflow;
        return count;
    }

    template<typename T>
    HeapSafeStringBase<T>::HeapSafeStringBase(Heap *heap, const SafeStringBase<T> &str, s32 alignment) : BufferedSafeStringBase<T>(nullptr, 0)
    {
        s32 size  = str.calcLength() + 1;
        T *buffer = (T *) allocateFromSeadHeap((u32) size * sizeof(T), heap, alignment);
        if (!buffer)
        {
            this->m_stringTop = &SafeStringBase<T>::cNullChar;
            return;
        }
        this->m_stringTop  = buffer;
        this->m_bufferSize = size;
        buffer[0]          = SafeStringBase<T>::cNullChar;
        this->copy(str);
    }

    template<typename T>
    HeapSafeStringBase<T>::~HeapSafeStringBase()
    {
        if (this->m_bufferSize > 0)
            freeToSeadHeap((void *) this->m_stringTop);
    }

    template class SafeStringBase<char>;
    template class SafeStringBase<char16_t>;
    template class BufferedSafeStringBase<char>;
    template class BufferedSafeStringBase<char16_t>;
    template class HeapSafeStringBase<char>;
    template class HeapSafeStringBase<char16_t>;
    template s32 replaceStringImpl_<char>(char *, s32 *, s32, const char *, s32, const SafeString &, const SafeString &, bool *);
    template s32 replaceStringImpl_<char16_t>(char16_t *, s32 *, s32, const char16_t *, s32, const WSafeString &, const WSafeString &, bool *);

} // namespace pead
