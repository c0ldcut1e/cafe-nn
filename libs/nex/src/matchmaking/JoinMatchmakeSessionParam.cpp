#include "MatchMakingHelpers.h"
#include <nn/nex/core/String.h>
#include <nn/nex/matchmaking/JoinMatchmakeSessionParam.h>
#include <nn/nex/rmc/Message.h>

namespace nn::nex
{

    void JoinMatchmakeSessionParam::Reset()
    {
        gid = 0;
        additionalParticipants.clear();
        gidForParticipationCheck = 0;
        option                   = 0;
        behavior                 = 0;
        userPassword             = String();
        systemPassword           = String();
        joinMessage              = String();
        participationCount       = 1;
    }

    void JoinMatchmakeSessionParam::StreamOut(Message &m) const
    {
        size_t s = m.BeginStructure();
        m.WriteU32(gid);
        AddU32s(m, additionalParticipants);
        m.WriteU32(gidForParticipationCheck);
        m.WriteU32(option);
        m.WriteU8(behavior);
        m.WriteString(userPassword);
        m.WriteString(systemPassword);
        m.WriteString(joinMessage);
        m.WriteU16(participationCount);
        m.EndStructure(s);
    }

} // namespace nn::nex
