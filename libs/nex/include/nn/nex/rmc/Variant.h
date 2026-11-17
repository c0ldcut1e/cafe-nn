/**
 * @file
 * @brief This holds one value of a few wire types, as MatchmakeParam stores them.
 */

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <nn/nex/core/DateTime.h>
#include <nn/nex/core/String.h>
#include <nn/nex/rmc/Message.h>
#include <nn/nex/types.h>

namespace nn::nex
{

    /**
     * @brief This holds one value of a few wire types, as MatchmakeParam stores them.
     */
    class Variant
    {
    public:
        enum Type
        {
            None         = 0,
            Int64        = 1,
            Double       = 2,
            Bool         = 3,
            StringType   = 4,
            DateTimeType = 5,
            UInt64       = 6
        };

        Variant() = default;
        Variant(const wchar_t *s) : m_type(StringType), m_string(s)
        {}
        Variant(const String &s) : m_type(StringType), m_string(s)
        {}
        Variant(qInt64 v) : m_type(Int64), m_int(v)
        {}
        Variant(qUnsignedInt32 v) : m_type(UInt64), m_int((qInt64) v)
        {}
        Variant(qUnsignedInt64 v) : m_type(UInt64), m_int((qInt64) v)
        {}
        Variant(double v) : m_type(Double), m_double(v)
        {}
        Variant(bool v) : m_type(Bool), m_int(v)
        {}
        Variant(const DateTime &v) : m_type(DateTimeType), m_int((qInt64) v.GetValue())
        {}
        Variant(const Variant &)            = default;
        Variant &operator=(const Variant &) = default;
        ~Variant()                          = default;

        Type GetType() const
        {
            return m_type;
        }
        qUnsignedInt32 GetUInt32Value() const
        {
            return (qUnsignedInt32) m_int;
        }
        qInt64 GetInt64Value() const
        {
            return m_int;
        }
        qUnsignedInt64 GetUInt64Value() const
        {
            return (qUnsignedInt64) m_int;
        }
        double GetDoubleValue() const
        {
            return m_double;
        }
        bool GetBoolValue() const
        {
            return m_int != 0;
        }
        const String &GetStringValue() const
        {
            return m_string;
        }
        DateTime GetDateTimeValue() const
        {
            return DateTime((qUnsignedInt64) m_int);
        }

        void Add(Message &m) const;
        bool Extract(Message &m);

    private:
        Type m_type     = None;
        qInt64 m_int    = 0;
        double m_double = 0;
        String m_string;
    };

} // namespace nn::nex
