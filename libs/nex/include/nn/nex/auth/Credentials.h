/**
 * @file
 * @brief This holds the PID, ticket and session key we present to the secure server.
 */

#pragma once

#include <nn/nex/types.h>

namespace nn::nex
{

    /**
     * @brief This holds the PID, ticket and session key we present to the secure server.
     */
    class Credentials
    {
    public:
        Credentials() = default;
        explicit Credentials(qUnsignedInt32 principalId) : m_principalId(principalId)
        {}
        qUnsignedInt32 GetPrincipalID() const
        {
            return m_principalId;
        }
        void SetPrincipalID(qUnsignedInt32 principalId)
        {
            m_principalId = principalId;
        }

    private:
        qUnsignedInt32 m_principalId = 0;
    };

} // namespace nn::nex
