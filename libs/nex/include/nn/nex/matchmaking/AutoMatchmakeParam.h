/**
 * @file
 * @brief This is what AutoMatchmake takes. The server joins the best match among the criteria, or creates the session given when nothing
 * matches.
 */

#pragma once

#include <map>
#include <nn/nex/core/String.h>
#include <nn/nex/matchmaking/MatchmakeSession.h>
#include <nn/nex/matchmaking/MatchmakeSessionSearchCriteria.h>
#include <nn/nex/rmc/Data.h>
#include <nn/nex/rmc/Message.h>
#include <nn/nex/types.h>

namespace nn::nex
{

    /**
     * @brief This is what AutoMatchmake takes. The server joins the best match among the criteria, or creates the session given when nothing matches.
     */
    class AutoMatchmakeParam : public Data
    {
    public:
        MatchmakeSession sourceSession;
        qVector<qUnsignedInt32> additionalParticipants;
        qUnsignedInt32 gidForParticipationCheck = 0;
        qUnsignedInt32 option                   = 0;
        String joinMessage;
        qUnsignedInt16 participationCount = 1;
        qVector<MatchmakeSessionSearchCriteria> searchCriteria;
        qVector<qUnsignedInt32> targetGids;

        const char *GetDataTypeName() const override
        {
            return "AutoMatchmakeParam";
        }
        Data *Clone() const override
        {
            return new AutoMatchmakeParam(*this);
        }
        void StreamOut(Message &m) const override;
    };

} // namespace nn::nex
