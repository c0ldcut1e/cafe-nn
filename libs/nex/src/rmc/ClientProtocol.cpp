#include <nn/nex/rmc/ClientProtocol.h>
#include <nn/nex/rmc/Message.h>
#include <nn/nex/runtime/ProtocolCallContext.h>
#include <nn/nex/types.h>

namespace nn::nex
{

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
