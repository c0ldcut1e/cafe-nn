/**
 * @file
 * @brief This is a pointer and a length, which pead passes in place of a raw array.
 */

#pragma once

#include <pead/peadTypes.h>

namespace pead
{
    template<typename T>
    class Buffer
    {
    public:
        Buffer() : m_size(0), m_buffer(nullptr)
        {}
        Buffer(s32 size, T *buffer) : m_size(size), m_buffer(buffer)
        {}
        template<s32 N>
        Buffer(T (&array)[N]) : m_size(N), m_buffer(array)
        {}

        s32 size() const
        {
            return m_size;
        }
        T *getBufferPtr() const
        {
            return m_buffer;
        }
        bool isBufferReady() const
        {
            return m_buffer != nullptr;
        }
        T &operator[](s32 index) const
        {
            return m_buffer[index];
        }
        T *begin() const
        {
            return m_buffer;
        }
        T *end() const
        {
            return m_buffer + m_size;
        }

        /**
         * @brief This finds key in a buffer sorted by cmp.
         *
         * @param key The value to look for.
         * @param cmp Returns a negative number, zero or a positive number when the element sorts before, equal to or after key.
         * @return The index of a matching element, or -1.
         */
        template<typename K>
        s32 binarySearch(const K &key, s32 (*cmp)(const T &, const K &)) const
        {
            s32 lo = 0;
            s32 hi = m_size - 1;
            while (lo <= hi)
            {
                s32 mid = (lo + hi) / 2;
                s32 c   = cmp(m_buffer[mid], key);
                if (c == 0)
                    return mid;
                if (c < 0)
                    lo = mid + 1;
                else
                    hi = mid - 1;
            }
            return -1;
        }

    private:
        s32 m_size;
        T *m_buffer;
    };

} // namespace pead
