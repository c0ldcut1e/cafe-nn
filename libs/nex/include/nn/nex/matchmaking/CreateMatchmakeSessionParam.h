/**
 * @file
 * @brief This is what CreateMatchmakeSession takes, a session plus the options that only matter at creation.
 */

#pragma once

#include <map>
#include <nn/nex/core/String.h>
#include <nn/nex/matchmaking/MatchmakeSession.h>
#include <nn/nex/rmc/Data.h>
#include <nn/nex/rmc/Message.h>
#include <nn/nex/types.h>

namespace nn::nex
{

    /**
     * @brief This is what CreateMatchmakeSession takes, a session plus the options that only matter at creation.
     */
    class CreateMatchmakeSessionParam : public Data
    {
    public:
        MatchmakeSession sourceSession;
        qVector<qUnsignedInt32> additionalParticipants;
        qUnsignedInt32 gidForParticipationCheck = 0;
        qUnsignedInt32 option                   = 0;
        String joinMessage;
        qUnsignedInt16 participationCount = 1;

        const char *GetDataTypeName() const override
        {
            return "CreateMatchmakeSessionParam";
        }
        Data *Clone() const override
        {
            return new CreateMatchmakeSessionParam(*this);
        }
        void StreamOut(Message &m) const override;
    };

} // namespace nn::nex
