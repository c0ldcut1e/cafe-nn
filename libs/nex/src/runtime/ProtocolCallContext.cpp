#include <map>
#include <nn/nex/rmc/Message.h>
#include <nn/nex/runtime/ProtocolCallContext.h>
#include <nn/nex/types.h>

namespace nn::nex
{

    ProtocolCallContext::~ProtocolCallContext()
    {
        if (GetState() == CallInProgress)
            Cancel();
    }

    void ProtocolCallContext::OnCancel()
    {
        if (m_cancel)
            m_cancel();
        m_cancel  = nullptr;
        m_handler = nullptr;
    }

    void ProtocolCallContext::HandleResponse(qResult result, Message *body)
    {
        ResponseHandler handler = std::move(m_handler);
        m_handler               = nullptr;
        m_cancel                = nullptr;
        if (!result.IsSuccess())
        {
            SignalFailure(result);
            return;
        }
        if (handler && body)
        {
            qResult parsed = handler(*body);
            if (!parsed.IsSuccess())
            {
                SignalFailure(parsed);
                return;
            }
        }
        SignalSuccess(result);
    }

} // namespace nn::nex
