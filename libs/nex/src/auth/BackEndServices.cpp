#include <algorithm>
#include <nn/nex/auth/BackEndServices.h>
#include <nn/nex/auth/NotificationEvent.h>
#include <nn/nex/auth/NotificationEventHandler.h>
#include <nn/nex/rmc/ClientProtocol.h>
#include <nn/nex/rmc/Message.h>
#include <nn/nex/types.h>

namespace nn::nex
{

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
