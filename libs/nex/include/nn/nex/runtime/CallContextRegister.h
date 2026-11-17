/**
 * @file
 * @brief This keeps every outstanding call by ID, so responses and timeouts can find their context.
 */

#pragma once

#include <functional>
#include <nn/nex/runtime/CallContext.h>
#include <nn/nex/types.h>
#include <vector>

namespace nn::nex
{

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
