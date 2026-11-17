#include <nn/nex/core/DateTime.h>
#include <nn/nex/core/String.h>
#include <nn/nex/nat/NATTraversal.h>

namespace nn::nex
{

    void NATTraversalRelayClient::Init()
    {
        if (!GetEndPoint())
            return;
        GetEndPoint()->RegisterRequestHandler(3,
                                              [this](qUnsignedInt32 methodId, Message &request, Message &)
                                              {
                                                  if (methodId != 2)
                                                      return qResult(QResult::Core_NotImplemented);
                                                  StationURL url = request.ExtractStationURL();
                                                  if (!request.IsValid())
                                                      return qResult(QResult::Core_InvalidArgument);
                                                  qList<StationURL> urls;
                                                  urls.push_back(url);
                                                  if (m_probeHandler)
                                                      m_probeHandler(urls);
                                                  return qResult(true);
                                              });
    }

    bool NATTraversalRelayClient::CallRequestProbeInitiationExt(ProtocolCallContext *context, const qList<StationURL> &targets,
                                                                const StationURL &source)
    {
        Message p;
        p.AddList(targets,
                  [](Message &s, const StationURL &u)
                  {
                      s.AddStationURL(u);
                  });
        p.AddStationURL(source);
        return Call(context, 3, p);
    }

    bool NATTraversalRelayClient::CallReportNATTraversalResult(ProtocolCallContext *context, const qUnsignedInt32 &cid, const bool &result,
                                                               const qUnsignedInt32 &rtt)
    {
        Message p;
        p.WriteU32(cid);
        p.WriteBool(result);
        p.WriteU32(rtt);
        return Call(context, 4, p);
    }

    bool NATTraversalRelayClient::CallReportNATProperties(ProtocolCallContext *context, const qUnsignedInt32 &natMapping,
                                                          const qUnsignedInt32 &natFiltering, const qUnsignedInt32 &rtt)
    {
        Message p;
        p.WriteU32(natMapping);
        p.WriteU32(natFiltering);
        p.WriteU32(rtt);
        return Call(context, 5, p);
    }

    bool NATTraversalRelayClient::CallGetRelaySignatureKey(ProtocolCallContext *context, qInt32 *relayMode, DateTime *currentTime, String *address,
                                                           qUnsignedInt16 *port, qInt32 *relayAddressType, qUnsignedInt32 *gameServerId)
    {
        return Call(context, 6, Message(),
                    [=](Message &m)
                    {
                        qInt32 mode       = (qInt32) m.ReadU32();
                        DateTime time     = m.ReadDateTime();
                        String addr       = m.ReadString();
                        qUnsignedInt16 p  = m.ReadU16();
                        qInt32 type       = (qInt32) m.ReadU32();
                        qUnsignedInt32 id = m.ReadU32();
                        if (!m.IsValid())
                            return qResult(QResult::Core_InvalidArgument);
                        if (relayMode)
                            *relayMode = mode;
                        if (currentTime)
                            *currentTime = time;
                        if (address)
                            *address = addr;
                        if (port)
                            *port = p;
                        if (relayAddressType)
                            *relayAddressType = type;
                        if (gameServerId)
                            *gameServerId = id;
                        return qResult(true);
                    });
    }

} // namespace nn::nex
