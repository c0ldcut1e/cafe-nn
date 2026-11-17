/**
 * @file
 * @brief RegisterEx sends this so the secure server can check the NEX token again.
 */

#pragma once

#include <nn/nex/core/String.h>
#include <nn/nex/rmc/Data.h>
#include <nn/nex/rmc/Message.h>
#include <vector>

namespace nn::nex
{

    /**
     * @brief RegisterEx sends this so the secure server can check the NEX token again.
     */
    class NintendoLoginData : public Data
    {
    public:
        String token;

        const char *GetDataTypeName() const override
        {
            return "NintendoLoginData";
        }
        Data *Clone() const override
        {
            return new NintendoLoginData(*this);
        }
        void StreamOut(Message &m) const override;
    };

} // namespace nn::nex
