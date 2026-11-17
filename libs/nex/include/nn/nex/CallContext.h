/**
 * @file
 * @brief Call contexts track one asynchronous call from start to completion. Every protocol method takes one, returns at once, and completes later on the thread that calls Scheduler::Dispatch.
 */

#pragma once

#include <functional>
#include <nn/nex/types.h>
#include <vector>

namespace nn::nex
{

    class Message;
    class CallContext;

    /**
     * @brief This is the interface a CallContext runs when it completes.
     */
    class CallbackRoot
    {
    public:
        virtual ~CallbackRoot()                 = default;
        virtual void Call(CallContext *context) = 0;
    };

    /**
     * @brief This adapts a plain function and a user pointer, which is how C games usually register completion callbacks.
     */
    template<typename T>
    class Callback : public CallbackRoot
    {
    public:
        using Function = void (*)(CallContext *, T);
        Callback(Function function, T argument) : m_function(function), m_argument(argument)
        {}
        void Call(CallContext *context) override
        {
            m_function(context, m_argument);
        }

    private:
        Function m_function;
        T m_argument;
    };

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

    class Credentials;

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

    /**
     * @brief This keeps every outstanding call by ID, so responses and timeouts can find their context.
     */
    class CallContextRegister
    {
    public:
        static CallContext *GetCallContext(qUnsignedInt32 id);
        static void Add(CallContext *context);
        static void Remove(CallContext *context);
    };

} // namespace nn::nex
