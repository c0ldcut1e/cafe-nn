#include "MatchMakingHelpers.h"
#include <nn/nex/matchmaking/UpdateMatchmakeSessionParam.h>
#include <nn/nex/rmc/Message.h>

namespace nn::nex
{

    void UpdateMatchmakeSessionParam::StreamOut(Message &m) const
    {
        size_t s = m.BeginStructure();
        m.WriteU32(gid);
        m.WriteU32(modificationFlag);
        AddU32s(m, attributes);
        m.WriteBool(openParticipation);
        m.WriteBuffer(applicationBuffer);
        m.WriteU8(progressScore);
        matchmakeParam.StreamOut(m);
        m.WriteDateTime(startedTime);
        m.WriteString(userPassword);
        m.WriteU32(gameMode);
        m.WriteString(description);
        m.WriteU16(minParticipants);
        m.WriteU16(maxParticipants);
        m.WriteU32(matchmakeSystemType);
        m.WriteU32(participationPolicy);
        m.WriteU32(policyArgument);
        m.WriteString(codeword);
        m.EndStructure(s);
    }

} // namespace nn::nex
