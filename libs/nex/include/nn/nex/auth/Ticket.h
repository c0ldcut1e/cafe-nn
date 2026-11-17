/**
 * @file
 * @brief This is what the authentication server hands back, decrypted with the account's derived key.
 */

#pragma once

#include <nn/nex/core/Buffer.h>
#include <nn/nex/types.h>

namespace nn::nex
{

    /**
     * @brief This is what the authentication server hands back, decrypted with the account's derived key.
     */
    struct Ticket
    {
        Buffer sessionKey;
        qUnsignedInt32 targetPrincipalId = 0;
        Buffer internalTicket;

        bool Parse(const Buffer &decrypted);
    };

} // namespace nn::nex
