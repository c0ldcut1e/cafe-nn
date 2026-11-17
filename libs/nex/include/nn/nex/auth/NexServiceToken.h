/**
 * @file
 * @brief This is what the console's account service returns for one game server, everything Login needs to reach it.
 */

#pragma once

#include <nn/nex/core/String.h>
#include <nn/nex/types.h>
#include <vector>

namespace nn::nex
{

    /**
     * @brief This is what the console's account service returns for one game server, everything Login needs to reach it.
     */
    struct NexServiceToken
    {
        String token;
        String nexPassword;
        qUnsignedInt32 principalId = 0;
        String host;
        qUnsignedInt16 port = 0;
    };

} // namespace nn::nex
