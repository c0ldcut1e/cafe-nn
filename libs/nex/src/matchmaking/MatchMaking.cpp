#include <nn/nex/MatchMaking.h>

namespace nn::nex
{

    namespace
    {

        void AddU32s(Message &m, const qVector<qUnsignedInt32> &v)
        {
            m.AddList(v,
                      [](Message &s, qUnsignedInt32 e)
                      {
                          s.WriteU32(e);
                      });
        }

        bool ExtractU32s(Message &m, qVector<qUnsignedInt32> *v)
        {
            return m.ExtractList(v,
                                 [](Message &s, qUnsignedInt32 &e)
                                 {
                                     e = s.ReadU32();
                                     return s.IsValid();
                                 });
        }

        void AddU32List(Message &m, const qList<qUnsignedInt32> &v)
        {
            m.AddList(v,
                      [](Message &s, qUnsignedInt32 e)
                      {
                          s.WriteU32(e);
                      });
        }

        bool ExtractGatherings(Message &m, qList<AnyObjectHolder<Gathering, String>> *out)
        {
            return m.ExtractList(out,
                                 [](Message &s, AnyObjectHolder<Gathering, String> &h)
                                 {
                                     return h.Extract(s);
                                 });
        }

        template<typename T>
        bool ExtractStructures(Message &m, qList<T> *out)
        {
            return m.ExtractList(out,
                                 [](Message &s, T &e)
                                 {
                                     return e.StreamIn(s);
                                 });
        }

        // Codes are the gathering IDs themselves because Nintendo's real scramble is unknown to us
        String g_codeKey;

    } // namespace

    void MatchmakeParam::StreamOut(Message &m) const
    {
        size_t s = m.BeginStructure();
        m.WriteU32((qUnsignedInt32) params.size());
        for (const auto &[key, value] : params)
        {
            m.WriteString(key);
            value.Add(m);
        }
        m.EndStructure(s);
    }

    bool MatchmakeParam::StreamIn(Message &m)
    {
        size_t end;
        if (!m.BeginExtractStructure(&end))
            return false;
        params.clear();
        qUnsignedInt32 count = m.ReadU32();
        for (qUnsignedInt32 i = 0; i < count && m.IsValid(); i++)
        {
            String key = m.ReadString();
            Variant value;
            if (!value.Extract(m))
                return false;
            params[key] = value;
        }
        m.EndExtractStructure(end);
        return m.IsValid();
    }

    void Gathering::StreamOut(Message &m) const
    {
        size_t s = m.BeginStructure();
        m.WriteU32(m_id);
        m.WriteU32(m_ownerPid);
        m.WriteU32(m_hostPid);
        m.WriteU16(m_minParticipants);
        m.WriteU16(m_maxParticipants);
        m.WriteU32(m_participationPolicy);
        m.WriteU32(m_policyArgument);
        m.WriteU32(m_flags);
        m.WriteU32(m_state);
        m.WriteString(m_description);
        m.EndStructure(s);
    }

    bool Gathering::StreamIn(Message &m)
    {
        size_t end;
        if (!m.BeginExtractStructure(&end))
            return false;
        m_id                  = m.ReadU32();
        m_ownerPid            = m.ReadU32();
        m_hostPid             = m.ReadU32();
        m_minParticipants     = m.ReadU16();
        m_maxParticipants     = m.ReadU16();
        m_participationPolicy = m.ReadU32();
        m_policyArgument      = m.ReadU32();
        m_flags               = m.ReadU32();
        m_state               = m.ReadU32();
        m_description         = m.ReadString();
        m.EndExtractStructure(end);
        return m.IsValid();
    }

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

    void CreateMatchmakeSessionParam::StreamOut(Message &m) const
    {
        size_t s = m.BeginStructure();
        sourceSession.StreamOut(m);
        AddU32s(m, additionalParticipants);
        m.WriteU32(gidForParticipationCheck);
        m.WriteU32(option);
        m.WriteString(joinMessage);
        m.WriteU16(participationCount);
        m.EndStructure(s);
    }

    void JoinMatchmakeSessionParam::Reset()
    {
        gid = 0;
        additionalParticipants.clear();
        gidForParticipationCheck = 0;
        option                   = 0;
        behavior                 = 0;
        userPassword             = String();
        systemPassword           = String();
        joinMessage              = String();
        participationCount       = 1;
    }

    void JoinMatchmakeSessionParam::StreamOut(Message &m) const
    {
        size_t s = m.BeginStructure();
        m.WriteU32(gid);
        AddU32s(m, additionalParticipants);
        m.WriteU32(gidForParticipationCheck);
        m.WriteU32(option);
        m.WriteU8(behavior);
        m.WriteString(userPassword);
        m.WriteString(systemPassword);
        m.WriteString(joinMessage);
        m.WriteU16(participationCount);
        m.EndStructure(s);
    }

    void AutoMatchmakeParam::StreamOut(Message &m) const
    {
        size_t s = m.BeginStructure();
        sourceSession.StreamOut(m);
        AddU32s(m, additionalParticipants);
        m.WriteU32(gidForParticipationCheck);
        m.WriteU32(option);
        m.WriteString(joinMessage);
        m.WriteU16(participationCount);
        m.AddList(searchCriteria,
                  [](Message &e, const MatchmakeSessionSearchCriteria &c)
                  {
                      c.StreamOut(e);
                  });
        AddU32s(m, targetGids);
        m.EndStructure(s);
    }

    void UpdateMatchmakeSessionParam::StreamOut(Message &m) const
    {
        size_t s = m.BeginStructure();
        m.WriteU32(gid);
        m.WriteU32(modificationFlag);
        AddU32s(m, attributes);
        m.WriteBool(openParticipation);
        m.WriteBuffer(applicationBuffer);
        m.WriteU8(progressScore);
        matchmakeParam.StreamOut(m);
        m.WriteDateTime(startedTime);
        m.WriteString(userPassword);
        m.WriteU32(gameMode);
        m.WriteString(description);
        m.WriteU16(minParticipants);
        m.WriteU16(maxParticipants);
        m.WriteU32(matchmakeSystemType);
        m.WriteU32(participationPolicy);
        m.WriteU32(policyArgument);
        m.WriteString(codeword);
        m.EndStructure(s);
    }

    void PersistentGathering::Reset()
    {
        *(Gathering *) this = Gathering();
        m_communityType     = PersistentGatheringType::Open;
        m_password          = String();
        m_attribs.assign(6, 0);
        m_applicationBuffer     = Buffer();
        m_startDate             = DateTime();
        m_endDate               = DateTime();
        m_matchmakeSessionCount = 0;
        m_participationCount    = 0;
    }

    void PersistentGathering::SetAttribute(qUnsignedInt32 index, qUnsignedInt32 value)
    {
        if (index >= m_attribs.size())
            m_attribs.resize(index + 1, 0);
        m_attribs[index] = value;
    }

    void PersistentGathering::StreamOut(Message &m) const
    {
        Gathering::StreamOut(m);
        size_t s = m.BeginStructure();
        m.WriteU32(m_communityType);
        m.WriteString(m_password);
        AddU32s(m, m_attribs);
        m.WriteBuffer(m_applicationBuffer);
        m.WriteDateTime(m_startDate);
        m.WriteDateTime(m_endDate);
        m.WriteU32(m_matchmakeSessionCount);
        m.WriteU32(m_participationCount);
        m.EndStructure(s);
    }

    bool PersistentGathering::StreamIn(Message &m)
    {
        if (!Gathering::StreamIn(m))
            return false;
        size_t end;
        if (!m.BeginExtractStructure(&end))
            return false;
        m_communityType = m.ReadU32();
        m_password      = m.ReadString();
        if (!ExtractU32s(m, &m_attribs))
            return false;
        m_applicationBuffer     = m.ReadBuffer();
        m_startDate             = m.ReadDateTime();
        m_endDate               = m.ReadDateTime();
        m_matchmakeSessionCount = m.ReadU32();
        m_participationCount    = m.ReadU32();
        m.EndExtractStructure(end);
        return m.IsValid();
    }

    const MatchmakeSession *PlayingSession::GetMatchmakeSession() const
    {
        Gathering *g = m_gathering.Get();
        // A string compare stands in for dynamic_cast because the console build has no RTTI
        if (g && String(g->GetDataTypeName()) == String("MatchmakeSession"))
            return (const MatchmakeSession *) g;
        return nullptr;
    }

    bool PlayingSession::StreamIn(Message &m)
    {
        size_t end;
        if (!m.BeginExtractStructure(&end))
            return false;
        m_principalId = m.ReadU32();
        if (!m_gathering.Extract(m))
            return false;
        m.EndExtractStructure(end);
        return m.IsValid();
    }

    static DataFactoryRegistration<Gathering> s_gathering("Gathering");
    static DataFactoryRegistration<MatchmakeSession> s_matchmakeSession("MatchmakeSession");
    static DataFactoryRegistration<PersistentGathering> s_persistentGathering("PersistentGathering");

    bool MatchMakingClient::UnregisterGathering(ProtocolCallContext *context, qUnsignedInt32 gid)
    {
        Message p;
        p.WriteU32(gid);
        return Call(context, 2, p,
                    [](Message &m)
                    {
                        return qResult(m.ReadBool() && m.IsValid());
                    });
    }

    bool MatchMakingClient::FindByID(ProtocolCallContext *context, qUnsignedInt32 gid, AnyObjectHolder<Gathering, String> *gathering)
    {
        Message p;
        p.WriteU32(gid);
        return Call(context, 21, p,
                    [=](Message &m)
                    {
                        bool found = m.ReadBool();
                        AnyObjectHolder<Gathering, String> holder;
                        if (!holder.Extract(m))
                            return qResult(QResult::Core_InvalidArgument);
                        if (gathering)
                            *gathering = holder;
                        return found ? qResult(true) : qResult(QResult::RendezVous_SessionVoid);
                    });
    }

    bool MatchMakingClient::FindByOwner(ProtocolCallContext *context, qUnsignedInt32 ownerPid, const ResultRange &range,
                                        qList<AnyObjectHolder<Gathering, String>> *gatherings)
    {
        Message p;
        p.WriteU32(ownerPid);
        range.Add(p);
        return Call(context, 22, p,
                    [=](Message &m)
                    {
                        qList<AnyObjectHolder<Gathering, String>> list;
                        if (!ExtractGatherings(m, &list))
                            return qResult(QResult::Core_InvalidArgument);
                        if (gatherings)
                            *gatherings = list;
                        return qResult(true);
                    });
    }

    bool MatchMakingClient::GetSessionURLs(ProtocolCallContext *context, qUnsignedInt32 gid, qList<StationURL> *urls)
    {
        Message p;
        p.WriteU32(gid);
        return Call(context, 41, p,
                    [=](Message &m)
                    {
                        qList<StationURL> list;
                        if (!m.ExtractList(&list,
                                           [](Message &s, StationURL &u)
                                           {
                                               u = s.ExtractStationURL();
                                               return s.IsValid();
                                           }))
                            return qResult(QResult::Core_InvalidArgument);
                        if (urls)
                            *urls = list;
                        return qResult(true);
                    });
    }

    bool MatchMakingClient::UpdateSessionHost(ProtocolCallContext *context, qUnsignedInt32 gid, bool isMigrateOwner)
    {
        Message p;
        p.WriteU32(gid);
        p.WriteBool(isMigrateOwner);
        return Call(context, 42, p);
    }

    bool MatchMakingClient::MigrateGatheringOwnership(ProtocolCallContext *context, qUnsignedInt32 gid, const qList<qUnsignedInt32> &candidates)
    {
        Message p;
        p.WriteU32(gid);
        AddU32List(p, candidates);
        p.WriteBool(false);
        return Call(context, 44, p);
    }

    bool MatchMakingClient::EndParticipation(ProtocolCallContext *context, qUnsignedInt32 gid, const String &message)
    {
        Message p;
        p.WriteU32(gid);
        p.WriteString(message);
        return m_ext.Call(context, 1, p,
                          [](Message &m)
                          {
                              return qResult(m.ReadBool() && m.IsValid());
                          });
    }

    namespace
    {

        ProtocolCallContext::ResponseHandler SessionResponse(MatchmakeSession *session)
        {
            return [=](Message &m)
            {
                MatchmakeSession result;
                if (!result.StreamIn(m))
                    return qResult(QResult::Core_InvalidArgument);
                if (session)
                    *session = result;
                return qResult(true);
            };
        }

        Message GidMessage(qUnsignedInt32 gid)
        {
            Message p;
            p.WriteU32(gid);
            return p;
        }

    } // namespace

    bool MatchmakeExtensionClient::CloseParticipation(ProtocolCallContext *context, qUnsignedInt32 gid)
    {
        return Call(context, 1, GidMessage(gid));
    }

    bool MatchmakeExtensionClient::OpenParticipation(ProtocolCallContext *context, qUnsignedInt32 gid)
    {
        return Call(context, 2, GidMessage(gid));
    }

    bool MatchmakeExtensionClient::BrowseMatchmakeSession(ProtocolCallContext *context, const MatchmakeSessionSearchCriteria &criteria,
                                                          const ResultRange &range, qList<AnyObjectHolder<Gathering, String>> *gatherings)
    {
        Message p;
        criteria.StreamOut(p);
        range.Add(p);
        return Call(context, 4, p,
                    [=](Message &m)
                    {
                        qList<AnyObjectHolder<Gathering, String>> list;
                        if (!ExtractGatherings(m, &list))
                            return qResult(QResult::Core_InvalidArgument);
                        if (gatherings)
                            *gatherings = list;
                        return qResult(true);
                    });
    }

    bool MatchmakeExtensionClient::ModifyMatchmakeSessionAttribute(ProtocolCallContext *context, qUnsignedInt32 gid, qUnsignedInt32 index,
                                                                   qUnsignedInt32 value)
    {
        Message p;
        p.WriteU32(gid);
        p.WriteU32(index);
        p.WriteU32(value);
        return Call(context, 8, p);
    }

    bool MatchmakeExtensionClient::UpdateNotificationData(ProtocolCallContext *context, qUnsignedInt32 type, qUnsignedInt32 param1,
                                                          qUnsignedInt32 param2, String strParam)
    {
        Message p;
        p.WriteU32(type);
        p.WriteU32(param1);
        p.WriteU32(param2);
        p.WriteString(strParam);
        return Call(context, 9, p);
    }

    bool MatchmakeExtensionClient::UpdateApplicationBuffer(ProtocolCallContext *context, qUnsignedInt32 gid, qVector<qByte> buffer)
    {
        Message p;
        p.WriteU32(gid);
        p.WriteBuffer(Buffer(buffer.data(), buffer.size()));
        return Call(context, 11, p);
    }

    bool MatchmakeExtensionClient::UpdateMatchmakeSessionAttribute(ProtocolCallContext *context, qUnsignedInt32 gid, qUnsignedInt32 *attributes)
    {
        Message p;
        p.WriteU32(gid);
        qVector<qUnsignedInt32> list;
        list.assign(attributes, attributes + 6);
        AddU32s(p, list);
        return Call(context, 12, p);
    }

    bool MatchmakeExtensionClient::GetFriendNotificationData(ProtocolCallContext *context, qList<NotificationEvent> *events,
                                                             const qList<qUnsignedInt32> &types)
    {
        Message p;
        AddU32List(p, types);
        return Call(context, 13, p,
                    [=](Message &m)
                    {
                        qList<NotificationEvent> list;
                        if (!ExtractStructures(m, &list))
                            return qResult(QResult::Core_InvalidArgument);
                        if (events)
                            *events = list;
                        return qResult(true);
                    });
    }

    bool MatchmakeExtensionClient::GetPlayingSession(ProtocolCallContext *context, const qList<qUnsignedInt32> &pids, qList<PlayingSession> *sessions)
    {
        Message p;
        AddU32List(p, pids);
        return Call(context, 16, p,
                    [=](Message &m)
                    {
                        qList<PlayingSession> list;
                        if (!ExtractStructures(m, &list))
                            return qResult(QResult::Core_InvalidArgument);
                        if (sessions)
                            *sessions = list;
                        return qResult(true);
                    });
    }

    bool MatchmakeExtensionClient::CreatePersistentGathering(ProtocolCallContext *context, const PersistentGathering &gathering, qUnsignedInt32 *gid,
                                                             const String &message)
    {
        Message p;
        gathering.StreamOut(p);
        p.WriteString(message);
        return Call(context, 17, p,
                    [=](Message &m)
                    {
                        qUnsignedInt32 id = m.ReadU32();
                        if (!m.IsValid())
                            return qResult(QResult::Core_InvalidArgument);
                        if (gid)
                            *gid = id;
                        return qResult(true);
                    });
    }

    bool MatchmakeExtensionClient::UpdatePersistentGathering(ProtocolCallContext *context, const PersistentGathering &gathering)
    {
        Message p;
        gathering.StreamOut(p);
        return Call(context, 18, p);
    }

    bool MatchmakeExtensionClient::JoinPersistentGathering(ProtocolCallContext *context, qUnsignedInt32 gid, const String &message,
                                                           const String &password)
    {
        Message p;
        p.WriteU32(gid);
        p.WriteString(message);
        p.WriteString(password);
        return Call(context, 19, p);
    }

    namespace
    {

        ProtocolCallContext::ResponseHandler PersistentList(qList<PersistentGathering> *out)
        {
            return [=](Message &m)
            {
                qList<PersistentGathering> list;
                if (!ExtractStructures(m, &list))
                    return qResult(QResult::Core_InvalidArgument);
                if (out)
                    *out = list;
                return qResult(true);
            };
        }

    } // namespace

    bool MatchmakeExtensionClient::FindPersistentGatheringByGatheringId(ProtocolCallContext *context, const qList<qUnsignedInt32> &gids,
                                                                        qList<PersistentGathering> *gatherings)
    {
        Message p;
        AddU32List(p, gids);
        return Call(context, 20, p, PersistentList(gatherings));
    }

    bool MatchmakeExtensionClient::FindOfficialPersistentGathering(ProtocolCallContext *context, bool isAvailableOnly, const ResultRange &range,
                                                                   qList<PersistentGathering> *gatherings)
    {
        Message p;
        p.WriteBool(isAvailableOnly);
        range.Add(p);
        return Call(context, 21, p, PersistentList(gatherings));
    }

    bool MatchmakeExtensionClient::FindPersistentGatheringByParticipant(ProtocolCallContext *context, qUnsignedInt32 pid, const ResultRange &range,
                                                                        qList<PersistentGathering> *gatherings)
    {
        Message p;
        p.WriteU32(pid);
        range.Add(p);
        return Call(context, 22, p, PersistentList(gatherings));
    }

    bool MatchmakeExtensionClient::UpdateProgressScore(qUnsignedInt32 gid, qByte score)
    {
        // A score update still in flight is simply superseded, because only the latest value matters to the server
        if (m_progressContext.GetState() == CallContext::CallInProgress)
            m_progressContext.Cancel();
        m_progressContext.Reset();
        Message p;
        p.WriteU32(gid);
        p.WriteU8(score);
        return Call(&m_progressContext, 34, p);
    }

    bool MatchmakeExtensionClient::GenerateMatchmakeSessionSystemPassword(ProtocolCallContext *context, qUnsignedInt32 gid, String *password)
    {
        return Call(context, 36, GidMessage(gid),
                    [=](Message &m)
                    {
                        String value = m.ReadString();
                        if (!m.IsValid())
                            return qResult(QResult::Core_InvalidArgument);
                        if (password)
                            *password = value;
                        return qResult(true);
                    });
    }

    bool MatchmakeExtensionClient::ClearMatchmakeSessionSystemPassword(ProtocolCallContext *context, qUnsignedInt32 gid)
    {
        return Call(context, 37, GidMessage(gid));
    }

    bool MatchmakeExtensionClient::CreateMatchmakeSession(ProtocolCallContext *context, const CreateMatchmakeSessionParam &param,
                                                          MatchmakeSession *session)
    {
        Message p;
        param.StreamOut(p);
        return Call(context, 38, p, SessionResponse(session));
    }

    bool MatchmakeExtensionClient::JoinMatchmakeSession(ProtocolCallContext *context, const JoinMatchmakeSessionParam &param,
                                                        MatchmakeSession *session)
    {
        Message p;
        param.StreamOut(p);
        return Call(context, 39, p, SessionResponse(session));
    }

    bool MatchmakeExtensionClient::AutoMatchmake(ProtocolCallContext *context, const AutoMatchmakeParam &param, MatchmakeSession *session)
    {
        Message p;
        param.StreamOut(p);
        return Call(context, 40, p, SessionResponse(session));
    }

    bool MatchmakeExtensionClient::FindMatchmakeSessionByGatheringIdDetail(ProtocolCallContext *context, qUnsignedInt32 gid,
                                                                           MatchmakeSession *session)
    {
        return Call(context, 41, GidMessage(gid), SessionResponse(session));
    }

    bool MatchmakeExtensionClient::UpdateMatchmakeSession(ProtocolCallContext *context, const UpdateMatchmakeSessionParam &param)
    {
        Message p;
        param.StreamOut(p);
        return Call(context, 44, p);
    }

    void MatchmakeExtensionClient::SetPersistentGatheringCodeKey(const String &key)
    {
        g_codeKey = key;
    }
    void MatchmakeExtensionClient::ClearPersistentGatheringCodeKey()
    {
        g_codeKey = String();
    }

    qUnsignedInt64 MatchmakeExtensionClient::GatheringIdToPersistentGatheringCode(qUnsignedInt32 gid)
    {
        return gid;
    }

    qUnsignedInt32 MatchmakeExtensionClient::PersistentGatheringCodeToGatheringId(qUnsignedInt64 code)
    {
        return code > 0xFFFFFFFFull ? 0 : (qUnsignedInt32) code;
    }

} // namespace nn::nex
