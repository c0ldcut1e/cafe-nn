/**
 * @file
 * @brief This adapts a plain function and a user pointer, which is how C games usually register completion callbacks.
 */

#pragma once

#include <functional>
#include <nn/nex/runtime/CallContext.h>
#include <vector>

namespace nn::nex
{

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

} // namespace nn::nex
