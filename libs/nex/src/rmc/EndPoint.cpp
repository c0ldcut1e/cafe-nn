#include <nn/nex/EndPoint.h>

namespace nn::nex
{

    EndPoint::EndPoint()
    {
        m_transport.SetReceiveHandler(
                [this](Buffer &data)
                {
                    OnReceive(data);
                });
        m_transport.SetDisconnectHandler(
                [this](qResult result)
                {
                    OnDisconnect(result);
                });
    }

    EndPoint::~EndPoint()
    {
        FailAll(qResult(QResult::Core_Cancelled));
        m_transport.SetDisconnectHandler(nullptr);
    }

    qResult EndPoint::Connect(CallContext *context, const StationURL &url, const char *accessKey, const Buffer &connectPayload,
                              const Buffer *sessionKey)
    {
        InetAddress address;
        String host = url.GetAddress();
        if (host.IsEmpty() || !address.SetAddress(host.CStr()))
            return qResult(QResult::Transport_DNSError);
        address.SetPortNumber(url.GetPortNumber());
        m_transport.SetAccessKey(accessKey);
        return m_transport.Connect(context, address, connectPayload, sessionKey);
    }

    void EndPoint::Close()
    {
        m_transport.Disconnect();
        FailAll(qResult(QResult::RendezVous_ConnectionDisconnected));
    }

    void EndPoint::FailAll(qResult result)
    {
        std::map<qUnsignedInt32, ProtocolCallContext *> pending;
        pending.swap(m_pending);
        for (auto &[id, context] : pending) context->HandleResponse(result, nullptr);
    }

    void EndPoint::OnDisconnect(qResult result)
    {
        FailAll(qResult(QResult::RendezVous_ConnectionDisconnected));
        if (m_onDisconnect)
            m_onDisconnect(result);
    }

    void EndPoint::WriteProtocolID(Message &m, qUnsignedInt16 protocolId, bool request)
    {
        // Identifiers that do not fit in seven bits are escaped with 0x7F and follow as a full 16 bit value
        if (protocolId < 0x7F)
        {
            m.WriteU8((qUnsignedInt8) protocolId | (request ? 0x80 : 0));
        }
        else
        {
            m.WriteU8(0x7F | (request ? 0x80 : 0));
            m.WriteU16(protocolId);
        }
    }

    bool EndPoint::Call(ProtocolCallContext *context, qUnsignedInt16 protocolId, qUnsignedInt32 methodId, const Message &parameters,
                        ProtocolCallContext::ResponseHandler handler)
    {
        if (!context || !context->InitiateCall())
            return false;
        if (!m_transport.IsConnected())
        {
            context->SignalFailure(qResult(QResult::RendezVous_ConnectionDisconnected));
            return false;
        }

        qUnsignedInt32 callId = m_nextCallId++;
        Message m;
        m.WriteU32(0);
        WriteProtocolID(m, protocolId, true);
        m.WriteU32(callId);
        Protocol::AddMethodID(&m, methodId);
        m.Write(parameters.GetData(), parameters.GetSize());
        m.PatchU32(0, (qUnsignedInt32) (m.GetSize() - 4));

        context->SetResponseHandler(std::move(handler));
        context->SetCancelHandler(
                [this, callId]()
                {
                    m_pending.erase(callId);
                });
        m_pending[callId] = context;

        qResult sent = m_transport.Send(Buffer(m.GetData(), m.GetSize()));
        if (!sent)
        {
            m_pending.erase(callId);
            context->HandleResponse(sent, nullptr);
            return false;
        }
        return true;
    }

    void EndPoint::RegisterRequestHandler(qUnsignedInt16 protocolId, RequestHandler handler)
    {
        if (handler)
            m_handlers[protocolId] = std::move(handler);
        else
            m_handlers.erase(protocolId);
    }

    void EndPoint::OnReceive(Buffer &data)
    {
        Message m(data.GetContentPtr(), data.GetContentSize());
        qUnsignedInt32 size = m.ReadU32();
        if (!m.IsValid() || size != m.GetRemaining())
            return;
        qUnsignedInt8 first       = m.ReadU8();
        bool request              = first & 0x80;
        qUnsignedInt16 protocolId = first & 0x7F;
        if (protocolId == 0x7F)
            protocolId = m.ReadU16();

        if (request)
        {
            qUnsignedInt32 callId   = m.ReadU32();
            qUnsignedInt32 methodId = m.ReadU32();
            if (!m.IsValid())
                return;
            Message body;
            qResult result(QResult::Core_NotImplemented);
            auto it = m_handlers.find(protocolId);
            if (it != m_handlers.end())
                result = it->second(methodId, m, body);

            Message response;
            response.WriteU32(0);
            WriteProtocolID(response, protocolId, false);
            if (result)
            {
                response.WriteU8(1);
                response.WriteU32(callId);
                response.WriteU32(methodId | 0x8000);
                response.Write(body.GetData(), body.GetSize());
            }
            else
            {
                response.WriteU8(0);
                response.WriteU32(result.GetReturnCode());
                response.WriteU32(callId);
            }
            response.PatchU32(0, (qUnsignedInt32) (response.GetSize() - 4));
            m_transport.Send(Buffer(response.GetData(), response.GetSize()));
            return;
        }

        bool success = m.ReadU8() != 0;
        qUnsignedInt32 callId;
        qResult result(true);
        if (success)
        {
            callId = m.ReadU32();
            m.ReadU32();
        }
        else
        {
            result = qResult(m.ReadU32() | 0x80000000);
            callId = m.ReadU32();
        }
        if (!m.IsValid())
            return;
        auto it = m_pending.find(callId);
        if (it == m_pending.end())
            return;
        ProtocolCallContext *context = it->second;
        m_pending.erase(it);
        context->HandleResponse(result, success ? &m : nullptr);
    }

    bool ClientProtocol::Call(ProtocolCallContext *context, qUnsignedInt32 methodId, const Message &parameters,
                              ProtocolCallContext::ResponseHandler handler)
    {
        if (!m_endPoint)
        {
            if (context && context->InitiateCall())
                context->SignalFailure(qResult(QResult::Core_InitializationFailure));
            return false;
        }
        return m_endPoint->Call(context, m_protocolId, methodId, parameters, std::move(handler));
    }

} // namespace nn::nex
