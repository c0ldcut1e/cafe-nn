#include <cstdio>
#include <nn/nex/Authentication.h>

#ifdef __WIIU__
#include <coreinit/dynload.h>
#include <nn/act.h>
#endif

namespace nn::nex
{

    namespace
    {

#ifdef __WIIU__

        // We describe the account service's result layout ourselves because wut does not
        struct ACTNexAuthenticationResult
        {
            char token[0x201];
            char nexPassword[0x41];
            qUnsignedInt8 padding[2];
            qUnsignedInt32 pid;
            char host[16];
            qUnsignedInt16 port;
            qUnsignedInt8 padding2[2];
        };
        static_assert(sizeof(ACTNexAuthenticationResult) == 0x25C);

        using AcquireFunction = qUnsignedInt32 (*)(ACTNexAuthenticationResult *, qUnsignedInt32);

        qResult AcquireFromAccountService(qUnsignedInt32 gameServerId, NexServiceToken *out)
        {
            // wut ships no import stub for this export, so we resolve it from the loaded module instead
            OSDynLoad_Module module;
            if (OSDynLoad_Acquire("nn_act.rpl", &module) != OS_DYNLOAD_OK)
                return qResult(QResult::Core_InitializationFailure);
            AcquireFunction acquire = nullptr;
            OSDynLoad_FindExport(module, OS_DYNLOAD_EXPORT_FUNC, "AcquireNexServiceToken__Q2_2nn3actFP26ACTNexAuthenticationResultUi",
                                 (void **) &acquire);
            if (!acquire)
            {
                OSDynLoad_Release(module);
                return qResult(QResult::Core_NotImplemented);
            }
            nn::act::Initialize();
            static ACTNexAuthenticationResult result;
            memset(&result, 0, sizeof(result));
            qUnsignedInt32 status = acquire(&result, gameServerId);
            nn::act::Finalize();
            OSDynLoad_Release(module);
            if (status & 0x80000000)
                return qResult(QResult::RendezVous_NotAuthenticated);
            out->token       = String(result.token);
            out->nexPassword = String(result.nexPassword);
            out->principalId = result.pid;
            out->host        = String(result.host);
            out->port        = result.port;
            return qResult(true);
        }

        NgsFacade::TokenProvider g_tokenProvider = AcquireFromAccountService;

#else

        NgsFacade::TokenProvider g_tokenProvider = nullptr;

#endif

    } // namespace

    // Each step's completion callback starts the next, so the whole login advances inside Scheduler::Dispatch
    class NgsFacade::LoginJob
    {
    public:
        enum Step
        {
            ConnectAuthentication,
            LoginEx,
            ConnectSecure,
            RegisterEx
        };

        LoginJob(NgsFacade *facade, ProtocolCallContext *context, qUnsignedInt32 gameServerId, const wchar_t *accessKey)
            : m_facade(facade), m_context(context), m_gameServerId(gameServerId), m_accessKey(String(accessKey).ToUtf8())
        {
            m_connectContext.RegisterCompletionCallback(new Callback<LoginJob *>(OnStep, this), true);
            m_callContext.RegisterCompletionCallback(new Callback<LoginJob *>(OnStep, this), true);
        }

        void Start()
        {
            NexServiceToken token;
            qResult acquired = g_tokenProvider ? g_tokenProvider(m_gameServerId, &token) : qResult(QResult::Core_InitializationFailure);
            if (!acquired)
            {
                Finish(acquired);
                return;
            }
            m_token = token;
            m_ticketGranting.Bind(&m_authentication);

            StationURL url;
            url.SetURLType(StationURL::prudps);
            url.SetAddress(token.host);
            url.SetPortNumber(token.port);
            m_step = ConnectAuthentication;
            m_connectContext.InitiateCall();
            qResult started = m_authentication.Connect(&m_connectContext, url, m_accessKey.c_str(), Buffer(), nullptr);
            if (!started)
            {
                m_connectContext.Cancel();
                Finish(started);
            }
        }

        void Abort(qResult result)
        {
            m_aborting = true;
            m_connectContext.Cancel();
            m_callContext.Cancel();
            m_authentication.Close();
            m_aborting = false;
            Finish(result);
        }

    private:
        static void OnStep(CallContext *context, LoginJob *job)
        {
            job->Advance(context);
        }

        void Advance(CallContext *context)
        {
            if (m_aborting || m_finished)
                return;
            if (context->GetState() != CallContext::CallSuccess)
            {
                m_authentication.Close();
                m_facade->m_secure.Close();
                Finish(context->GetOutcome());
                return;
            }
            switch (m_step)
            {
                case ConnectAuthentication:
                    SendLoginEx();
                    break;
                case LoginEx:
                    ConnectSecureServer();
                    break;
                case ConnectSecure:
                    SendRegisterEx();
                    break;
                case RegisterEx:
                    CompleteLogin();
                    break;
            }
        }

        void SendLoginEx()
        {
            AuthenticationInfo info;
            info.token = m_token.token;
            wchar_t name[16];
            swprintf(name, 16, L"%u", m_token.principalId);
            m_step = LoginEx;
            m_callContext.Reset();
            m_callContext.RegisterCompletionCallback(new Callback<LoginJob *>(OnStep, this), true);
            m_ticketGranting.LoginEx(&m_callContext, String(name), info, &m_result, &m_principalId, &m_ticket, &m_connectionData, nullptr);
        }

        void ConnectSecureServer()
        {
            m_authentication.Close();
            Buffer key = KerberosEncryption::DeriveKey(m_principalId, m_token.nexPassword);
            Buffer decrypted;
            Ticket ticket;
            if (!KerberosEncryption(key).Decrypt(m_ticket, &decrypted) || !ticket.Parse(decrypted))
            {
                Finish(qResult(QResult::RendezVous_EncryptionFailure));
                return;
            }
            m_sessionKey = ticket.sessionKey;

            const StationURL &secure = m_connectionData.GetRegularProtocols();
            RandomNumberGenerator rng;
            m_check = rng.GetRandomNumber();
            ByteStream request;
            request.WriteU32(m_principalId);
            request.WriteU32(secure.GetConnectionID());
            request.WriteU32(m_check);
            ByteStream payload;
            payload.WriteBuffer(ticket.internalTicket);
            payload.WriteBuffer(KerberosEncryption(m_sessionKey).Encrypt(Buffer(request.GetData(), request.GetSize())));

            m_step = ConnectSecure;
            m_connectContext.Reset();
            m_connectContext.RegisterCompletionCallback(new Callback<LoginJob *>(OnStep, this), true);
            m_connectContext.InitiateCall();
            qResult started = m_facade->m_secure.Connect(&m_connectContext, secure, m_accessKey.c_str(), Buffer(payload.GetData(), payload.GetSize()),
                                                         &m_sessionKey);
            if (!started)
            {
                m_connectContext.Cancel();
                Finish(started);
            }
        }

        void SendRegisterEx()
        {
            // We check the returned value because the server proves it read our ticket by answering with our check plus one
            ByteStream response(m_facade->m_secure.GetTransport().GetConnectResponse().GetContentPtr(),
                                m_facade->m_secure.GetTransport().GetConnectResponse().GetContentSize());
            Buffer proof = response.ReadBuffer();
            if (response.IsValid() && proof.GetContentSize() == 4)
            {
                ByteStream value(proof.GetContentPtr(), 4);
                if (value.ReadU32() != m_check + 1)
                {
                    m_facade->m_secure.Close();
                    Finish(qResult(QResult::Transport_IncorrectRemoteAuthentication));
                    return;
                }
            }

            InetAddress local = m_facade->m_secure.GetTransport().GetLocalAddress();
            StationURL url;
            url.SetURLType(StationURL::prudp);
            url.SetAddress(local.GetAddressStr());
            url.SetPortNumber(local.GetPortNumber());
            url.SetNATFiltering(NATProperties::UnknownFiltering);
            url.SetNATMapping(NATProperties::UnknownMapping);
            url.SetStreamID(15);
            url.SetType(StationURL::Public);
            url.SetParameter("pmp", "0");
            url.SetParameter("upnp", "0");
            m_facade->m_localUrls.clear();
            m_facade->m_localUrls.push_back(url);

            NintendoLoginData login;
            login.token = m_token.token;
            m_secureConnection.Bind(&m_facade->m_secure);
            m_step = RegisterEx;
            m_callContext.Reset();
            m_callContext.RegisterCompletionCallback(new Callback<LoginJob *>(OnStep, this), true);
            m_secureConnection.RegisterEx(&m_callContext, m_facade->m_localUrls, login, &m_result, &m_facade->m_connectionId, &m_facade->m_publicUrl);
        }

        void CompleteLogin()
        {
            m_facade->m_principalId = m_principalId;
            m_facade->m_credentials.SetPrincipalID(m_principalId);
            Finish(qResult(true));
        }

        void Finish(qResult result)
        {
            if (m_finished)
                return;
            m_finished                   = true;
            m_facade->m_lastLoginError   = result;
            ProtocolCallContext *context = m_context;
            NgsFacade *facade            = m_facade;
            // The game's callback may start a new login, so the job is gone before we signal anything
            facade->m_login = nullptr;
            Scheduler::GetInstance()->Queue(new DeleteJob(this));
            if (result)
                context->SignalSuccess(result);
            else
                context->SignalFailure(result);
        }

        class DeleteJob : public Job
        {
        public:
            explicit DeleteJob(LoginJob *job) : Job(SystemJob), m_job(job)
            {
                DeleteWhenComplete();
            }

        protected:
            State Execute() override
            {
                delete m_job;
                return Complete;
            }

        private:
            LoginJob *m_job;
        };

        NgsFacade *m_facade;
        ProtocolCallContext *m_context;
        qUnsignedInt32 m_gameServerId;
        std::string m_accessKey;
        NexServiceToken m_token;
        Step m_step     = ConnectAuthentication;
        bool m_aborting = false;
        bool m_finished = false;

        EndPoint m_authentication;
        TicketGrantingProtocolClient m_ticketGranting;
        SecureConnectionProtocolClient m_secureConnection;
        CallContext m_connectContext;
        ProtocolCallContext m_callContext;

        qResult m_result;
        qUnsignedInt32 m_principalId = 0;
        Buffer m_ticket;
        RVConnectionData m_connectionData;
        Buffer m_sessionKey;
        qUnsignedInt32 m_check = 0;
    };

    NgsFacade *NgsFacade::s_instance = nullptr;

    NgsFacade::NgsFacade()
    {
        if (!s_instance)
            s_instance = this;
    }

    NgsFacade::~NgsFacade()
    {
        if (m_login)
            m_login->Abort(qResult(QResult::Core_Cancelled));
        m_secure.Close();
        if (s_instance == this)
            s_instance = nullptr;
    }

    NgsFacade *NgsFacade::GetInstance()
    {
        return s_instance;
    }

    void NgsFacade::SetTokenProvider(TokenProvider provider)
    {
        g_tokenProvider = provider;
    }

    bool NgsFacade::Login(ProtocolCallContext *context, qUnsignedInt32 gameServerId, const wchar_t *accessKey, qInt32 nexVersion,
                          qUnsignedInt32 timeout)
    {
        (void) nexVersion;
        (void) timeout;
        if (!context || m_login || IsConnected() || !context->InitiateCall())
            return false;
        m_login = new LoginJob(this, context, gameServerId, accessKey);
        m_login->Start();
        return true;
    }

    bool NgsFacade::Logout(ProtocolCallContext *context)
    {
        if (!context || !context->InitiateCall())
            return false;
        if (m_login)
            m_login->Abort(qResult(QResult::Core_Cancelled));
        m_secure.Close();
        m_principalId  = 0;
        m_connectionId = 0;
        context->SignalSuccess();
        return true;
    }

} // namespace nn::nex
