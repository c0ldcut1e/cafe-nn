/**
 * @file
 * @brief These are the login protocols and NgsFacade, which runs the whole login. Login goes through the authentication server first and then the secure server, and every other protocol runs over the secure connection that this leaves open.
 */

#pragma once

#include <nn/nex/EndPoint.h>
#include <nn/nex/Kerberos.h>
#include <nn/nex/NotificationEvent.h>
#include <vector>

namespace nn::nex
{

    /**
     * @brief The authentication server returns this with the ticket. It holds the secure server's station URL and the server time.
     */
    class RVConnectionData
    {
    public:
        const StationURL &GetRegularProtocols() const
        {
            return m_regular;
        }
        const StationURL &GetSpecialProtocolsURL() const
        {
            return m_special;
        }
        const qList<qByte> &GetSpecialProtocols() const
        {
            return m_specialProtocols;
        }
        const DateTime &GetCurrentUTCTime() const
        {
            return m_currentTime;
        }
        bool Extract(Message &m);

    private:
        StationURL m_regular;
        qList<qByte> m_specialProtocols;
        StationURL m_special;
        DateTime m_currentTime;
    };

    /**
     * @brief LoginEx sends this as its extra data. It carries the NEX token and the NEX version the game was built against.
     */
    class AuthenticationInfo : public Data
    {
    public:
        String token;
        qUnsignedInt32 ngsVersion    = 3;
        qByte tokenType              = 1;
        qUnsignedInt32 serverVersion = 0;

        const char *GetDataTypeName() const override
        {
            return "AuthenticationInfo";
        }
        Data *Clone() const override
        {
            return new AuthenticationInfo(*this);
        }
        void StreamOut(Message &m) const override;
    };

    /**
     * @brief RegisterEx sends this so the secure server can check the NEX token again.
     */
    class NintendoLoginData : public Data
    {
    public:
        String token;

        const char *GetDataTypeName() const override
        {
            return "NintendoLoginData";
        }
        Data *Clone() const override
        {
            return new NintendoLoginData(*this);
        }
        void StreamOut(Message &m) const override;
    };

    /**
     * @brief This is the client for the authentication server, which turns a NEX token into a ticket for the secure server.
     */
    class TicketGrantingProtocolClient : public ClientProtocol
    {
    public:
        TicketGrantingProtocolClient() : ClientProtocol(10)
        {}

        /**
         * @brief Logs in by user name alone and returns the PID, a sealed ticket and the secure server's URL. Protocol 10 method 1.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param userName The account's PID written in decimal.
         * @param returnValue Receives the server's result code. Filled in when context completes, so it has to stay alive until then.
         * @param principalId Receives our PID. Filled in when context completes, so it has to stay alive until then.
         * @param ticket Receives the ticket sealed under the account's key. Filled in when context completes, so it has to stay alive until then.
         * @param connectionData Receives the secure server's station URL. Filled in when context completes, so it has to stay alive until then.
         * @param returnMessage Receives the server's message, such as its build name. Filled in when context completes, so it has to stay alive until then.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool Login(ProtocolCallContext *context, const String &userName, qResult *returnValue, qUnsignedInt32 *principalId, Buffer *ticket,
                   RVConnectionData *connectionData, String *returnMessage);

        /**
         * @brief Logs in with a NEX token in extraData, which is what Wii U servers expect. The results match Login. Protocol 10 method 2.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param userName The account's PID written in decimal.
         * @param extraData An AuthenticationInfo holding the NEX token.
         * @param returnValue Receives the server's result code. Filled in when context completes, so it has to stay alive until then.
         * @param principalId Receives our PID. Filled in when context completes, so it has to stay alive until then.
         * @param ticket Receives the ticket sealed under the account's key. Filled in when context completes, so it has to stay alive until then.
         * @param connectionData Receives the secure server's station URL. Filled in when context completes, so it has to stay alive until then.
         * @param returnMessage Receives the server's message, such as its build name. Filled in when context completes, so it has to stay alive until then.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool LoginEx(ProtocolCallContext *context, const String &userName, const Data &extraData, qResult *returnValue, qUnsignedInt32 *principalId,
                     Buffer *ticket, RVConnectionData *connectionData, String *returnMessage);

        /**
         * @brief Asks for a ticket from sourcePid to targetPid, which is how we get the secure server's ticket after LoginEx. Protocol 10 method 3.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param sourcePid Our PID.
         * @param targetPid The PID of the server the ticket is for, which is the secure server's PID from connectionData.
         * @param returnValue Receives the server's result code. Filled in when context completes, so it has to stay alive until then.
         * @param ticket Receives the ticket sealed under the account's key. Filled in when context completes, so it has to stay alive until then.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool RequestTicket(ProtocolCallContext *context, qUnsignedInt32 sourcePid, qUnsignedInt32 targetPid, qResult *returnValue, Buffer *ticket);

    private:
        bool LoginCommon(ProtocolCallContext *context, qUnsignedInt32 methodId, const Message &parameters, qResult *returnValue,
                         qUnsignedInt32 *principalId, Buffer *ticket, RVConnectionData *connectionData, String *returnMessage);
    };

    /**
     * @brief This is the client for the secure server's own protocol, which registers our station URLs and looks up other players' URLs.
     */
    class SecureConnectionProtocolClient : public ClientProtocol
    {
    public:
        SecureConnectionProtocolClient() : ClientProtocol(11)
        {}

        /**
         * @brief Registers our local station URLs and returns our connection ID and the public URL the server saw us at. Protocol 11 method 1.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param urls Our station URLs.
         * @param returnValue Receives the server's result code. Filled in when context completes, so it has to stay alive until then.
         * @param connectionId Receives our connection ID, which peers put in RVCID. Filled in when context completes, so it has to stay alive until then.
         * @param publicUrl Receives our address as the server saw it. Filled in when context completes, so it has to stay alive until then.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool Register(ProtocolCallContext *context, const qList<StationURL> &urls, qResult *returnValue, qUnsignedInt32 *connectionId,
                      StationURL *publicUrl);

        /**
         * @brief Does the same as Register but also sends NintendoLoginData, which Wii U servers require. Protocol 11 method 4.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param urls Our station URLs.
         * @param loginData A NintendoLoginData holding the NEX token.
         * @param returnValue Receives the server's result code. Filled in when context completes, so it has to stay alive until then.
         * @param connectionId Receives our connection ID, which peers put in RVCID. Filled in when context completes, so it has to stay alive until then.
         * @param publicUrl Receives our address as the server saw it. Filled in when context completes, so it has to stay alive until then.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool RegisterEx(ProtocolCallContext *context, const qList<StationURL> &urls, const Data &loginData, qResult *returnValue,
                        qUnsignedInt32 *connectionId, StationURL *publicUrl);

        /**
         * @brief Returns the station URLs another player registered, so a peer can connect to them. Protocol 11 method 3.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param cidTarget The other player's connection ID.
         * @param pidTarget The other player's PID.
         * @param returnValue Receives the server's result code. Filled in when context completes, so it has to stay alive until then.
         * @param urls Receives the other player's station URLs. Filled in when context completes, so it has to stay alive until then.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool RequestURLs(ProtocolCallContext *context, qUnsignedInt32 cidTarget, qUnsignedInt32 pidTarget, bool *returnValue,
                         qList<StationURL> *urls);

        /**
         * @brief Replaces our registered station URLs. Protocol 11 method 6.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param urls Our station URLs.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool UpdateURLs(ProtocolCallContext *context, const qList<StationURL> &urls);

        /**
         * @brief Replaces one registered station URL with another. Protocol 11 method 7.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param target The registered URL to replace.
         * @param url The URL that replaces it.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool ReplaceURL(ProtocolCallContext *context, const StationURL &target, const StationURL &url);

        /**
         * @brief Sends the server an opaque report blob. Protocol 11 method 8.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param reportId A game-defined report type.
         * @param data The report contents.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool SendReport(ProtocolCallContext *context, qUnsignedInt32 reportId, const qVector<qByte> &data);
    };

    /**
     * @brief This is what the console's account service returns for one game server, everything Login needs to reach it.
     */
    struct NexServiceToken
    {
        String token;
        String nexPassword;
        qUnsignedInt32 principalId = 0;
        String host;
        qUnsignedInt16 port = 0;
    };

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

    /**
     * @brief This is the entry point a game uses, and there is one per process. Login connects everything, and the results are read from the BackEndServices getters afterwards.
     */
    class NgsFacade : public BackEndServices
    {
    public:
        using TokenProvider = qResult (*)(qUnsignedInt32 gameServerId, NexServiceToken *out);

        NgsFacade();
        ~NgsFacade() override;

        static NgsFacade *GetInstance();

        /**
         * @brief This runs the whole login: token, authentication server, ticket, secure server and station registration.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param gameServerId The game's server ID, such as 0x101D9D00 for Minecraft.
         * @param accessKey The game's NEX access key, which the authentication server checks.
         * @param nexVersion Ignored, since we send the version this library implements.
         * @param timeout Ignored. Use Wait on context with a timeout instead.
         * @return False when a login is already running or we are connected. The result arrives through context.
         */
        bool Login(ProtocolCallContext *context, qUnsignedInt32 gameServerId, const wchar_t *accessKey, qInt32 nexVersion = 0,
                   qUnsignedInt32 timeout = 0);

        /**
         * @brief Closes the secure connection. Bound protocol clients stop working until the next Login.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @return False when context is already in use.
         */
        bool Logout(ProtocolCallContext *context);

        /**
         * @brief This holds the result of the last failed Login, including errors from before the authentication server answered.
         */
        qResult GetLastLoginErrorCode() const
        {
            return m_lastLoginError;
        }

        /**
         * @brief The console build asks the account service by default, and a host build must supply its own token source.
         *
         * @param provider Called at the start of each Login to get the token. A null provider makes Login fail.
         */
        static void SetTokenProvider(TokenProvider provider);

    private:
        class LoginJob;
        friend class LoginJob;

        LoginJob *m_login        = nullptr;
        qResult m_lastLoginError = qResult(true);
        static NgsFacade *s_instance;
    };

} // namespace nn::nex
