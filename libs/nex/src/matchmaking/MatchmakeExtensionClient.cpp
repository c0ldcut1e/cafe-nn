#include "MatchMakingHelpers.h"
#include <nn/nex/auth/NotificationEvent.h>
#include <nn/nex/core/Buffer.h>
#include <nn/nex/core/String.h>
#include <nn/nex/matchmaking/AutoMatchmakeParam.h>
#include <nn/nex/matchmaking/CreateMatchmakeSessionParam.h>
#include <nn/nex/matchmaking/Gathering.h>
#include <nn/nex/matchmaking/JoinMatchmakeSessionParam.h>
#include <nn/nex/matchmaking/MatchmakeExtensionClient.h>
#include <nn/nex/matchmaking/MatchmakeSession.h>
#include <nn/nex/matchmaking/MatchmakeSessionSearchCriteria.h>
#include <nn/nex/matchmaking/PersistentGathering.h>
#include <nn/nex/matchmaking/PlayingSession.h>
#include <nn/nex/matchmaking/UpdateMatchmakeSessionParam.h>
#include <nn/nex/rmc/AnyObjectHolder.h>
#include <nn/nex/rmc/Message.h>
#include <nn/nex/rmc/ResultRange.h>
#include <nn/nex/runtime/CallContext.h>
#include <nn/nex/runtime/ProtocolCallContext.h>
#include <nn/nex/types.h>

namespace nn::nex
{

    namespace
    {

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
