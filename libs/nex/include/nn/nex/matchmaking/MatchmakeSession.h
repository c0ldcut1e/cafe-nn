/**
 * @file
 * @brief This is a joinable game session with the attributes and game mode that searches match on.
 */

#pragma once

#include <map>
#include <nn/nex/core/Buffer.h>
#include <nn/nex/core/DateTime.h>
#include <nn/nex/core/String.h>
#include <nn/nex/matchmaking/Gathering.h>
#include <nn/nex/matchmaking/MatchmakeDefs.h>
#include <nn/nex/matchmaking/MatchmakeParam.h>
#include <nn/nex/rmc/Data.h>
#include <nn/nex/rmc/Message.h>
#include <nn/nex/types.h>

namespace nn::nex
{

    /**
     * @brief This is a joinable game session with the attributes and game mode that searches match on.
     */
    class MatchmakeSession : public Gathering
    {
    public:
        MatchmakeSession()
        {
            Reset();
        }

        void Reset();
        qUnsignedInt32 GetAttribute(qUnsignedInt32 index) const
        {
            return index < m_attributes.size() ? m_attributes[index] : 0;
        }
        void SetAttribute(qUnsignedInt32 index, qUnsignedInt32 value);
        void SetGameMode(qUnsignedInt32 v)
        {
            m_gameMode = v;
        }
        void SetMatchmakeSystemType(MatchmakeSystemType type, qUnsignedInt32 argument = 0);
        void SetOpenParticipation(bool v)
        {
            m_openParticipation = v;
        }
        void SetApplicationBuffer(const qVector<qByte> &v)
        {
            m_applicationBuffer = Buffer(v.data(), v.size());
        }
        void SetProgressScore(qByte v)
        {
            m_progressScore = v;
        }
        void SetUserPassword(const String &v)
        {
            m_userPassword = v;
        }
        void SetMatchmakeParam(const MatchmakeParam &v)
        {
            m_matchmakeParam = v;
        }

        qUnsignedInt32 GetGameMode() const
        {
            return m_gameMode;
        }
        bool GetOpenParticipation() const
        {
            return m_openParticipation;
        }
        qUnsignedInt32 GetMatchmakeSystemType() const
        {
            return m_matchmakeSystemType;
        }
        const Buffer &GetApplicationBuffer() const
        {
            return m_applicationBuffer;
        }
        qUnsignedInt32 GetParticipationCount() const
        {
            return m_participationCount;
        }
        qByte GetProgressScore() const
        {
            return m_progressScore;
        }
        const Buffer &GetSessionKey() const
        {
            return m_sessionKey;
        }
        const qVector<qUnsignedInt32> &GetAttributes() const
        {
            return m_attributes;
        }
        const DateTime &GetStartedTime() const
        {
            return m_startedTime;
        }

        const char *GetDataTypeName() const override
        {
            return "MatchmakeSession";
        }
        Data *Clone() const override
        {
            return new MatchmakeSession(*this);
        }
        void StreamOut(Message &m) const override;
        bool StreamIn(Message &m) override;

    private:
        qUnsignedInt32 m_gameMode;
        qVector<qUnsignedInt32> m_attributes;
        bool m_openParticipation;
        qUnsignedInt32 m_matchmakeSystemType;
        Buffer m_applicationBuffer;
        qUnsignedInt32 m_participationCount;
        qByte m_progressScore;
        Buffer m_sessionKey;
        qUnsignedInt32 m_option0;
        MatchmakeParam m_matchmakeParam;
        DateTime m_startedTime;
        String m_userPassword;
        qUnsignedInt32 m_referGid;
        bool m_userPasswordEnabled;
        bool m_systemPasswordEnabled;
    };

} // namespace nn::nex
