/**
 * @file
 * @brief This is a search filter for BrowseMatchmakeSession and AutoMatchmake. Each attribute is a value or a range written as a string,
 * and an empty string matches anything.
 */

#pragma once

#include <map>
#include <nn/nex/core/String.h>
#include <nn/nex/matchmaking/MatchmakeDefs.h>
#include <nn/nex/matchmaking/MatchmakeParam.h>
#include <nn/nex/rmc/Data.h>
#include <nn/nex/rmc/Message.h>
#include <nn/nex/types.h>

namespace nn::nex
{

    /**
     * @brief This is a search filter for BrowseMatchmakeSession and AutoMatchmake. Each attribute is a value or a range written as a string, and an empty string matches anything.
     */
    class MatchmakeSessionSearchCriteria : public Data
    {
    public:
        MatchmakeSessionSearchCriteria()
        {
            Reset();
        }

        void Reset();
        void SetAttribute(qUnsignedInt32 index, const qVector<qUnsignedInt32> &values);
        void SetAttributeWithRange(qUnsignedInt32 index, qUnsignedInt32 min, qUnsignedInt32 max);
        void SetGameMode(qUnsignedInt32 v);
        void SetMinParticipants(qUnsignedInt16 v);
        void SetMinParticipantsWithRange(qUnsignedInt16 min, qUnsignedInt16 max);
        void SetMaxParticipants(qUnsignedInt16 v);
        void SetMaxParticipantsWithRange(qUnsignedInt16 min, qUnsignedInt16 max);
        void SetMatchmakeSystemType(MatchmakeSystemType v);
        void SetVacantOnly(bool v)
        {
            m_vacantOnly = v;
        }
        void SetExcludeLocked(bool v)
        {
            m_excludeLocked = v;
        }
        void SetExcludeNonHostPid(bool v)
        {
            m_excludeNonHostPid = v;
        }
        void SetExcludeUserPasswordSet(bool v)
        {
            m_excludeUserPasswordSet = v;
        }
        void SetSelectionMethod(MatchmakeSelectionMethod method, const MatchmakeParam &param);

        const char *GetDataTypeName() const override
        {
            return "MatchmakeSessionSearchCriteria";
        }
        Data *Clone() const override
        {
            return new MatchmakeSessionSearchCriteria(*this);
        }
        void StreamOut(Message &m) const override;

    private:
        qVector<String> m_attribs;
        String m_gameMode;
        String m_minParticipants;
        String m_maxParticipants;
        String m_matchmakeSystemType;
        bool m_vacantOnly;
        bool m_excludeLocked;
        bool m_excludeNonHostPid;
        qUnsignedInt32 m_selectionMethod;
        qUnsignedInt16 m_vacantParticipants;
        MatchmakeParam m_matchmakeParam;
        bool m_excludeUserPasswordSet;
        bool m_excludeSystemPasswordSet;
        qUnsignedInt32 m_referGid;
    };

} // namespace nn::nex
