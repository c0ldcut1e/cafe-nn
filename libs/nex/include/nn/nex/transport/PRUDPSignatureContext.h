/**
 * @file
 * @brief This holds what both peers derive the per-packet HMAC from, and it changes once the secure session key is known.
 */

#pragma once

#include <nn/nex/core/Buffer.h>
#include <nn/nex/types.h>

namespace nn::nex
{

    /**
     * @brief This holds what both peers derive the per-packet HMAC from, and it changes once the secure session key is known.
     */
    struct PRUDPSignatureContext
    {
        qUnsignedInt8 accessKeyHash[16] = {};
        qUnsignedInt32 accessKeySum     = 0;
        Buffer sessionKey;

        /**
         * @brief Syn and Connect are signed with the peer's connection signature, so the caller fills this in per direction.
         */
        qUnsignedInt8 connectionSignature[16] = {};

        void SetAccessKey(const char *accessKey);
    };

} // namespace nn::nex
