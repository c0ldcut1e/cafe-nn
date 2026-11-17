/**
 * @file
 * @brief This makes sure pia's modules start up and shut down in a fixed order. Each module registers on start up and unregisters on
 * shut down, so the stack always holds the running modules from the bottom layer up.
 */

#pragma once

#include <nn/pia/Result.h>

namespace nn::pia::common
{
    class ModuleStackChecker
    {
    public:
        enum ModuleType
        {
            ModuleType_Common = 2,
            ModuleType_Max    = 23,
        };

        static constexpr u32 StackSize = 0x17;

        /**
         * @brief Pushes a module that is starting up.
         *
         * @param type The module. The first one has to be ModuleType_Common, and every later one has to sit at the same layer or above the top.
         * @return InvalidArgument for an unknown type, and InvalidState when the stack is full or the order is wrong.
         */
        static Result Register(ModuleType type);

        /**
         * @brief Pops a module that is shutting down.
         *
         * @param type Has to be the module on top, so modules shut down in the reverse order they started.
         * @return InvalidArgument for an unknown type, and InvalidState when the stack is empty or the type is not on top.
         */
        static Result Unregister(ModuleType type);

        static bool IsRegistered(ModuleType type);
        static u32 GetCount();

    private:
        static s32 GetOrder(ModuleType type);
    };
} // namespace nn::pia::common
