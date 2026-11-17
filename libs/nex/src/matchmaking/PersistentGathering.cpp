#include "MatchMakingHelpers.h"
#include <nn/nex/core/Buffer.h>
#include <nn/nex/core/DateTime.h>
#include <nn/nex/core/String.h>
#include <nn/nex/matchmaking/Gathering.h>
#include <nn/nex/matchmaking/MatchmakeDefs.h>
#include <nn/nex/matchmaking/PersistentGathering.h>
#include <nn/nex/rmc/Data.h>
#include <nn/nex/rmc/Message.h>
#include <nn/nex/types.h>

namespace nn::nex
{

    void PersistentGathering::Reset()
    {
        *(Gathering *) this = Gathering();
        m_communityType     = PersistentGatheringType::Open;
        m_password          = String();
        m_attribs.assign(6, 0);
        m_applicationBuffer     = Buffer();
        m_startDate             = DateTime();
        m_endDate               = DateTime();
        m_matchmakeSessionCount = 0;
        m_participationCount    = 0;
    }

    void PersistentGathering::SetAttribute(qUnsignedInt32 index, qUnsignedInt32 value)
    {
        if (index >= m_attribs.size())
            m_attribs.resize(index + 1, 0);
        m_attribs[index] = value;
    }

    void PersistentGathering::StreamOut(Message &m) const
    {
        Gathering::StreamOut(m);
        size_t s = m.BeginStructure();
        m.WriteU32(m_communityType);
        m.WriteString(m_password);
        AddU32s(m, m_attribs);
        m.WriteBuffer(m_applicationBuffer);
        m.WriteDateTime(m_startDate);
        m.WriteDateTime(m_endDate);
        m.WriteU32(m_matchmakeSessionCount);
        m.WriteU32(m_participationCount);
        m.EndStructure(s);
    }

    bool PersistentGathering::StreamIn(Message &m)
    {
        if (!Gathering::StreamIn(m))
            return false;
        size_t end;
        if (!m.BeginExtractStructure(&end))
            return false;
        m_communityType = m.ReadU32();
        m_password      = m.ReadString();
        if (!ExtractU32s(m, &m_attribs))
            return false;
        m_applicationBuffer     = m.ReadBuffer();
        m_startDate             = m.ReadDateTime();
        m_endDate               = m.ReadDateTime();
        m_matchmakeSessionCount = m.ReadU32();
        m_participationCount    = m.ReadU32();
        m.EndExtractStructure(end);
        return m.IsValid();
    }

    static DataFactoryRegistration<PersistentGathering> s_persistentGathering("PersistentGathering");

} // namespace nn::nex
