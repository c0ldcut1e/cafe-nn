#include <nn/nex/core/String.h>
#include <nn/nex/matchmaking/Gathering.h>
#include <nn/nex/matchmaking/MatchmakeSession.h>
#include <nn/nex/matchmaking/PlayingSession.h>
#include <nn/nex/rmc/Message.h>

namespace nn::nex
{

    const MatchmakeSession *PlayingSession::GetMatchmakeSession() const
    {
        Gathering *g = m_gathering.Get();
        // A string compare stands in for dynamic_cast because the console build has no RTTI
        if (g && String(g->GetDataTypeName()) == String("MatchmakeSession"))
            return (const MatchmakeSession *) g;
        return nullptr;
    }

    bool PlayingSession::StreamIn(Message &m)
    {
        size_t end;
        if (!m.BeginExtractStructure(&end))
            return false;
        m_principalId = m.ReadU32();
        if (!m_gathering.Extract(m))
            return false;
        m.EndExtractStructure(end);
        return m.IsValid();
    }

} // namespace nn::nex
