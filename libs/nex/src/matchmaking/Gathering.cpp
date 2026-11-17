#include <nn/nex/matchmaking/Gathering.h>
#include <nn/nex/rmc/Data.h>
#include <nn/nex/rmc/Message.h>

namespace nn::nex
{

    void Gathering::StreamOut(Message &m) const
    {
        size_t s = m.BeginStructure();
        m.WriteU32(m_id);
        m.WriteU32(m_ownerPid);
        m.WriteU32(m_hostPid);
        m.WriteU16(m_minParticipants);
        m.WriteU16(m_maxParticipants);
        m.WriteU32(m_participationPolicy);
        m.WriteU32(m_policyArgument);
        m.WriteU32(m_flags);
        m.WriteU32(m_state);
        m.WriteString(m_description);
        m.EndStructure(s);
    }

    bool Gathering::StreamIn(Message &m)
    {
        size_t end;
        if (!m.BeginExtractStructure(&end))
            return false;
        m_id                  = m.ReadU32();
        m_ownerPid            = m.ReadU32();
        m_hostPid             = m.ReadU32();
        m_minParticipants     = m.ReadU16();
        m_maxParticipants     = m.ReadU16();
        m_participationPolicy = m.ReadU32();
        m_policyArgument      = m.ReadU32();
        m_flags               = m.ReadU32();
        m_state               = m.ReadU32();
        m_description         = m.ReadString();
        m.EndExtractStructure(end);
        return m.IsValid();
    }

    static DataFactoryRegistration<Gathering> s_gathering("Gathering");

} // namespace nn::nex
