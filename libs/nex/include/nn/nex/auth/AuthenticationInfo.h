/**
 * @file
 * @brief LoginEx sends this as its extra data. It carries the NEX token and the NEX version the game was built against.
 */

#pragma once

#include <nn/nex/core/String.h>
#include <nn/nex/rmc/Data.h>
#include <nn/nex/rmc/Message.h>
#include <nn/nex/types.h>
#include <vector>

namespace nn::nex
{

    /**
     * @brief LoginEx sends this as its extra data. It carries the NEX token and the NEX version the game was built against.
     */
    class AuthenticationInfo : public Data
    {
    public:
        String token;
        qUnsignedInt32 ngsVersion    = 3;
        qByte tokenType              = 1;
        qUnsignedInt32 serverVersion = 0;

        const char *GetDataTypeName() const override
        {
            return "AuthenticationInfo";
        }
        Data *Clone() const override
        {
            return new AuthenticationInfo(*this);
        }
        void StreamOut(Message &m) const override;
    };

} // namespace nn::nex
