#include <nn/nex/rmc/Message.h>
#include <nn/nex/rmc/Variant.h>
#include <nn/nex/types.h>

namespace nn::nex
{

    void Variant::Add(Message &m) const
    {
        m.WriteU8((qUnsignedInt8) m_type);
        switch (m_type)
        {
            case Int64:
            case DateTimeType:
            case UInt64:
                m.WriteU64((qUnsignedInt64) m_int);
                break;
            case Double: {
                qUnsignedInt64 bits;
                memcpy(&bits, &m_double, 8);
                m.WriteU64(bits);
                break;
            }
            case Bool:
                m.WriteBool(m_int != 0);
                break;
            case StringType:
                m.WriteString(m_string);
                break;
            case None:
                break;
        }
    }

    bool Variant::Extract(Message &m)
    {
        m_type = (Type) m.ReadU8();
        switch (m_type)
        {
            case Int64:
            case DateTimeType:
            case UInt64:
                m_int = (qInt64) m.ReadU64();
                break;
            case Double: {
                qUnsignedInt64 bits = m.ReadU64();
                memcpy(&m_double, &bits, 8);
                break;
            }
            case Bool:
                m_int = m.ReadBool();
                break;
            case StringType:
                m_string = m.ReadString();
                break;
            case None:
                break;
            default:
                return false;
        }
        return m.IsValid();
    }

} // namespace nn::nex
