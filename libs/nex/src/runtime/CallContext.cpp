#include <map>
#include <nn/nex/CallContext.h>
#include <nn/nex/Message.h>
#include <nn/nex/Scheduler.h>

namespace nn::nex
{

    namespace
    {

        std::map<qUnsignedInt32, CallContext *> &Registry()
        {
            static std::map<qUnsignedInt32, CallContext *> registry;
            return registry;
        }

        qUnsignedInt32 g_nextContextId = 1;

    } // namespace

    CallContext *CallContextRegister::GetCallContext(qUnsignedInt32 id)
    {
        auto it = Registry().find(id);
        return it == Registry().end() ? nullptr : it->second;
    }

    void CallContextRegister::Add(CallContext *context)
    {
        Registry()[context->GetID()] = context;
    }
    void CallContextRegister::Remove(CallContext *context)
    {
        Registry().erase(context->GetID());
    }

    CallContext::CallContext() : m_id(g_nextContextId++)
    {
        CallContextRegister::Add(this);
    }

    CallContext::~CallContext()
    {
        // A context destroyed mid-call must not leave a pending response pointing at freed memory
        if (m_state == CallInProgress)
            Cancel();
        ReleaseCallbacks();
        CallContextRegister::Remove(this);
    }

    bool CallContext::InitiateCall()
    {
        if (m_state == CallInProgress)
            return false;
        m_state   = CallInProgress;
        m_outcome = qResult(true);
        return true;
    }

    bool CallContext::Wait(qUnsignedInt32 timeout)
    {
        TimeValue deadline = Time::ConvertTimeoutToDeadline(timeout);
        while (m_state == CallInProgress)
        {
            if (timeout != 0xFFFFFFFF && Time::GetTime() >= deadline)
                return false;
            Scheduler::GetInstance()->Dispatch(1);
        }
        return m_state == CallSuccess;
    }

    bool CallContext::Cancel(State state)
    {
        if (m_state != CallInProgress)
            return false;
        OnCancel();
        Complete(state, qResult(QResult::Core_Cancelled));
        return true;
    }

    bool CallContext::Reset()
    {
        if (m_state == CallInProgress)
            return false;
        m_state   = Available;
        m_outcome = qResult(true);
        ReleaseCallbacks();
        return true;
    }

    void CallContext::SignalSuccess(qResult result)
    {
        Complete(CallSuccess, result);
    }

    void CallContext::SignalFailure(qResult result)
    {
        // A failure reported with a success code would look like success to the game, so we force the failure bit
        Complete(CallFailure, result.IsSuccess() ? qResult(QResult::Core_Unknown) : result);
    }

    void CallContext::RegisterCompletionCallback(CallbackRoot *callback, bool owned, bool front)
    {
        if (front)
            m_callbacks.insert(m_callbacks.begin(), {callback, owned});
        else
            m_callbacks.push_back({callback, owned});
    }

    void CallContext::Complete(State state, qResult result)
    {
        if (m_state != CallInProgress)
            return;
        m_state   = state;
        m_outcome = result;
        // Callbacks may reset this context and register new ones, so we iterate over a snapshot
        std::vector<Registered> callbacks = m_callbacks;
        for (const Registered &r : callbacks) r.callback->Call(this);
    }

    void CallContext::ReleaseCallbacks()
    {
        for (const Registered &r : m_callbacks)
            if (r.owned)
                delete r.callback;
        m_callbacks.clear();
    }

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
