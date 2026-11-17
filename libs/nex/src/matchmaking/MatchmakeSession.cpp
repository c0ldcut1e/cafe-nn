#include "MatchMakingHelpers.h"
#include <nn/nex/core/Buffer.h>
#include <nn/nex/core/DateTime.h>
#include <nn/nex/core/String.h>
#include <nn/nex/matchmaking/Gathering.h>
#include <nn/nex/matchmaking/MatchmakeDefs.h>
#include <nn/nex/matchmaking/MatchmakeParam.h>
#include <nn/nex/matchmaking/MatchmakeSession.h>
#include <nn/nex/rmc/Data.h>
#include <nn/nex/rmc/Message.h>
#include <nn/nex/types.h>

namespace nn::nex
{

    void MatchmakeSession::Reset()
    {
        *(Gathering *) this = Gathering();
        m_gameMode          = 0;
        m_attributes.assign(6, 0);
        m_openParticipation     = true;
        m_matchmakeSystemType   = MatchmakeSystemTypeInvalid;
        m_applicationBuffer     = Buffer();
        m_participationCount    = 0;
        m_progressScore         = 100;
        m_sessionKey            = Buffer();
        m_option0               = 0;
        m_matchmakeParam        = MatchmakeParam();
        m_startedTime           = DateTime();
        m_userPassword          = String();
        m_referGid              = 0;
        m_userPasswordEnabled   = false;
        m_systemPasswordEnabled = false;
    }

    void MatchmakeSession::SetAttribute(qUnsignedInt32 index, qUnsignedInt32 value)
    {
        if (index >= m_attributes.size())
            m_attributes.resize(index + 1, 0);
        m_attributes[index] = value;
    }

    void MatchmakeSession::SetMatchmakeSystemType(MatchmakeSystemType type, qUnsignedInt32 argument)
    {
        m_matchmakeSystemType = type;
        // We put the argument in the participation policy because the server checks friend and invite access there
        if (type == MatchmakeSystemTypeFriend || type == MatchmakeSystemTypeFriendInvite)
        {
            m_participationPolicy = 98;
            m_policyArgument      = argument;
        }
    }

    void MatchmakeSession::StreamOut(Message &m) const
    {
        Gathering::StreamOut(m);
        size_t s = m.BeginStructure();
        m.WriteU32(m_gameMode);
        AddU32s(m, m_attributes);
        m.WriteBool(m_openParticipation);
        m.WriteU32(m_matchmakeSystemType);
        m.WriteBuffer(m_applicationBuffer);
        m.WriteU32(m_participationCount);
        m.WriteU8(m_progressScore);
        m.WriteBuffer(m_sessionKey);
        m.WriteU32(m_option0);
        m_matchmakeParam.StreamOut(m);
        m.WriteDateTime(m_startedTime);
        m.WriteString(m_userPassword);
        m.WriteU32(m_referGid);
        m.WriteBool(m_userPasswordEnabled);
        m.WriteBool(m_systemPasswordEnabled);
        m.EndStructure(s);
    }

    bool MatchmakeSession::StreamIn(Message &m)
    {
        if (!Gathering::StreamIn(m))
            return false;
        size_t end;
        if (!m.BeginExtractStructure(&end))
            return false;
        m_gameMode = m.ReadU32();
        if (!ExtractU32s(m, &m_attributes))
            return false;
        m_openParticipation   = m.ReadBool();
        m_matchmakeSystemType = m.ReadU32();
        m_applicationBuffer   = m.ReadBuffer();
        m_participationCount  = m.ReadU32();
        // Older servers end the structure here, so every later field is optional
        if (m.GetCursor() < end)
        {
            m_progressScore = m.ReadU8();
            m_sessionKey    = m.ReadBuffer();
        }
        if (m.GetCursor() < end)
        {
            m_option0 = m.ReadU32();
            if (!m_matchmakeParam.StreamIn(m))
                return false;
            m_startedTime           = m.ReadDateTime();
            m_userPassword          = m.ReadString();
            m_referGid              = m.ReadU32();
            m_userPasswordEnabled   = m.ReadBool();
            m_systemPasswordEnabled = m.ReadBool();
        }
        m.EndExtractStructure(end);
        return m.IsValid();
    }

    static DataFactoryRegistration<MatchmakeSession> s_matchmakeSession("MatchmakeSession");

} // namespace nn::nex
