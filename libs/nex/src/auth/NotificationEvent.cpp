#include <algorithm>
#include <nn/nex/auth/NotificationEvent.h>
#include <nn/nex/rmc/Data.h>
#include <nn/nex/rmc/Message.h>

namespace nn::nex
{

    void NotificationEvent::StreamOut(Message &m) const
    {
        size_t s = m.BeginStructure();
        m.WriteU32(m_pidSource);
        m.WriteU32(m_type);
        m.WriteU32(m_param1);
        m.WriteU32(m_param2);
        m.WriteString(m_strParam);
        m.WriteU32(m_param3);
        m.EndStructure(s);
    }

    bool NotificationEvent::StreamIn(Message &m)
    {
        size_t end;
        if (!m.BeginExtractStructure(&end))
            return false;
        m_pidSource = m.ReadU32();
        m_type      = m.ReadU32();
        m_param1    = m.ReadU32();
        m_param2    = m.ReadU32();
        m_strParam  = m.ReadString();
        // Older servers stop before the third parameter, so we only read it when the structure has room for it
        m_param3 = m.GetCursor() + 4 <= end ? m.ReadU32() : 0;
        m.EndExtractStructure(end);
        return m.IsValid();
    }

    static DataFactoryRegistration<NotificationEvent> s_notificationEvent("NotificationEvent");

} // namespace nn::nex
