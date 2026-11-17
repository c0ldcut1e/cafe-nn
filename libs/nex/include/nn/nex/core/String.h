/**
 * @file
 * @brief NEX strings travel as UTF-8, so we convert here instead of in every serializer.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <list>
#include <nn/nex/types.h>
#include <string>
#include <vector>

namespace nn::nex
{

    class String
    {
    public:
        String() = default;
        String(const qChar *s) : m_str(s ? s : L"")
        {}
        String(const char *s)
        {
            Assign(s);
        }

        const qChar *CStr() const
        {
            return m_str.c_str();
        }
        size_t GetLength() const
        {
            return m_str.size();
        }
        bool IsEmpty() const
        {
            return m_str.empty();
        }

        /**
         * @brief NEX strings travel as UTF-8, so we convert here instead of in every serializer.
         */
        std::string ToUtf8() const;
        void Assign(const char *utf8);

        bool operator==(const String &o) const
        {
            return m_str == o.m_str;
        }
        bool operator<(const String &o) const
        {
            return m_str < o.m_str;
        }

    private:
        std::wstring m_str;
    };

} // namespace nn::nex
