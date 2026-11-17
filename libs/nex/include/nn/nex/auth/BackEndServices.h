/**
 * @file
 * @brief This owns the secure connection and our identity on it once login finishes. Protocol clients and notification handlers are
 * attached here.
 */

#pragma once

#include <nn/nex/auth/Credentials.h>
#include <nn/nex/auth/NotificationEvent.h>
#include <nn/nex/auth/NotificationEventHandler.h>
#include <nn/nex/core/StationURL.h>
#include <nn/nex/rmc/ClientProtocol.h>
#include <nn/nex/rmc/EndPoint.h>
#include <nn/nex/types.h>
#include <vector>

namespace nn::nex
{

    /**
     * @brief This owns the secure connection and our identity on it once login finishes. Protocol clients and notification handlers are attached here.
     */
    class BackEndServices
    {
    public:
        BackEndServices();
        virtual ~BackEndServices();

        bool IsConnected() const;
        void RegisterNotificationEventHandler(NotificationEventHandler *handler);
        void UnregisterNotificationEventHandler(NotificationEventHandler *handler);

        /**
         * @brief Protocol clients send their calls over the secure connection, so the game binds each one here after login.
         *
         * @param protocol The client to bind. It has to outlive the connection or be unbound by Logout.
         */
        void RegisterProtocol(ClientProtocol *protocol);

        EndPoint *GetSecureEndPoint()
        {
            return &m_secure;
        }
        qUnsignedInt32 GetPrincipalID() const
        {
            return m_principalId;
        }
        qUnsignedInt32 GetConnectionID() const
        {
            return m_connectionId;
        }
        const StationURL &GetPublicURL() const
        {
            return m_publicUrl;
        }
        const qList<StationURL> &GetLocalURLs() const
        {
            return m_localUrls;
        }
        Credentials *GetCredentials()
        {
            return &m_credentials;
        }

    protected:
        void DispatchNotification(const NotificationEvent &event);

        EndPoint m_secure;
        Credentials m_credentials;
        qUnsignedInt32 m_principalId  = 0;
        qUnsignedInt32 m_connectionId = 0;
        StationURL m_publicUrl;
        qList<StationURL> m_localUrls;

    private:
        std::vector<NotificationEventHandler *> m_handlers;
    };

} // namespace nn::nex
