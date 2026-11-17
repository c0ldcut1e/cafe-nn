#include <algorithm>
#include <nn/nex/Authentication.h>

namespace nn::nex
{

    bool RVConnectionData::Extract(Message &m)
    {
        size_t end;
        if (!m.BeginExtractStructure(&end))
            return false;
        m_regular = m.ExtractStationURL();
        if (!_Type_qlist<_Type_byte>::Extract(&m, &m_specialProtocols))
            return false;
        m_special = m.ExtractStationURL();
        if (m.GetCursor() + 8 <= end)
            m_currentTime = m.ReadDateTime();
        m.EndExtractStructure(end);
        return m.IsValid();
    }

    void AuthenticationInfo::StreamOut(Message &m) const
    {
        // We write an empty header for the parent Data level because the server expects one per class level
        m.EndStructure(m.BeginStructure());
        size_t s = m.BeginStructure();
        m.WriteString(token);
        m.WriteU32(ngsVersion);
        m.WriteU8(tokenType);
        m.WriteU32(serverVersion);
        m.EndStructure(s);
    }

    void NintendoLoginData::StreamOut(Message &m) const
    {
        size_t s = m.BeginStructure();
        m.WriteString(token);
        m.EndStructure(s);
    }

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

    void NotificationEvent::StreamOut(Message &m) const
    {
        size_t s = m.BeginStructure();
        m.WriteU32(m_pidSource);
        m.WriteU32(m_type);
        m.WriteU32(m_param1);
        m.WriteU32(m_param2);
        m.WriteString(m_strParam);
        m.WriteU32(m_param3);
        m.EndStructure(s);
    }

    bool NotificationEvent::StreamIn(Message &m)
    {
        size_t end;
        if (!m.BeginExtractStructure(&end))
            return false;
        m_pidSource = m.ReadU32();
        m_type      = m.ReadU32();
        m_param1    = m.ReadU32();
        m_param2    = m.ReadU32();
        m_strParam  = m.ReadString();
        // Older servers stop before the third parameter, so we only read it when the structure has room for it
        m_param3 = m.GetCursor() + 4 <= end ? m.ReadU32() : 0;
        m.EndExtractStructure(end);
        return m.IsValid();
    }

    static DataFactoryRegistration<NotificationEvent> s_notificationEvent("NotificationEvent");

    BackEndServices::BackEndServices()
    {
        m_secure.RegisterRequestHandler(14,
                                        [this](qUnsignedInt32 methodId, Message &request, Message &)
                                        {
                                            // We accept method 2 as well because newer servers send the same event under it
                                            if (methodId != 1 && methodId != 2)
                                                return qResult(QResult::Core_NotImplemented);
                                            NotificationEvent event;
                                            if (!event.StreamIn(request))
                                                return qResult(QResult::Core_InvalidArgument);
                                            DispatchNotification(event);
                                            return qResult(true);
                                        });
    }

    BackEndServices::~BackEndServices() = default;

    bool BackEndServices::IsConnected() const
    {
        return m_secure.IsConnected();
    }

    void BackEndServices::RegisterNotificationEventHandler(NotificationEventHandler *handler)
    {
        if (std::find(m_handlers.begin(), m_handlers.end(), handler) == m_handlers.end())
            m_handlers.push_back(handler);
    }

    void BackEndServices::UnregisterNotificationEventHandler(NotificationEventHandler *handler)
    {
        m_handlers.erase(std::remove(m_handlers.begin(), m_handlers.end(), handler), m_handlers.end());
    }

    void BackEndServices::RegisterProtocol(ClientProtocol *protocol)
    {
        protocol->Bind(&m_secure);
    }

    void BackEndServices::DispatchNotification(const NotificationEvent &event)
    {
        std::vector<NotificationEventHandler *> handlers = m_handlers;
        for (NotificationEventHandler *h : handlers) h->ProcessNotificationEvent(event);
    }

} // namespace nn::nex
