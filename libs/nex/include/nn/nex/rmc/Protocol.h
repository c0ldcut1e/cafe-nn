/**
 * @file
 * @brief This is Protocol, which writes RMC method IDs into a Message.
 */

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <nn/nex/rmc/Message.h>
#include <nn/nex/types.h>

namespace nn::nex
{

    class Protocol
    {
    public:
        static void AddMethodID(Message *message, qUnsignedInt32 methodId)
        {
            message->WriteU32(methodId);
        }
    };

} // namespace nn::nex
