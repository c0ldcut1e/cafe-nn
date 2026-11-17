#include "MatchMakingHelpers.h"
#include <nn/nex/matchmaking/CreateMatchmakeSessionParam.h>
#include <nn/nex/rmc/Message.h>

namespace nn::nex
{

    void CreateMatchmakeSessionParam::StreamOut(Message &m) const
    {
        size_t s = m.BeginStructure();
        sourceSession.StreamOut(m);
        AddU32s(m, additionalParticipants);
        m.WriteU32(gidForParticipationCheck);
        m.WriteU32(option);
        m.WriteString(joinMessage);
        m.WriteU16(participationCount);
        m.EndStructure(s);
    }

} // namespace nn::nex
