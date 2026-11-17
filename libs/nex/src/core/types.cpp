#include <nn/nex/types.h>

namespace nn::nex
{

    const DateTime DateTime::Never(0);

    // We count days with the proleptic Gregorian civil calendar so the result matches what time() would have returned
    qUnsignedInt64 DateTime::ToEpochTime() const
    {
        qInt64 y = GetYear(), m = GetMonth(), d = GetDay();
        y -= m <= 2;
        qInt64 era  = (y >= 0 ? y : y - 399) / 400;
        qInt64 yoe  = y - era * 400;
        qInt64 doy  = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
        qInt64 doe  = yoe * 365 + yoe / 4 - yoe / 100 + doy;
        qInt64 days = era * 146097 + doe - 719468;
        return (qUnsignedInt64) (days * 86400 + GetHour() * 3600 + GetMinute() * 60 + GetSecond());
    }

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
