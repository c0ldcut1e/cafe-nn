/**
 * @file
 * @brief Every protocol client derives from this and sends its calls through the endpoint it is bound to. Bind it with
 * BackEndServices::RegisterProtocol after login.
 */

#pragma once

#include <nn/nex/rmc/EndPoint.h>
#include <nn/nex/rmc/Message.h>
#include <nn/nex/runtime/ProtocolCallContext.h>
#include <nn/nex/types.h>

namespace nn::nex
{

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
