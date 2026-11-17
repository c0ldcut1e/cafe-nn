#include <nn/nex/core/DateTime.h>
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

} // namespace nn::nex
