#include <nn/nex/Message.h>

namespace nn::nex
{

    size_t Message::BeginStructure(qUnsignedInt8 version)
    {
        WriteU8(version);
        size_t start = GetSize();
        WriteU32(0);
        return start;
    }

    void Message::EndStructure(size_t start)
    {
        PatchU32(start, (qUnsignedInt32) (GetSize() - start - 4));
    }

    bool Message::BeginExtractStructure(size_t *end, qUnsignedInt8 *version)
    {
        qUnsignedInt8 v     = ReadU8();
        qUnsignedInt32 size = ReadU32();
        if (!IsValid() || size > GetRemaining())
            return false;
        if (version)
            *version = v;
        *end = GetCursor() + size;
        return true;
    }

    namespace
    {

        std::map<std::string, DataFactory::Creator> &Creators()
        {
            static std::map<std::string, DataFactory::Creator> creators;
            return creators;
        }

    } // namespace

    void DataFactory::Register(const char *name, Creator creator)
    {
        Creators()[name] = creator;
    }

    Data *DataFactory::Create(const String &name)
    {
        auto it = Creators().find(name.ToUtf8());
        return it == Creators().end() ? nullptr : it->second();
    }

    void AnyDataHolder::Add(Message &m) const
    {
        m.WriteString(String(m_data ? m_data->GetDataTypeName() : ""));
        size_t outer = m.GetSize();
        m.WriteU32(0);
        m.WriteU32(0);
        if (m_data)
            m_data->StreamOut(m);
        qUnsignedInt32 inner = (qUnsignedInt32) (m.GetSize() - outer - 8);
        m.PatchU32(outer, inner + 4);
        m.PatchU32(outer + 4, inner);
    }

    bool AnyDataHolder::Extract(Message &m)
    {
        String name = m.ReadString();
        m.ReadU32();
        qUnsignedInt32 size = m.ReadU32();
        if (!m.IsValid() || size > m.GetRemaining())
            return false;
        size_t end = m.GetCursor() + size;
        m_data.reset(DataFactory::Create(name));
        // An unknown class is skipped rather than failing the whole message, so one odd entry in a list costs only itself
        if (m_data && !m_data->StreamIn(m))
            m_data.reset();
        m.SetCursor(end);
        return m.IsValid();
    }

    void ResultRange::Add(Message &m) const
    {
        size_t s = m.BeginStructure();
        m.WriteU32(m_offset);
        m.WriteU32(m_size);
        m.EndStructure(s);
    }

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
