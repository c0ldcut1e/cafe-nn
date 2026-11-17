/**
 * @file
 * @brief This carries RMC calls over one PRUDP connection and routes the requests the server makes of us.
 */

#pragma once

#include <nn/nex/core/Buffer.h>
#include <nn/nex/core/StationURL.h>
#include <nn/nex/rmc/Message.h>
#include <nn/nex/runtime/CallContext.h>
#include <nn/nex/runtime/ProtocolCallContext.h>
#include <nn/nex/transport/PRUDPEndPoint.h>
#include <nn/nex/types.h>

namespace nn::nex
{

    /**
     * @brief This carries RMC calls over one PRUDP connection and routes the requests the server makes of us.
     */
    class EndPoint
    {
    public:
        /**
         * @brief The handler fills the response and returns the result the server should see.
         */
        using RequestHandler    = std::function<qResult(qUnsignedInt32 methodId, Message &request, Message &response)>;
        using DisconnectHandler = std::function<void(qResult)>;

        EndPoint();
        ~EndPoint();
        EndPoint(const EndPoint &)            = delete;
        EndPoint &operator=(const EndPoint &) = delete;

        qResult Connect(CallContext *context, const StationURL &url, const char *accessKey, const Buffer &connectPayload, const Buffer *sessionKey);
        void Close();
        bool IsConnected() const
        {
            return m_transport.IsConnected();
        }
        PRUDPEndPoint &GetTransport()
        {
            return m_transport;
        }

        bool Call(ProtocolCallContext *context, qUnsignedInt16 protocolId, qUnsignedInt32 methodId, const Message &parameters,
                  ProtocolCallContext::ResponseHandler handler);
        void RegisterRequestHandler(qUnsignedInt16 protocolId, RequestHandler handler);
        void SetDisconnectHandler(DisconnectHandler handler)
        {
            m_onDisconnect = std::move(handler);
        }

    private:
        void OnReceive(Buffer &data);
        void OnDisconnect(qResult result);
        void FailAll(qResult result);
        static void WriteProtocolID(Message &m, qUnsignedInt16 protocolId, bool request);

        PRUDPEndPoint m_transport;
        qUnsignedInt32 m_nextCallId = 1;
        std::map<qUnsignedInt32, ProtocolCallContext *> m_pending;
        std::map<qUnsignedInt16, RequestHandler> m_handlers;
        DisconnectHandler m_onDisconnect;
    };

} // namespace nn::nex
