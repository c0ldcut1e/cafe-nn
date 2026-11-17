/**
 * @file
 * @brief This is a free-form map of named values that newer matchmaking calls send alongside sessions.
 */

#pragma once

#include <map>
#include <nn/nex/core/String.h>
#include <nn/nex/rmc/Data.h>
#include <nn/nex/rmc/Message.h>
#include <nn/nex/rmc/Variant.h>

namespace nn::nex
{

    /**
     * @brief This is a free-form map of named values that newer matchmaking calls send alongside sessions.
     */
    class MatchmakeParam : public Data
    {
    public:
        std::map<String, Variant> params;

        const char *GetDataTypeName() const override
        {
            return "MatchmakeParam";
        }
        Data *Clone() const override
        {
            return new MatchmakeParam(*this);
        }
        void StreamOut(Message &m) const override;
        bool StreamIn(Message &m) override;
    };

} // namespace nn::nex
