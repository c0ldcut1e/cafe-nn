#include "MatchMakingHelpers.h"
#include <nn/nex/core/StationURL.h>
#include <nn/nex/core/String.h>
#include <nn/nex/matchmaking/Gathering.h>
#include <nn/nex/matchmaking/MatchMakingClient.h>
#include <nn/nex/rmc/AnyObjectHolder.h>
#include <nn/nex/rmc/Message.h>
#include <nn/nex/rmc/ResultRange.h>
#include <nn/nex/runtime/ProtocolCallContext.h>
#include <nn/nex/types.h>

namespace nn::nex
{

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

} // namespace nn::nex
