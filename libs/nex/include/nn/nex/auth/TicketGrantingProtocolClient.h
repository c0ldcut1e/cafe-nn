/**
 * @file
 * @brief This is the client for the authentication server, which turns a NEX token into a ticket for the secure server.
 */

#pragma once

#include <nn/nex/auth/RVConnectionData.h>
#include <nn/nex/core/Buffer.h>
#include <nn/nex/core/String.h>
#include <nn/nex/rmc/ClientProtocol.h>
#include <nn/nex/rmc/Data.h>
#include <nn/nex/rmc/Message.h>
#include <nn/nex/runtime/ProtocolCallContext.h>
#include <nn/nex/types.h>
#include <vector>

namespace nn::nex
{

    /**
     * @brief This is the client for the authentication server, which turns a NEX token into a ticket for the secure server.
     */
    class TicketGrantingProtocolClient : public ClientProtocol
    {
    public:
        TicketGrantingProtocolClient() : ClientProtocol(10)
        {}

        /**
         * @brief Logs in by user name alone and returns the PID, a sealed ticket and the secure server's URL. Protocol 10 method 1.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param userName The account's PID written in decimal.
         * @param returnValue Receives the server's result code. Filled in when context completes, so it has to stay alive until then.
         * @param principalId Receives our PID. Filled in when context completes, so it has to stay alive until then.
         * @param ticket Receives the ticket sealed under the account's key. Filled in when context completes, so it has to stay alive until then.
         * @param connectionData Receives the secure server's station URL. Filled in when context completes, so it has to stay alive until then.
         * @param returnMessage Receives the server's message, such as its build name. Filled in when context completes, so it has to stay alive until then.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool Login(ProtocolCallContext *context, const String &userName, qResult *returnValue, qUnsignedInt32 *principalId, Buffer *ticket,
                   RVConnectionData *connectionData, String *returnMessage);

        /**
         * @brief Logs in with a NEX token in extraData, which is what Wii U servers expect. The results match Login. Protocol 10 method 2.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param userName The account's PID written in decimal.
         * @param extraData An AuthenticationInfo holding the NEX token.
         * @param returnValue Receives the server's result code. Filled in when context completes, so it has to stay alive until then.
         * @param principalId Receives our PID. Filled in when context completes, so it has to stay alive until then.
         * @param ticket Receives the ticket sealed under the account's key. Filled in when context completes, so it has to stay alive until then.
         * @param connectionData Receives the secure server's station URL. Filled in when context completes, so it has to stay alive until then.
         * @param returnMessage Receives the server's message, such as its build name. Filled in when context completes, so it has to stay alive until then.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool LoginEx(ProtocolCallContext *context, const String &userName, const Data &extraData, qResult *returnValue, qUnsignedInt32 *principalId,
                     Buffer *ticket, RVConnectionData *connectionData, String *returnMessage);

        /**
         * @brief Asks for a ticket from sourcePid to targetPid, which is how we get the secure server's ticket after LoginEx. Protocol 10 method 3.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param sourcePid Our PID.
         * @param targetPid The PID of the server the ticket is for, which is the secure server's PID from connectionData.
         * @param returnValue Receives the server's result code. Filled in when context completes, so it has to stay alive until then.
         * @param ticket Receives the ticket sealed under the account's key. Filled in when context completes, so it has to stay alive until then.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool RequestTicket(ProtocolCallContext *context, qUnsignedInt32 sourcePid, qUnsignedInt32 targetPid, qResult *returnValue, Buffer *ticket);

    private:
        bool LoginCommon(ProtocolCallContext *context, qUnsignedInt32 methodId, const Message &parameters, qResult *returnValue,
                         qUnsignedInt32 *principalId, Buffer *ticket, RVConnectionData *connectionData, String *returnMessage);
    };

} // namespace nn::nex
