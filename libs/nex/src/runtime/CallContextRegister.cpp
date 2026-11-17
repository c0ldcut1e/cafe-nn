#include <map>
#include <nn/nex/runtime/CallContext.h>
#include <nn/nex/runtime/CallContextRegister.h>
#include <nn/nex/types.h>

namespace nn::nex
{

    namespace
    {

        std::map<qUnsignedInt32, CallContext *> &Registry()
        {
            static std::map<qUnsignedInt32, CallContext *> registry;
            return registry;
        }

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

} // namespace nn::nex
