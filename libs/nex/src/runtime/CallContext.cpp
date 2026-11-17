#include <map>
#include <nn/nex/platform/Time.h>
#include <nn/nex/runtime/CallContext.h>
#include <nn/nex/runtime/CallContextRegister.h>
#include <nn/nex/runtime/Callback.h>
#include <nn/nex/runtime/Scheduler.h>
#include <nn/nex/types.h>

namespace nn::nex
{

    namespace
    {

        qUnsignedInt32 g_nextContextId = 1;

    } // namespace

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

} // namespace nn::nex
