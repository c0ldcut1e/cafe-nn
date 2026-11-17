/**
 * @file
 * @brief This pairs a player with the gathering they are in, as GetPlayingSession returns it.
 */

#pragma once

#include <map>
#include <nn/nex/core/String.h>
#include <nn/nex/matchmaking/Gathering.h>
#include <nn/nex/matchmaking/MatchmakeSession.h>
#include <nn/nex/rmc/AnyObjectHolder.h>
#include <nn/nex/rmc/Data.h>
#include <nn/nex/rmc/Message.h>
#include <nn/nex/types.h>

namespace nn::nex
{

    /**
     * @brief This pairs a player with the gathering they are in, as GetPlayingSession returns it.
     */
    class PlayingSession : public Data
    {
    public:
        qUnsignedInt32 GetPrincipalID() const
        {
            return m_principalId;
        }
        const MatchmakeSession *GetMatchmakeSession() const;

        const char *GetDataTypeName() const override
        {
            return "PlayingSession";
        }
        Data *Clone() const override
        {
            return new PlayingSession(*this);
        }
        bool StreamIn(Message &m) override;

    private:
        qUnsignedInt32 m_principalId = 0;
        AnyObjectHolder<Gathering, String> m_gathering;
    };

} // namespace nn::nex
