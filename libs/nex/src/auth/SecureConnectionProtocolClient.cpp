#include <algorithm>
#include <nn/nex/auth/SecureConnectionProtocolClient.h>
#include <nn/nex/core/Buffer.h>
#include <nn/nex/core/StationURL.h>
#include <nn/nex/rmc/AnyDataHolder.h>
#include <nn/nex/rmc/Data.h>
#include <nn/nex/rmc/Message.h>
#include <nn/nex/runtime/ProtocolCallContext.h>
#include <nn/nex/types.h>

namespace nn::nex
{

    namespace
    {

        ProtocolCallContext::ResponseHandler RegisterResponse(qResult *returnValue, qUnsignedInt32 *connectionId, StationURL *publicUrl)
        {
            return [=](Message &m)
            {
                qResult result(m.ReadU32());
                qUnsignedInt32 cid = m.ReadU32();
                StationURL url     = m.ExtractStationURL();
                if (!m.IsValid())
                    return qResult(QResult::Core_InvalidArgument);
                if (returnValue)
                    *returnValue = result;
                if (connectionId)
                    *connectionId = cid;
                if (publicUrl)
                    *publicUrl = url;
                return result;
            };
        }

        void AddURLs(Message &m, const qList<StationURL> &urls)
        {
            m.AddList(urls,
                      [](Message &s, const StationURL &u)
                      {
                          s.AddStationURL(u);
                      });
        }

    } // namespace

    bool SecureConnectionProtocolClient::Register(ProtocolCallContext *context, const qList<StationURL> &urls, qResult *returnValue,
                                                  qUnsignedInt32 *connectionId, StationURL *publicUrl)
    {
        Message p;
        AddURLs(p, urls);
        return Call(context, 1, p, RegisterResponse(returnValue, connectionId, publicUrl));
    }

    bool SecureConnectionProtocolClient::RegisterEx(ProtocolCallContext *context, const qList<StationURL> &urls, const Data &loginData,
                                                    qResult *returnValue, qUnsignedInt32 *connectionId, StationURL *publicUrl)
    {
        Message p;
        AddURLs(p, urls);
        AnyDataHolder(loginData).Add(p);
        return Call(context, 4, p, RegisterResponse(returnValue, connectionId, publicUrl));
    }

    bool SecureConnectionProtocolClient::RequestURLs(ProtocolCallContext *context, qUnsignedInt32 cidTarget, qUnsignedInt32 pidTarget,
                                                     bool *returnValue, qList<StationURL> *urls)
    {
        Message p;
        p.WriteU32(cidTarget);
        p.WriteU32(pidTarget);
        return Call(context, 3, p,
                    [=](Message &m)
                    {
                        bool found = m.ReadBool();
                        qList<StationURL> list;
                        if (!m.ExtractList(&list,
                                           [](Message &s, StationURL &u)
                                           {
                                               u = s.ExtractStationURL();
                                               return s.IsValid();
                                           }))
                            return qResult(QResult::Core_InvalidArgument);
                        if (returnValue)
                            *returnValue = found;
                        if (urls)
                            *urls = list;
                        return qResult(true);
                    });
    }

    bool SecureConnectionProtocolClient::UpdateURLs(ProtocolCallContext *context, const qList<StationURL> &urls)
    {
        Message p;
        AddURLs(p, urls);
        return Call(context, 6, p);
    }

    bool SecureConnectionProtocolClient::ReplaceURL(ProtocolCallContext *context, const StationURL &target, const StationURL &url)
    {
        Message p;
        p.AddStationURL(target);
        p.AddStationURL(url);
        return Call(context, 7, p);
    }

    bool SecureConnectionProtocolClient::SendReport(ProtocolCallContext *context, qUnsignedInt32 reportId, const qVector<qByte> &data)
    {
        Message p;
        p.WriteU32(reportId);
        p.WriteQBuffer(qBuffer(data.data(), data.size()));
        return Call(context, 8, p);
    }

} // namespace nn::nex
