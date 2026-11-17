/**
 * @file
 * @brief This is the client for the extended matchmaking protocol that most Wii U games use to create, find and join sessions.
 */

#pragma once

#include <map>
#include <nn/nex/auth/NotificationEvent.h>
#include <nn/nex/core/String.h>
#include <nn/nex/matchmaking/AutoMatchmakeParam.h>
#include <nn/nex/matchmaking/CreateMatchmakeSessionParam.h>
#include <nn/nex/matchmaking/Gathering.h>
#include <nn/nex/matchmaking/JoinMatchmakeSessionParam.h>
#include <nn/nex/matchmaking/MatchmakeSession.h>
#include <nn/nex/matchmaking/MatchmakeSessionSearchCriteria.h>
#include <nn/nex/matchmaking/PersistentGathering.h>
#include <nn/nex/matchmaking/PlayingSession.h>
#include <nn/nex/matchmaking/UpdateMatchmakeSessionParam.h>
#include <nn/nex/rmc/AnyObjectHolder.h>
#include <nn/nex/rmc/ClientProtocol.h>
#include <nn/nex/rmc/ResultRange.h>
#include <nn/nex/runtime/ProtocolCallContext.h>
#include <nn/nex/types.h>

namespace nn::nex
{

    /**
     * @brief This is the client for the extended matchmaking protocol that most Wii U games use to create, find and join sessions.
     */
    class MatchmakeExtensionClient : public ClientProtocol
    {
    public:
        MatchmakeExtensionClient() : ClientProtocol(109)
        {}

        /**
         * @brief Stops new players from joining a session we own. Protocol 109 method 1.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param gid The ID from Gathering::GetID.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool CloseParticipation(ProtocolCallContext *context, qUnsignedInt32 gid);

        /**
         * @brief Lets new players join a session again. Protocol 109 method 2.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param gid The ID from Gathering::GetID.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool OpenParticipation(ProtocolCallContext *context, qUnsignedInt32 gid);

        /**
         * @brief Returns the sessions that match the criteria, without joining any. Protocol 109 method 4.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param criteria The filter. Empty attributes match anything.
         * @param range Which page of results to return.
         * @param gatherings Receives the gatherings found. Filled in when context completes, so it has to stay alive until then.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool BrowseMatchmakeSession(ProtocolCallContext *context, const MatchmakeSessionSearchCriteria &criteria, const ResultRange &range,
                                    qList<AnyObjectHolder<Gathering, String>> *gatherings);

        /**
         * @brief Sets one attribute of a session we own. Protocol 109 method 8.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param gid The ID from Gathering::GetID.
         * @param index The attribute index, from 0 to 5.
         * @param value The new attribute value.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool ModifyMatchmakeSessionAttribute(ProtocolCallContext *context, qUnsignedInt32 gid, qUnsignedInt32 index, qUnsignedInt32 value);

        /**
         * @brief Publishes our own status, which friends read with GetFriendNotificationData. Protocol 109 method 9.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param type A game-defined notification type.
         * @param param1 A value whose meaning the game picks for type.
         * @param param2 A second value whose meaning the game picks for type.
         * @param strParam A game-defined string.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool UpdateNotificationData(ProtocolCallContext *context, qUnsignedInt32 type, qUnsignedInt32 param1, qUnsignedInt32 param2, String strParam);

        /**
         * @brief Replaces the game-defined data stored with a session. Protocol 109 method 11.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param gid The ID from Gathering::GetID.
         * @param buffer The new application data.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool UpdateApplicationBuffer(ProtocolCallContext *context, qUnsignedInt32 gid, qVector<qByte> buffer);

        /**
         * @brief Replaces all six attributes of a session we own. Protocol 109 method 12.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param gid The ID from Gathering::GetID.
         * @param attributes Points at six values, since the server keeps a fixed array of six per session.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool UpdateMatchmakeSessionAttribute(ProtocolCallContext *context, qUnsignedInt32 gid, qUnsignedInt32 *attributes);

        /**
         * @brief Returns the status our friends published with UpdateNotificationData, filtered by type. Protocol 109 method 13.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param events Receives the friends' notifications. Filled in when context completes, so it has to stay alive until then.
         * @param types The notification types to return.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool GetFriendNotificationData(ProtocolCallContext *context, qList<NotificationEvent> *events, const qList<qUnsignedInt32> &types);

        /**
         * @brief Returns the session each of the given players is in, which is how a game joins a friend. Protocol 109 method 16.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param pids The players to look up.
         * @param sessions Receives one entry for each player in a session. Filled in when context completes, so it has to stay alive until then.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool GetPlayingSession(ProtocolCallContext *context, const qList<qUnsignedInt32> &pids, qList<PlayingSession> *sessions);

        /**
         * @brief Protocol 109 method 17.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param gathering The gathering to create. Its ID is ignored.
         * @param gid Receives the new gathering's ID. Filled in when context completes, so it has to stay alive until then.
         * @param message A message shown to the other participants.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool CreatePersistentGathering(ProtocolCallContext *context, const PersistentGathering &gathering, qUnsignedInt32 *gid,
                                       const String &message);

        /**
         * @brief Protocol 109 method 18.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param gathering The gathering with its new fields. Its ID picks which one to update.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool UpdatePersistentGathering(ProtocolCallContext *context, const PersistentGathering &gathering);

        /**
         * @brief Protocol 109 method 19.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param gid The ID from Gathering::GetID.
         * @param message A message shown to the other participants.
         * @param password The password for a PasswordLocked gathering, otherwise empty.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool JoinPersistentGathering(ProtocolCallContext *context, qUnsignedInt32 gid, const String &message, const String &password);

        /**
         * @brief Protocol 109 method 20.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param gids The gatherings to look up.
         * @param gatherings Receives the gatherings found. Filled in when context completes, so it has to stay alive until then.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool FindPersistentGatheringByGatheringId(ProtocolCallContext *context, const qList<qUnsignedInt32> &gids,
                                                  qList<PersistentGathering> *gatherings);

        /**
         * @brief Returns the gatherings the server operator created, optionally only those open to join. Protocol 109 method 21.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param isAvailableOnly Whether to leave out gatherings that are full or closed.
         * @param range Which page of results to return.
         * @param gatherings Receives the gatherings found. Filled in when context completes, so it has to stay alive until then.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool FindOfficialPersistentGathering(ProtocolCallContext *context, bool isAvailableOnly, const ResultRange &range,
                                             qList<PersistentGathering> *gatherings);

        /**
         * @brief Returns the persistent gatherings pid belongs to. Protocol 109 method 22.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param pid The player whose gatherings to return.
         * @param range Which page of results to return.
         * @param gatherings Receives the gatherings found. Filled in when context completes, so it has to stay alive until then.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool FindPersistentGatheringByParticipant(ProtocolCallContext *context, qUnsignedInt32 pid, const ResultRange &range,
                                                  qList<PersistentGathering> *gatherings);

        /**
         * @brief Reports how far the session's game has progressed, which AutoMatchmake uses to skip nearly finished sessions. Nintendo's signature takes no context, so we send it on one we own. Protocol 109 method 34.
         *
         * @param gid The ID from Gathering::GetID.
         * @param score Progress from 0 to 100.
         * @return False when the call could not be sent.
         */
        bool UpdateProgressScore(qUnsignedInt32 gid, qByte score);

        /**
         * @brief Makes the server generate a password that joining players then need. Protocol 109 method 36.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param gid The ID from Gathering::GetID.
         * @param password Receives the new password. Filled in when context completes, so it has to stay alive until then.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool GenerateMatchmakeSessionSystemPassword(ProtocolCallContext *context, qUnsignedInt32 gid, String *password);

        /**
         * @brief Protocol 109 method 37.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param gid The ID from Gathering::GetID.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool ClearMatchmakeSessionSystemPassword(ProtocolCallContext *context, qUnsignedInt32 gid);

        /**
         * @brief Creates a session with us as owner and host, and returns it with the gid filled in. Protocol 109 method 38.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param param The session to send and the options for the call.
         * @param session Receives the session as the server stored it. Filled in when context completes, so it has to stay alive until then.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool CreateMatchmakeSession(ProtocolCallContext *context, const CreateMatchmakeSessionParam &param, MatchmakeSession *session);

        /**
         * @brief Joins a known session by gid and returns it. Protocol 109 method 39.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param param The session to send and the options for the call.
         * @param session Receives the session as the server stored it. Filled in when context completes, so it has to stay alive until then.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool JoinMatchmakeSession(ProtocolCallContext *context, const JoinMatchmakeSessionParam &param, MatchmakeSession *session);

        /**
         * @brief Joins the best matching session, or creates one when nothing matches, and returns it. Protocol 109 method 40.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param param The session to send and the options for the call.
         * @param session Receives the session as the server stored it. Filled in when context completes, so it has to stay alive until then.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool AutoMatchmake(ProtocolCallContext *context, const AutoMatchmakeParam &param, MatchmakeSession *session);

        /**
         * @brief Protocol 109 method 41.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param gid The ID from Gathering::GetID.
         * @param session Receives the session as the server stored it. Filled in when context completes, so it has to stay alive until then.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool FindMatchmakeSessionByGatheringIdDetail(ProtocolCallContext *context, qUnsignedInt32 gid, MatchmakeSession *session);

        /**
         * @brief Protocol 109 method 44.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param param The session to send and the options for the call.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool UpdateMatchmakeSession(ProtocolCallContext *context, const UpdateMatchmakeSessionParam &param);

        /**
         * @brief Sets the key that scrambles gids into the codes players share. The game sets it once, before converting any codes.
         *
         * @param key The game's code key.
         */
        static void SetPersistentGatheringCodeKey(const String &key);
        static void ClearPersistentGatheringCodeKey();
        static qUnsignedInt64 GatheringIdToPersistentGatheringCode(qUnsignedInt32 gid);
        static qUnsignedInt32 PersistentGatheringCodeToGatheringId(qUnsignedInt64 code);

    private:
        ProtocolCallContext m_progressContext;
    };

} // namespace nn::nex
