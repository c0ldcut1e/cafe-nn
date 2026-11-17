/**
 * @file
 * @brief This is the NAT traversal protocol client, which reports NAT properties and relays probe requests between peers after login.
 */

#pragma once

#include <functional>
#include <nn/nex/auth/AuthenticationInfo.h>
#include <nn/nex/auth/BackEndServices.h>
#include <nn/nex/auth/NexServiceToken.h>
#include <nn/nex/auth/NgsFacade.h>
#include <nn/nex/auth/NintendoLoginData.h>
#include <nn/nex/auth/RVConnectionData.h>
#include <nn/nex/auth/SecureConnectionProtocolClient.h>
#include <nn/nex/auth/TicketGrantingProtocolClient.h>
#include <nn/nex/core/DateTime.h>
#include <nn/nex/core/String.h>

namespace nn::nex
{

    /**
     * @brief This is the NAT traversal protocol client. The server relays probe requests from other players through it, and it reports our NAT type and connection results.
     */
    class NATTraversalRelayClient : public ClientProtocol
    {
    public:
        /**
         * @brief We hand probe requests to the game because only its P2P layer can punch towards these URLs.
         */
        using ProbeHandler = std::function<void(const qList<StationURL> &urls)>;

        NATTraversalRelayClient() : ClientProtocol(3)
        {}

        /**
         * @brief We hook the server-to-client probe request onto the bound endpoint here, so call it after Bind.
         */
        void Init();
        void SetProbeHandler(ProbeHandler handler)
        {
            m_probeHandler = std::move(handler);
        }

        /**
         * @brief Asks the server to tell the players at targets to probe source, so both sides punch through their NATs at once. Protocol 3 method 3.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param targets The station URLs of the players, as RequestURLs returned them.
         * @param source Our station URL that they should probe.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool CallRequestProbeInitiationExt(ProtocolCallContext *context, const qList<StationURL> &targets, const StationURL &source);

        /**
         * @brief Reports whether we reached cid and the round trip time. Protocol 3 method 4.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param cid The connection ID of the peer we tried.
         * @param result Whether we reached it.
         * @param rtt The round trip time in milliseconds.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool CallReportNATTraversalResult(ProtocolCallContext *context, const qUnsignedInt32 &cid, const bool &result, const qUnsignedInt32 &rtt);

        /**
         * @brief Reports our NAT mapping and filtering type. Protocol 3 method 5.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param natMapping Our NAT mapping type.
         * @param natFiltering Our NAT filtering type.
         * @param rtt The round trip time in milliseconds.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool CallReportNATProperties(ProtocolCallContext *context, const qUnsignedInt32 &natMapping, const qUnsignedInt32 &natFiltering,
                                     const qUnsignedInt32 &rtt);

        /**
         * @brief Returns the relay server settings for when direct connection fails. Protocol 3 method 6.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param relayMode Receives whether relaying is enabled. Filled in when context completes, so it has to stay alive until then.
         * @param currentTime Receives the server time. Filled in when context completes, so it has to stay alive until then.
         * @param address Receives the relay server's address. Filled in when context completes, so it has to stay alive until then.
         * @param port Receives the relay server's port. Filled in when context completes, so it has to stay alive until then.
         * @param relayAddressType Receives the relay address type. Filled in when context completes, so it has to stay alive until then.
         * @param gameServerId Receives the game server ID the relay is for. Filled in when context completes, so it has to stay alive until then.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool CallGetRelaySignatureKey(ProtocolCallContext *context, qInt32 *relayMode, DateTime *currentTime, String *address, qUnsignedInt16 *port,
                                      qInt32 *relayAddressType, qUnsignedInt32 *gameServerId);

    private:
        ProbeHandler m_probeHandler;
    };

} // namespace nn::nex
