/**
 * @file
 * @brief We keep the decoding of the response with the context, so each client method stays one self-contained function.
 */

#pragma once

#include <functional>
#include <nn/nex/auth/Credentials.h>
#include <nn/nex/rmc/Message.h>
#include <nn/nex/runtime/CallContext.h>
#include <nn/nex/types.h>
#include <vector>

namespace nn::nex
{

    /**
     * @brief We keep the decoding of the response with the context, so each client method stays one self-contained function.
     */
    class ProtocolCallContext : public CallContext
    {
    public:
        using ResponseHandler = std::function<qResult(Message &)>;
        using CancelHandler   = std::function<void()>;

        explicit ProtocolCallContext(Credentials *credentials = nullptr) : m_credentials(credentials)
        {}
        ~ProtocolCallContext() override;

        Credentials *GetCredentials() const
        {
            return m_credentials;
        }

        void SetResponseHandler(ResponseHandler handler)
        {
            m_handler = std::move(handler);
        }
        void SetCancelHandler(CancelHandler handler)
        {
            m_cancel = std::move(handler);
        }
        void HandleResponse(qResult result, Message *body);

    protected:
        void OnCancel() override;

    private:
        Credentials *m_credentials;
        ResponseHandler m_handler;
        CancelHandler m_cancel;
    };

} // namespace nn::nex
