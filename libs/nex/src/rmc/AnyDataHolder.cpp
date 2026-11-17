#include <nn/nex/core/String.h>
#include <nn/nex/rmc/AnyDataHolder.h>
#include <nn/nex/rmc/Data.h>
#include <nn/nex/rmc/Message.h>
#include <nn/nex/types.h>

namespace nn::nex
{

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

} // namespace nn::nex
