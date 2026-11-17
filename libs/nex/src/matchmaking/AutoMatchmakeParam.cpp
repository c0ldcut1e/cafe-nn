#include "MatchMakingHelpers.h"
#include <nn/nex/matchmaking/AutoMatchmakeParam.h>
#include <nn/nex/matchmaking/MatchmakeSessionSearchCriteria.h>
#include <nn/nex/rmc/Message.h>

namespace nn::nex
{

    void AutoMatchmakeParam::StreamOut(Message &m) const
    {
        size_t s = m.BeginStructure();
        sourceSession.StreamOut(m);
        AddU32s(m, additionalParticipants);
        m.WriteU32(gidForParticipationCheck);
        m.WriteU32(option);
        m.WriteString(joinMessage);
        m.WriteU16(participationCount);
        m.AddList(searchCriteria,
                  [](Message &e, const MatchmakeSessionSearchCriteria &c)
                  {
                      c.StreamOut(e);
                  });
        AddU32s(m, targetGids);
        m.EndStructure(s);
    }

} // namespace nn::nex
