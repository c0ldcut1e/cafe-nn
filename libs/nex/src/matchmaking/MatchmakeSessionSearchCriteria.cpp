#include <nn/nex/core/String.h>
#include <nn/nex/matchmaking/MatchmakeDefs.h>
#include <nn/nex/matchmaking/MatchmakeParam.h>
#include <nn/nex/matchmaking/MatchmakeSessionSearchCriteria.h>
#include <nn/nex/rmc/Message.h>
#include <nn/nex/types.h>

namespace nn::nex
{

    namespace
    {

        String Number(qUnsignedInt32 v)
        {
            wchar_t buf[16];
            swprintf(buf, 16, L"%u", v);
            return String(buf);
        }

        String Range(qUnsignedInt32 min, qUnsignedInt32 max)
        {
            wchar_t buf[32];
            swprintf(buf, 32, L"%u,%u", min, max);
            return String(buf);
        }

    } // namespace

    void MatchmakeSessionSearchCriteria::Reset()
    {
        m_attribs.assign(6, String());
        m_gameMode                 = String();
        m_minParticipants          = String();
        m_maxParticipants          = String();
        m_matchmakeSystemType      = String();
        m_vacantOnly               = true;
        m_excludeLocked            = true;
        m_excludeNonHostPid        = false;
        m_selectionMethod          = MatchmakeSelectionMethodRandom;
        m_vacantParticipants       = 1;
        m_matchmakeParam           = MatchmakeParam();
        m_excludeUserPasswordSet   = false;
        m_excludeSystemPasswordSet = false;
        m_referGid                 = 0;
    }

    void MatchmakeSessionSearchCriteria::SetAttribute(qUnsignedInt32 index, const qVector<qUnsignedInt32> &values)
    {
        if (index >= m_attribs.size())
            m_attribs.resize(index + 1);
        std::wstring joined;
        for (size_t i = 0; i < values.size(); i++)
        {
            if (i)
                joined += L"|";
            joined += Number(values[i]).CStr();
        }
        m_attribs[index] = String(joined.c_str());
    }

    void MatchmakeSessionSearchCriteria::SetAttributeWithRange(qUnsignedInt32 index, qUnsignedInt32 min, qUnsignedInt32 max)
    {
        if (index >= m_attribs.size())
            m_attribs.resize(index + 1);
        m_attribs[index] = Range(min, max);
    }

    void MatchmakeSessionSearchCriteria::SetGameMode(qUnsignedInt32 v)
    {
        m_gameMode = Number(v);
    }

    void MatchmakeSessionSearchCriteria::SetMinParticipants(qUnsignedInt16 v)
    {
        m_minParticipants = Number(v);
    }

    void MatchmakeSessionSearchCriteria::SetMaxParticipants(qUnsignedInt16 v)
    {
        m_maxParticipants = Number(v);
    }

    void MatchmakeSessionSearchCriteria::SetMatchmakeSystemType(MatchmakeSystemType v)
    {
        m_matchmakeSystemType = Number(v);
    }

    void MatchmakeSessionSearchCriteria::SetMinParticipantsWithRange(qUnsignedInt16 min, qUnsignedInt16 max)
    {
        m_minParticipants = Range(min, max);
    }

    void MatchmakeSessionSearchCriteria::SetMaxParticipantsWithRange(qUnsignedInt16 min, qUnsignedInt16 max)
    {
        m_maxParticipants = Range(min, max);
    }

    void MatchmakeSessionSearchCriteria::SetSelectionMethod(MatchmakeSelectionMethod method, const MatchmakeParam &param)
    {
        m_selectionMethod = method;
        m_matchmakeParam  = param;
    }

    void MatchmakeSessionSearchCriteria::StreamOut(Message &m) const
    {
        size_t s = m.BeginStructure();
        m.AddList(m_attribs,
                  [](Message &e, const String &v)
                  {
                      e.WriteString(v);
                  });
        m.WriteString(m_gameMode);
        m.WriteString(m_minParticipants);
        m.WriteString(m_maxParticipants);
        m.WriteString(m_matchmakeSystemType);
        m.WriteBool(m_vacantOnly);
        m.WriteBool(m_excludeLocked);
        m.WriteBool(m_excludeNonHostPid);
        m.WriteU32(m_selectionMethod);
        m.WriteU16(m_vacantParticipants);
        m_matchmakeParam.StreamOut(m);
        m.WriteBool(m_excludeUserPasswordSet);
        m.WriteBool(m_excludeSystemPasswordSet);
        m.WriteU32(m_referGid);
        m.EndStructure(s);
    }

} // namespace nn::nex
