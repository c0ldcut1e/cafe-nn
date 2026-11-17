/**
 * @file
 * @brief These are the enums the matchmaking structures and calls share.
 */

#pragma once

#include <map>

namespace nn::nex
{

    /**
     * @brief This says who may join a session.
     */
    enum MatchmakeSystemType
    {
        MatchmakeSystemTypeInvalid             = 0,
        MatchmakeSystemTypeAnybody             = 1,
        MatchmakeSystemTypeFriend              = 2,
        MatchmakeSystemTypeFriendInvite        = 3,
        MatchmakeSystemTypePersistentGathering = 6
    };

    /**
     * @brief This says how AutoMatchmake ranks the sessions that match.
     */
    enum MatchmakeSelectionMethod
    {
        MatchmakeSelectionMethodRandom                        = 0,
        MatchmakeSelectionMethodNearestNeighbor               = 1,
        MatchmakeSelectionMethodBroadenRange                  = 2,
        MatchmakeSelectionMethodProgressScore                 = 3,
        MatchmakeSelectionMethodBroadenRangeWithProgressScore = 4,
        MatchmakeSelectionMethodScoreBased                    = 5
    };

    namespace PersistentGatheringType
    {
        enum PersistentGatheringType
        {
            Open           = 0,
            PasswordLocked = 1,
            Official       = 2
        };
    } // namespace PersistentGatheringType

} // namespace nn::nex
