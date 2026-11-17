/**
 * @file
 * @brief This is the base of every group of players the server tracks. Its ID, often called the gid, is what most matchmaking calls take.
 */

#pragma once

#include <map>
#include <nn/nex/core/String.h>
#include <nn/nex/rmc/Data.h>
#include <nn/nex/rmc/Message.h>
#include <nn/nex/types.h>

namespace nn::nex
{

    /**
     * @brief This is the base of every group of players the server tracks. Its ID, often called the gid, is what most matchmaking calls take.
     */
    class Gathering : public Data
    {
    public:
        Gathering()           = default;
        ~Gathering() override = default;

        qUnsignedInt32 GetID() const
        {
            return m_id;
        }
        qUnsignedInt32 GetOwnerPID() const
        {
            return m_ownerPid;
        }
        qUnsignedInt32 GetHostPID() const
        {
            return m_hostPid;
        }
        qUnsignedInt16 GetMinParticipants() const
        {
            return m_minParticipants;
        }
        qUnsignedInt16 GetMaxParticipants() const
        {
            return m_maxParticipants;
        }
        qUnsignedInt32 GetParticipationPolicy() const
        {
            return m_participationPolicy;
        }
        qUnsignedInt32 GetPolicyArgument() const
        {
            return m_policyArgument;
        }
        qUnsignedInt32 GetFlags() const
        {
            return m_flags;
        }
        qUnsignedInt32 GetState() const
        {
            return m_state;
        }
        const String &GetDescription() const
        {
            return m_description;
        }

        void SetID(qUnsignedInt32 v)
        {
            m_id = v;
        }
        void SetMinParticipants(qUnsignedInt16 v)
        {
            m_minParticipants = v;
        }
        void SetMaxParticipants(qUnsignedInt16 v)
        {
            m_maxParticipants = v;
        }
        void SetParticipationPolicy(qUnsignedInt32 v)
        {
            m_participationPolicy = v;
        }
        void SetPolicyArgument(qUnsignedInt32 v)
        {
            m_policyArgument = v;
        }
        void SetFlags(qUnsignedInt32 v)
        {
            m_flags = v;
        }
        void SetDescription(const String &v)
        {
            m_description = v;
        }

        const char *GetDataTypeName() const override
        {
            return "Gathering";
        }
        Data *Clone() const override
        {
            return new Gathering(*this);
        }
        void StreamOut(Message &m) const override;
        bool StreamIn(Message &m) override;

    protected:
        qUnsignedInt32 m_id                  = 0;
        qUnsignedInt32 m_ownerPid            = 0;
        qUnsignedInt32 m_hostPid             = 0;
        qUnsignedInt16 m_minParticipants     = 0;
        qUnsignedInt16 m_maxParticipants     = 0;
        qUnsignedInt32 m_participationPolicy = 0;
        qUnsignedInt32 m_policyArgument      = 0;
        qUnsignedInt32 m_flags               = 0;
        qUnsignedInt32 m_state               = 0;
        String m_description;
    };

} // namespace nn::nex
