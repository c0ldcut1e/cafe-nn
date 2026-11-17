/**
 * @file
 * @brief This is a gathering that outlives its sessions, such as a community. Sessions are created inside it.
 */

#pragma once

#include <map>
#include <nn/nex/core/Buffer.h>
#include <nn/nex/core/DateTime.h>
#include <nn/nex/core/String.h>
#include <nn/nex/matchmaking/Gathering.h>
#include <nn/nex/matchmaking/MatchmakeDefs.h>
#include <nn/nex/rmc/Data.h>
#include <nn/nex/rmc/Message.h>
#include <nn/nex/types.h>

namespace nn::nex
{

    /**
     * @brief This is a gathering that outlives its sessions, such as a community. Sessions are created inside it.
     */
    class PersistentGathering : public Gathering
    {
    public:
        PersistentGathering()
        {
            Reset();
        }

        void Reset();
        qUnsignedInt32 GetAttribute(qUnsignedInt32 index) const
        {
            return index < m_attribs.size() ? m_attribs[index] : 0;
        }
        void SetAttribute(qUnsignedInt32 index, qUnsignedInt32 value);
        PersistentGatheringType::PersistentGatheringType GetPersistentGatheringType() const
        {
            return (PersistentGatheringType::PersistentGatheringType) m_communityType;
        }
        void SetPersistentGatheringType(PersistentGatheringType::PersistentGatheringType v)
        {
            m_communityType = v;
        }
        void SetPassword(const String &v)
        {
            m_password = v;
        }
        void SetApplicationBuffer(const qVector<qByte> &v)
        {
            m_applicationBuffer = Buffer(v.data(), v.size());
        }
        void SetParticipationPeriod(const DateTime &start, const DateTime &end)
        {
            m_startDate = start;
            m_endDate   = end;
        }

        const Buffer &GetApplicationBuffer() const
        {
            return m_applicationBuffer;
        }
        qUnsignedInt32 GetMatchmakeSessionCount() const
        {
            return m_matchmakeSessionCount;
        }
        qUnsignedInt32 GetParticipationCount() const
        {
            return m_participationCount;
        }

        const char *GetDataTypeName() const override
        {
            return "PersistentGathering";
        }
        Data *Clone() const override
        {
            return new PersistentGathering(*this);
        }
        void StreamOut(Message &m) const override;
        bool StreamIn(Message &m) override;

    private:
        qUnsignedInt32 m_communityType;
        String m_password;
        qVector<qUnsignedInt32> m_attribs;
        Buffer m_applicationBuffer;
        DateTime m_startDate;
        DateTime m_endDate;
        qUnsignedInt32 m_matchmakeSessionCount;
        qUnsignedInt32 m_participationCount;
    };

} // namespace nn::nex
