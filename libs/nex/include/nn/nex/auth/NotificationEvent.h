/**
 * @file
 * @brief This is one event pushed by the secure server. The type says what happened, and the parameters are the PIDs or gids involved.
 */

#pragma once

#include <nn/nex/core/String.h>
#include <nn/nex/rmc/Data.h>
#include <nn/nex/rmc/Message.h>
#include <nn/nex/types.h>

namespace nn::nex
{

    /**
     * @brief This is one event pushed by the secure server. The type says what happened, and the parameters are the PIDs or gids involved.
     */
    class NotificationEvent : public Data
    {
    public:
        NotificationEvent() = default;

        qUnsignedInt32 GetSource() const
        {
            return m_pidSource;
        }
        qUnsignedInt32 GetType() const
        {
            return m_type;
        }

        /**
         * @brief The server encodes the event category in the thousands and the subtype below that.
         *
         * @return The type divided by 1000, which is one of the values below, such as ParticipationEvent.
         */
        qUnsignedInt32 GetMainType() const
        {
            return m_type / 1000;
        }
        qUnsignedInt32 GetSubType() const
        {
            return m_type % 1000;
        }
        qUnsignedInt32 GetParam1() const
        {
            return m_param1;
        }
        qUnsignedInt32 GetParam2() const
        {
            return m_param2;
        }
        qUnsignedInt32 GetParam3() const
        {
            return m_param3;
        }
        const String &GetStringParam() const
        {
            return m_strParam;
        }

        void SetSource(qUnsignedInt32 pid)
        {
            m_pidSource = pid;
        }
        void SetType(qUnsignedInt32 type)
        {
            m_type = type;
        }
        void SetParam1(qUnsignedInt32 v)
        {
            m_param1 = v;
        }
        void SetParam2(qUnsignedInt32 v)
        {
            m_param2 = v;
        }
        void SetParam3(qUnsignedInt32 v)
        {
            m_param3 = v;
        }
        void SetStringParam(const String &s)
        {
            m_strParam = s;
        }

        const char *GetDataTypeName() const override
        {
            return "NotificationEvent";
        }
        Data *Clone() const override
        {
            return new NotificationEvent(*this);
        }
        void StreamOut(Message &m) const override;
        bool StreamIn(Message &m) override;

    private:
        qUnsignedInt32 m_pidSource = 0;
        qUnsignedInt32 m_type      = 0;
        qUnsignedInt32 m_param1    = 0;
        qUnsignedInt32 m_param2    = 0;
        String m_strParam;
        qUnsignedInt32 m_param3 = 0;
    };

    namespace NotificationEvents
    {
        enum Type
        {
            ParticipationEvent    = 3,
            OwnershipChangeEvent  = 4,
            GameNotificationType1 = 101,
            GatheringUnregistered = 109,
            HostChangeEvent       = 110,
            SwitchGathering       = 122,
        };
    } // namespace NotificationEvents

} // namespace nn::nex
