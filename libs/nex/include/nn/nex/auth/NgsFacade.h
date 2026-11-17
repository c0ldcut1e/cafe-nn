/**
 * @file
 * @brief NgsFacade logs in through the authentication server and then the secure server, and every other protocol runs over the
 * secure connection it leaves open.
 */

#pragma once

#include <nn/nex/auth/BackEndServices.h>
#include <nn/nex/auth/NexServiceToken.h>
#include <nn/nex/runtime/ProtocolCallContext.h>
#include <nn/nex/types.h>
#include <vector>

namespace nn::nex
{

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
