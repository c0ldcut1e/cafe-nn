#include <algorithm>
#include <nn/nex/auth/RVConnectionData.h>
#include <nn/nex/rmc/Message.h>

namespace nn::nex
{

    bool RVConnectionData::Extract(Message &m)
    {
        size_t end;
        if (!m.BeginExtractStructure(&end))
            return false;
        m_regular = m.ExtractStationURL();
        if (!_Type_qlist<_Type_byte>::Extract(&m, &m_specialProtocols))
            return false;
        m_special = m.ExtractStationURL();
        if (m.GetCursor() + 8 <= end)
            m_currentTime = m.ReadDateTime();
        m.EndExtractStructure(end);
        return m.IsValid();
    }

} // namespace nn::nex
