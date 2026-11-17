/**
 * @file
 * @brief This is the client for the secure server's own protocol, which registers our station URLs and looks up other players' URLs.
 */

#pragma once

#include <nn/nex/core/StationURL.h>
#include <nn/nex/rmc/ClientProtocol.h>
#include <nn/nex/rmc/Data.h>
#include <nn/nex/runtime/ProtocolCallContext.h>
#include <nn/nex/types.h>
#include <vector>

namespace nn::nex
{

    /**
     * @brief This is the client for the secure server's own protocol, which registers our station URLs and looks up other players' URLs.
     */
    class SecureConnectionProtocolClient : public ClientProtocol
    {
    public:
        SecureConnectionProtocolClient() : ClientProtocol(11)
        {}

        /**
         * @brief Registers our local station URLs and returns our connection ID and the public URL the server saw us at. Protocol 11 method 1.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param urls Our local addresses, one per interface. The server keeps them so peers can ask for them.
         * @param returnValue Receives the server's result code. Filled in when context completes, so it has to stay alive until then.
         * @param connectionId Receives our connection ID, which peers put in RVCID. Filled in when context completes, so it has to stay alive until then.
         * @param publicUrl Receives our address as the server saw it. Filled in when context completes, so it has to stay alive until then.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool Register(ProtocolCallContext *context, const qList<StationURL> &urls, qResult *returnValue, qUnsignedInt32 *connectionId,
                      StationURL *publicUrl);

        /**
         * @brief Does the same as Register but also sends NintendoLoginData, which Wii U servers require. Protocol 11 method 4.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param urls Our local addresses, one per interface. The server keeps them so peers can ask for them.
         * @param loginData A NintendoLoginData holding the NEX token.
         * @param returnValue Receives the server's result code. Filled in when context completes, so it has to stay alive until then.
         * @param connectionId Receives our connection ID, which peers put in RVCID. Filled in when context completes, so it has to stay alive until then.
         * @param publicUrl Receives our address as the server saw it. Filled in when context completes, so it has to stay alive until then.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool RegisterEx(ProtocolCallContext *context, const qList<StationURL> &urls, const Data &loginData, qResult *returnValue,
                        qUnsignedInt32 *connectionId, StationURL *publicUrl);

        /**
         * @brief Returns the station URLs another player registered, so a peer can connect to them. Protocol 11 method 3.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param cidTarget The other player's connection ID.
         * @param pidTarget The other player's PID.
         * @param returnValue Receives the server's result code. Filled in when context completes, so it has to stay alive until then.
         * @param urls Receives the other player's station URLs. Filled in when context completes, so it has to stay alive until then.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool RequestURLs(ProtocolCallContext *context, qUnsignedInt32 cidTarget, qUnsignedInt32 pidTarget, bool *returnValue,
                         qList<StationURL> *urls);

        /**
         * @brief Replaces our registered station URLs. Protocol 11 method 6.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param urls Our local addresses, one per interface. The server keeps them so peers can ask for them.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool UpdateURLs(ProtocolCallContext *context, const qList<StationURL> &urls);

        /**
         * @brief Replaces one registered station URL with another. Protocol 11 method 7.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param target The registered URL to replace.
         * @param url The URL that replaces it.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool ReplaceURL(ProtocolCallContext *context, const StationURL &target, const StationURL &url);

        /**
         * @brief Sends the server an opaque report blob. Protocol 11 method 8.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param reportId A game-defined report type.
         * @param data The report contents.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool SendReport(ProtocolCallContext *context, qUnsignedInt32 reportId, const qVector<qByte> &data);
    };

} // namespace nn::nex
