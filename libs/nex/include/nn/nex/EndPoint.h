/**
 * @file
 * @brief This is the RMC layer, which matches calls to responses on top of a PRUDP connection, and the ClientProtocol base that every protocol client derives from.
 */

#pragma once

#include <nn/nex/Message.h>
#include <nn/nex/PRUDPEndPoint.h>

namespace nn::nex
{

    /**
     * @brief This holds the PID, ticket and session key we present to the secure server.
     */
    class Credentials
    {
    public:
        Credentials() = default;
        explicit Credentials(qUnsignedInt32 principalId) : m_principalId(principalId)
        {}
        qUnsignedInt32 GetPrincipalID() const
        {
            return m_principalId;
        }
        void SetPrincipalID(qUnsignedInt32 principalId)
        {
            m_principalId = principalId;
        }

    private:
        qUnsignedInt32 m_principalId = 0;
    };

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

    /**
     * @brief Every protocol client derives from this and sends its calls through the endpoint it is bound to. Bind it with BackEndServices::RegisterProtocol after login.
     */
    class ClientProtocol
    {
    public:
        explicit ClientProtocol(qUnsignedInt16 protocolId) : m_protocolId(protocolId)
        {}
        virtual ~ClientProtocol() = default;

        virtual void Bind(EndPoint *endPoint)
        {
            m_endPoint = endPoint;
        }
        EndPoint *GetEndPoint() const
        {
            return m_endPoint;
        }
        qUnsignedInt16 GetProtocolID() const
        {
            return m_protocolId;
        }

    protected:
        bool Call(ProtocolCallContext *context, qUnsignedInt32 methodId, const Message &parameters,
                  ProtocolCallContext::ResponseHandler handler = nullptr);

    private:
        qUnsignedInt16 m_protocolId;
        EndPoint *m_endPoint = nullptr;
    };

} // namespace nn::nex
