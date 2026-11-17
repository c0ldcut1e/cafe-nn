#include <nn/nex/rmc/Message.h>
#include <nn/nex/rmc/ResultRange.h>

namespace nn::nex
{

    void ResultRange::Add(Message &m) const
    {
        size_t s = m.BeginStructure();
        m.WriteU32(m_offset);
        m.WriteU32(m_size);
        m.EndStructure(s);
    }

} // namespace nn::nex
