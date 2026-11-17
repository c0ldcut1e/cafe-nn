#include <nn/pia/common/ModuleStackChecker.h>

namespace nn::pia::common
{
    namespace
    {
        ModuleStackChecker::ModuleType s_stack[ModuleStackChecker::StackSize];
        u32 s_count;
    } // namespace

    s32 ModuleStackChecker::GetOrder(ModuleType type)
    {
        // A module may only start on top of modules from the same layer or a lower one. Negative orders are types that don't exist.
        static constexpr s8 s_orders[ModuleType_Max] = {-1, -1, 0, 2, 3, 4, 2, 5, -1, -1, -1, 4, 4, 4, -1, 1, 2, 6, 2, 2, -1, -1, 2};
        if ((u32) type >= ModuleType_Max)
            return -1;
        return s_orders[type];
    }

    Result ModuleStackChecker::Register(ModuleType type)
    {
        if (GetOrder(type) < 0)
            return ResultInvalidArgument();
        if (s_count >= StackSize)
            return ResultInvalidState();
        if (s_count == 0)
        {
            if (type != ModuleType_Common)
                return ResultInvalidState();
        }
        else if (GetOrder(type) < GetOrder(s_stack[s_count - 1]))
        {
            return ResultInvalidState();
        }
        s_stack[s_count++] = type;
        return ResultSuccess();
    }

    Result ModuleStackChecker::Unregister(ModuleType type)
    {
        if (GetOrder(type) < 0)
            return ResultInvalidArgument();
        if (s_count == 0 || s_stack[s_count - 1] != type)
            return ResultInvalidState();
        s_count--;
        return ResultSuccess();
    }

    bool ModuleStackChecker::IsRegistered(ModuleType type)
    {
        for (u32 i = 0; i < s_count; i++)
        {
            if (s_stack[i] == type)
                return true;
        }
        return false;
    }

    u32 ModuleStackChecker::GetCount()
    {
        return s_count;
    }
} // namespace nn::pia::common
