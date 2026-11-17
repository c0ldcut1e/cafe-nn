/**
 * @file
 * @brief This is what UpdateMatchmakeSession takes, and its modification flags say which session fields change.
 */

#pragma once

#include <map>
#include <nn/nex/core/Buffer.h>
#include <nn/nex/core/DateTime.h>
#include <nn/nex/core/String.h>
#include <nn/nex/matchmaking/MatchmakeParam.h>
#include <nn/nex/rmc/Data.h>
#include <nn/nex/rmc/Message.h>
#include <nn/nex/types.h>

namespace nn::nex
{

    /**
     * @brief This is what UpdateMatchmakeSession takes, and its modification flags say which session fields change.
     */
    class UpdateMatchmakeSessionParam : public Data
    {
    public:
        qUnsignedInt32 gid              = 0;
        qUnsignedInt32 modificationFlag = 0;
        qVector<qUnsignedInt32> attributes;
        bool openParticipation = false;
        Buffer applicationBuffer;
        qByte progressScore = 0;
        MatchmakeParam matchmakeParam;
        DateTime startedTime;
        String userPassword;
        qUnsignedInt32 gameMode = 0;
        String description;
        qUnsignedInt16 minParticipants     = 0;
        qUnsignedInt16 maxParticipants     = 0;
        qUnsignedInt32 matchmakeSystemType = 0;
        qUnsignedInt32 participationPolicy = 0;
        qUnsignedInt32 policyArgument      = 0;
        String codeword;

        const char *GetDataTypeName() const override
        {
            return "UpdateMatchmakeSessionParam";
        }
        Data *Clone() const override
        {
            return new UpdateMatchmakeSessionParam(*this);
        }
        void StreamOut(Message &m) const override;
    };

} // namespace nn::nex
