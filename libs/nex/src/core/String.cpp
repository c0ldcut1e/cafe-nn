#include <nn/nex/core/String.h>
#include <nn/nex/types.h>

namespace nn::nex
{

    std::string String::ToUtf8() const
    {
        std::string out;
        for (size_t i = 0; i < m_str.size(); i++)
        {
            qUnsignedInt32 c = (qUnsignedInt32) m_str[i];
            // Windows has a 16 bit wchar_t, so characters outside the BMP arrive as surrogate pairs we join here
            if (c >= 0xD800 && c < 0xDC00 && i + 1 < m_str.size() && (qUnsignedInt32) m_str[i + 1] - 0xDC00 < 0x400)
                c = 0x10000 + ((c - 0xD800) << 10) + ((qUnsignedInt32) m_str[++i] - 0xDC00);
            if (c < 0x80)
            {
                out += (char) c;
            }
            else if (c < 0x800)
            {
                out += (char) (0xC0 | (c >> 6));
                out += (char) (0x80 | (c & 0x3F));
            }
            else if (c < 0x10000)
            {
                out += (char) (0xE0 | (c >> 12));
                out += (char) (0x80 | ((c >> 6) & 0x3F));
                out += (char) (0x80 | (c & 0x3F));
            }
            else
            {
                out += (char) (0xF0 | (c >> 18));
                out += (char) (0x80 | ((c >> 12) & 0x3F));
                out += (char) (0x80 | ((c >> 6) & 0x3F));
                out += (char) (0x80 | (c & 0x3F));
            }
        }
        return out;
    }

    void String::Assign(const char *utf8)
    {
        m_str.clear();
        if (!utf8)
            return;
        const auto *p = (const qByte *) utf8;
        while (*p)
        {
            qUnsignedInt32 c = *p++;
            qInt32 extra     = c >= 0xF0 ? 3 : c >= 0xE0 ? 2 : c >= 0xC0 ? 1 : 0;
            if (extra)
                c &= 0x3F >> extra;
            for (; extra && (*p & 0xC0) == 0x80; extra--) c = (c << 6) | (*p++ & 0x3F);
            if constexpr (sizeof(wchar_t) == 2)
            {
                if (c >= 0x10000)
                {
                    m_str += (wchar_t) (0xD800 + ((c - 0x10000) >> 10));
                    c = 0xDC00 + ((c - 0x10000) & 0x3FF);
                }
            }
            m_str += (wchar_t) c;
        }
    }

} // namespace nn::nex
