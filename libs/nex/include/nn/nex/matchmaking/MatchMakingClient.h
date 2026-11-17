/**
 * @file
 * @brief This is the client for the original matchmaking protocol. It also carries EndParticipation, which the server keeps under protocol
 * 50.
 */

#pragma once

#include <map>
#include <nn/nex/core/StationURL.h>
#include <nn/nex/core/String.h>
#include <nn/nex/matchmaking/Gathering.h>
#include <nn/nex/rmc/AnyObjectHolder.h>
#include <nn/nex/rmc/ClientProtocol.h>
#include <nn/nex/rmc/EndPoint.h>
#include <nn/nex/rmc/ResultRange.h>
#include <nn/nex/runtime/ProtocolCallContext.h>
#include <nn/nex/types.h>

namespace nn::nex
{

    /**
     * @brief This is the client for the original matchmaking protocol. It also carries EndParticipation, which the server keeps under protocol 50.
     */
    class MatchMakingClient : public ClientProtocol
    {
    public:
        MatchMakingClient() : ClientProtocol(21), m_ext(50)
        {}

        void Bind(EndPoint *endPoint) override
        {
            ClientProtocol::Bind(endPoint);
            m_ext.Bind(endPoint);
        }

        /**
         * @brief Deletes a gathering we own. Protocol 21 method 2.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param gid The ID from Gathering::GetID.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool UnregisterGathering(ProtocolCallContext *context, qUnsignedInt32 gid);

        /**
         * @brief Protocol 21 method 21.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param gid The ID from Gathering::GetID.
         * @param gathering Receives the gathering, whatever its class. Filled in when context completes, so it has to stay alive until then.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool FindByID(ProtocolCallContext *context, qUnsignedInt32 gid, AnyObjectHolder<Gathering, String> *gathering);

        /**
         * @brief Returns the gatherings owned by ownerPid. Protocol 21 method 22.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param ownerPid The principal ID of the owner.
         * @param range Which page of results to return.
         * @param gatherings Receives the gatherings found. Filled in when context completes, so it has to stay alive until then.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool FindByOwner(ProtocolCallContext *context, qUnsignedInt32 ownerPid, const ResultRange &range,
                         qList<AnyObjectHolder<Gathering, String>> *gatherings);

        /**
         * @brief Returns the host's station URLs for a session, which is what a joining player connects to. Protocol 21 method 41.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param gid The ID from Gathering::GetID.
         * @param urls Receives the host's station URLs. Filled in when context completes, so it has to stay alive until then.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool GetSessionURLs(ProtocolCallContext *context, qUnsignedInt32 gid, qList<StationURL> *urls);

        /**
         * @brief Makes us the session host, and the owner too when isMigrateOwner is set. Protocol 21 method 42.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param gid The ID from Gathering::GetID.
         * @param isMigrateOwner Whether ownership moves to us as well.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool UpdateSessionHost(ProtocolCallContext *context, qUnsignedInt32 gid, bool isMigrateOwner);

        /**
         * @brief Hands ownership to the first of the candidates the server accepts. Protocol 21 method 44.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param gid The ID from Gathering::GetID.
         * @param candidates PIDs in order of preference.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool MigrateGatheringOwnership(ProtocolCallContext *context, qUnsignedInt32 gid, const qList<qUnsignedInt32> &candidates);

        /**
         * @brief Leaves a gathering and shows the message to the remaining participants. Protocol 50 method 1.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param gid The ID from Gathering::GetID.
         * @param message A message shown to the other participants.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool EndParticipation(ProtocolCallContext *context, qUnsignedInt32 gid, const String &message);

    private:
        class Extension : public ClientProtocol
        {
        public:
            explicit Extension(qUnsignedInt16 id) : ClientProtocol(id)
            {}
            using ClientProtocol::Call;
        };
        Extension m_ext;
    };

} // namespace nn::nex
