#include <algorithm>
#include <nn/nex/auth/RVConnectionData.h>
#include <nn/nex/auth/TicketGrantingProtocolClient.h>
#include <nn/nex/core/Buffer.h>
#include <nn/nex/core/String.h>
#include <nn/nex/rmc/AnyDataHolder.h>
#include <nn/nex/rmc/Data.h>
#include <nn/nex/rmc/Message.h>
#include <nn/nex/runtime/ProtocolCallContext.h>
#include <nn/nex/types.h>

namespace nn::nex
{

    bool TicketGrantingProtocolClient::LoginCommon(ProtocolCallContext *context, qUnsignedInt32 methodId, const Message &parameters,
                                                   qResult *returnValue, qUnsignedInt32 *principalId, Buffer *ticket,
                                                   RVConnectionData *connectionData, String *returnMessage)
    {
        return Call(context, methodId, parameters,
                    [=](Message &m)
                    {
                        qResult result(m.ReadU32());
                        qUnsignedInt32 pid = m.ReadU32();
                        Buffer response    = m.ReadBuffer();
                        RVConnectionData data;
                        if (!data.Extract(m))
                            return qResult(QResult::Core_InvalidArgument);
                        String message = m.ReadString();
                        if (!m.IsValid())
                            return qResult(QResult::Core_InvalidArgument);
                        if (returnValue)
                            *returnValue = result;
                        if (principalId)
                            *principalId = pid;
                        if (ticket)
                            *ticket = response;
                        if (connectionData)
                            *connectionData = data;
                        if (returnMessage)
                            *returnMessage = message;
                        return result;
                    });
    }

    bool TicketGrantingProtocolClient::Login(ProtocolCallContext *context, const String &userName, qResult *returnValue, qUnsignedInt32 *principalId,
                                             Buffer *ticket, RVConnectionData *connectionData, String *returnMessage)
    {
        Message p;
        p.WriteString(userName);
        return LoginCommon(context, 1, p, returnValue, principalId, ticket, connectionData, returnMessage);
    }

    bool TicketGrantingProtocolClient::LoginEx(ProtocolCallContext *context, const String &userName, const Data &extraData, qResult *returnValue,
                                               qUnsignedInt32 *principalId, Buffer *ticket, RVConnectionData *connectionData, String *returnMessage)
    {
        Message p;
        p.WriteString(userName);
        AnyDataHolder(extraData).Add(p);
        return LoginCommon(context, 2, p, returnValue, principalId, ticket, connectionData, returnMessage);
    }

    bool TicketGrantingProtocolClient::RequestTicket(ProtocolCallContext *context, qUnsignedInt32 sourcePid, qUnsignedInt32 targetPid,
                                                     qResult *returnValue, Buffer *ticket)
    {
        Message p;
        p.WriteU32(sourcePid);
        p.WriteU32(targetPid);
        return Call(context, 3, p,
                    [=](Message &m)
                    {
                        qResult result(m.ReadU32());
                        Buffer response = m.ReadBuffer();
                        if (!m.IsValid())
                            return qResult(QResult::Core_InvalidArgument);
                        if (returnValue)
                            *returnValue = result;
                        if (ticket)
                            *ticket = response;
                        return result;
                    });
    }

} // namespace nn::nex
