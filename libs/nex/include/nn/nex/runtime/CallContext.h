/**
 * @file
 * @brief This tracks the state and outcome of one asynchronous operation. The caller keeps it alive until it completes, and it can be
 * reused after that.
 */

#pragma once

#include <functional>
#include <nn/nex/runtime/Callback.h>
#include <nn/nex/types.h>
#include <vector>

namespace nn::nex
{

    /**
     * @brief This tracks the state and outcome of one asynchronous operation. The caller keeps it alive until it completes, and it can be reused after that.
     */
    class CallContext
    {
    public:
        enum State
        {
            Available      = 0,
            CallInProgress = 1,
            CallSuccess    = 2,
            CallFailure    = 3,
            CallCancelled  = 4
        };

        CallContext();
        virtual ~CallContext();
        CallContext(const CallContext &)            = delete;
        CallContext &operator=(const CallContext &) = delete;

        State GetState() const
        {
            return m_state;
        }
        qResult GetOutcome() const
        {
            return m_outcome;
        }
        bool IsInProgress() const
        {
            return m_state == CallInProgress;
        }

        /**
         * @brief This pumps the scheduler itself, so it is safe to call from the game's only thread.
         *
         * @param timeout Milliseconds to wait before giving up.
         * @return Whether the context completed before the timeout.
         */
        bool Wait(qUnsignedInt32 timeout = 0xFFFFFFFF);
        bool Cancel(State state = CallCancelled);
        bool Reset();
        void SignalSuccess(qResult result = qResult(true));
        void SignalFailure(qResult result);

        /**
         * @brief An owned callback is deleted when the context resets, and a front callback runs before the others.
         *
         * @param callback Runs once when the context completes.
         * @param owned Whether the context deletes callback when it resets.
         * @param front Whether callback runs before those already registered.
         */
        void RegisterCompletionCallback(CallbackRoot *callback, bool owned = false, bool front = false);

        bool InitiateCall();
        qUnsignedInt32 GetID() const
        {
            return m_id;
        }

    protected:
        virtual void OnCancel()
        {}

    private:
        void Complete(State state, qResult result);
        void ReleaseCallbacks();

        struct Registered
        {
            CallbackRoot *callback;
            bool owned;
        };

        State m_state = Available;
        qResult m_outcome;
        qUnsignedInt32 m_id;
        std::vector<Registered> m_callbacks;
    };

} // namespace nn::nex
