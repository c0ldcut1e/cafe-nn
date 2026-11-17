/**
 * @file
 * @brief This is what JoinMatchmakeSession takes, a gid plus the join message and password.
 */

#pragma once

#include <map>
#include <nn/nex/core/String.h>
#include <nn/nex/rmc/Data.h>
#include <nn/nex/rmc/Message.h>
#include <nn/nex/types.h>

namespace nn::nex
{

    /**
     * @brief This is what JoinMatchmakeSession takes, a gid plus the join message and password.
     */
    class JoinMatchmakeSessionParam : public Data
    {
    public:
        JoinMatchmakeSessionParam()
        {
            Reset();
        }
        void Reset();

        qUnsignedInt32 gid;
        qVector<qUnsignedInt32> additionalParticipants;
        qUnsignedInt32 gidForParticipationCheck;
        qUnsignedInt32 option;
        qByte behavior;
        String userPassword;
        String systemPassword;
        String joinMessage;
        qUnsignedInt16 participationCount;

        const char *GetDataTypeName() const override
        {
            return "JoinMatchmakeSessionParam";
        }
        Data *Clone() const override
        {
            return new JoinMatchmakeSessionParam(*this);
        }
        void StreamOut(Message &m) const override;
    };

} // namespace nn::nex
