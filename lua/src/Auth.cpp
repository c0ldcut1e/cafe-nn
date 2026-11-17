#include "Binding.h"
#include <nn/nex/auth/AuthenticationInfo.h>
#include <nn/nex/auth/NintendoLoginData.h>
#include <nn/nex/auth/NotificationEvent.h>
#include <nn/nex/auth/NotificationEventHandler.h>
#include <nn/nex/auth/RVConnectionData.h>
#include <nn/nex/auth/SecureConnectionProtocolClient.h>
#include <nn/nex/auth/TicketGrantingProtocolClient.h>
#include <nn/nex/types.h>

namespace nn::nex::lua
{
    class LuaNotificationHandler : public NotificationEventHandler
    {
    public:
        void ProcessNotificationEvent(const NotificationEvent &event) override
        {
            lua_State *L = CallbackState();
            if (!L || !PushTracked(L, this))
                return;
            qInt32 self = lua_gettop(L);
            if (GetUserValue(L, self, "callback") == LUA_TFUNCTION)
            {
                Push(L, event);
                ProtectedCall(L, 1, 0);
            }
            else
            {
                lua_pop(L, 1);
            }
            lua_pop(L, 1);
        }
    };

    NEX_LUA_CLASS(TicketGrantingProtocolClient);
    NEX_LUA_CLASS(SecureConnectionProtocolClient);
    NEX_LUA_CLASS(NgsFacade);
    NEX_LUA_CLASS(AuthenticationInfo);
    NEX_LUA_CLASS(NintendoLoginData);

    template<>
    struct ClassName<LuaNotificationHandler>
    {
        static constexpr const char *value = "NotificationEventHandler";
    };

    int Push(lua_State *L, const RVConnectionData &v)
    {
        lua_createtable(L, 0, 4);
        Push(L, v.GetRegularProtocols());
        lua_setfield(L, -2, "RegularProtocols");
        Push(L, v.GetSpecialProtocols());
        lua_setfield(L, -2, "SpecialProtocols");
        Push(L, v.GetSpecialProtocolsURL());
        lua_setfield(L, -2, "SpecialProtocolsURL");
        Push(L, v.GetCurrentUTCTime());
        lua_setfield(L, -2, "CurrentUTCTime");
        return 1;
    }

    namespace
    {
        // Protocols can bind to an EndPoint of their own or to the secure endpoint an NgsFacade logged in with
        EndPoint *CheckEndPoint(lua_State *L, int index)
        {
            if (EndPoint *endPoint = Test<EndPoint>(L, index))
                return endPoint;
            if (NgsFacade *facade = Test<NgsFacade>(L, index))
                return facade->GetSecureEndPoint();
            luaL_typeerror(L, index, "EndPoint or NgsFacade");
            return nullptr;
        }

        int NewEndPoint(lua_State *L)
        {
            New<EndPoint>(L);
            return 1;
        }

        int EndPointConnect(lua_State *L)
        {
            EndPoint *self  = Check<EndPoint>(L, 1);
            StationURL url  = Get<StationURL>(L, 2);
            const char *key = luaL_checkstring(L, 3);
            Buffer payload  = lua_isnoneornil(L, 4) ? Buffer() : Get<Buffer>(L, 4);
            bool hasSession = !lua_isnoneornil(L, 5);
            Buffer session  = hasSession ? Get<Buffer>(L, 5) : Buffer();
            LuaCall *call   = NewCall(L, 1);
            qResult result  = self->Connect(&call->context, url, key, payload, hasSession ? &session : nullptr);
            Push(L, result);
            return 2;
        }

        int EndPointClose(lua_State *L)
        {
            Check<EndPoint>(L, 1)->Close();
            return 0;
        }

        const luaL_Reg EndPointMethods[] = {
                {"Connect", EndPointConnect},
                {"Close", EndPointClose},
                NEX_LUA_GET(EndPoint, IsConnected),
                {nullptr, nullptr},
        };

        // The protocol keeps what it is bound to alive, since its calls go through that endpoint
        int ClientProtocolBind(lua_State *L)
        {
            ClientProtocol *self = Check<ClientProtocol>(L, 1);
            self->Bind(CheckEndPoint(L, 2));
            KeepAlive(L, 1, 2);
            return 0;
        }

        const luaL_Reg ClientProtocolMethods[] = {
                {"Bind", ClientProtocolBind},
                NEX_LUA_GET(ClientProtocol, GetProtocolID),
                {nullptr, nullptr},
        };

        int TicketGrantingLogin(lua_State *L)
        {
            auto *self  = Check<TicketGrantingProtocolClient>(L, 1);
            String user = Get<String>(L, 2);
            auto &out   = NewCallWith<qResult, qUnsignedInt32, Buffer, RVConnectionData, String>(L, 1);
            return Started(L, self->Login(ContextOf(L, -1), user, &std::get<0>(out), &std::get<1>(out), &std::get<2>(out), &std::get<3>(out),
                                          &std::get<4>(out)));
        }

        int TicketGrantingLoginEx(lua_State *L)
        {
            auto *self  = Check<TicketGrantingProtocolClient>(L, 1);
            String user = Get<String>(L, 2);
            Data *extra = Check<Data>(L, 3);
            auto &out   = NewCallWith<qResult, qUnsignedInt32, Buffer, RVConnectionData, String>(L, 1);
            return Started(L, self->LoginEx(ContextOf(L, -1), user, *extra, &std::get<0>(out), &std::get<1>(out), &std::get<2>(out),
                                            &std::get<3>(out), &std::get<4>(out)));
        }

        int TicketGrantingRequestTicket(lua_State *L)
        {
            auto *self = Check<TicketGrantingProtocolClient>(L, 1);
            auto src   = Get<qUnsignedInt32>(L, 2);
            auto tgt   = Get<qUnsignedInt32>(L, 3);
            auto &out  = NewCallWith<qResult, Buffer>(L, 1);
            return Started(L, self->RequestTicket(ContextOf(L, -1), src, tgt, &std::get<0>(out), &std::get<1>(out)));
        }

        const luaL_Reg TicketGrantingMethods[] = {
                {"Login", TicketGrantingLogin},
                {"LoginEx", TicketGrantingLoginEx},
                {"RequestTicket", TicketGrantingRequestTicket},
                {nullptr, nullptr},
        };

        // Byte arrays come from Lua as strings, since a table of numbers is a clumsy way to hold binary data
        qVector<qByte> ReadBytes(lua_State *L, int index)
        {
            size_t size;
            const char *data = luaL_checklstring(L, index, &size);
            return qVector<qByte>(data, data + size);
        }

        int SecureRegister(lua_State *L)
        {
            auto *self = Check<SecureConnectionProtocolClient>(L, 1);
            auto urls  = Get<qList<StationURL>>(L, 2);
            auto &out  = NewCallWith<qResult, qUnsignedInt32, StationURL>(L, 1);
            return Started(L, self->Register(ContextOf(L, -1), urls, &std::get<0>(out), &std::get<1>(out), &std::get<2>(out)));
        }

        int SecureRegisterEx(lua_State *L)
        {
            auto *self  = Check<SecureConnectionProtocolClient>(L, 1);
            auto urls   = Get<qList<StationURL>>(L, 2);
            Data *login = Check<Data>(L, 3);
            auto &out   = NewCallWith<qResult, qUnsignedInt32, StationURL>(L, 1);
            return Started(L, self->RegisterEx(ContextOf(L, -1), urls, *login, &std::get<0>(out), &std::get<1>(out), &std::get<2>(out)));
        }

        int SecureRequestURLs(lua_State *L)
        {
            auto *self = Check<SecureConnectionProtocolClient>(L, 1);
            auto cid   = Get<qUnsignedInt32>(L, 2);
            auto pid   = Get<qUnsignedInt32>(L, 3);
            auto &out  = NewCallWith<bool, qList<StationURL>>(L, 1);
            return Started(L, self->RequestURLs(ContextOf(L, -1), cid, pid, &std::get<0>(out), &std::get<1>(out)));
        }

        int SecureUpdateURLs(lua_State *L)
        {
            auto *self = Check<SecureConnectionProtocolClient>(L, 1);
            auto urls  = Get<qList<StationURL>>(L, 2);
            NewCall(L, 1);
            return Started(L, self->UpdateURLs(ContextOf(L, -1), urls));
        }

        int SecureReplaceURL(lua_State *L)
        {
            auto *self = Check<SecureConnectionProtocolClient>(L, 1);
            auto from  = Get<StationURL>(L, 2);
            auto to    = Get<StationURL>(L, 3);
            NewCall(L, 1);
            return Started(L, self->ReplaceURL(ContextOf(L, -1), from, to));
        }

        int SecureSendReport(lua_State *L)
        {
            auto *self = Check<SecureConnectionProtocolClient>(L, 1);
            auto id    = Get<qUnsignedInt32>(L, 2);
            auto data  = ReadBytes(L, 3);
            NewCall(L, 1);
            return Started(L, self->SendReport(ContextOf(L, -1), id, data));
        }

        const luaL_Reg SecureConnectionMethods[] = {
                {"Register", SecureRegister},
                {"RegisterEx", SecureRegisterEx},
                {"RequestURLs", SecureRequestURLs},
                {"UpdateURLs", SecureUpdateURLs},
                {"ReplaceURL", SecureReplaceURL},
                {"SendReport", SecureSendReport},
                {nullptr, nullptr},
        };

        int NewNotificationHandler(lua_State *L)
        {
            luaL_checktype(L, 1, LUA_TFUNCTION);
            New<LuaNotificationHandler>(L);
            SetUserValue(L, -1, "callback", 1);
            Track(L, -1);
            return 1;
        }

        int NgsFacadeLogin(lua_State *L)
        {
            auto *self        = Check<NgsFacade>(L, 1);
            auto gameServerId = Get<qUnsignedInt32>(L, 2);
            String accessKey  = Get<String>(L, 3);
            qInt32 nexVersion = (qInt32) luaL_optinteger(L, 4, 0);
            auto timeout      = (qUnsignedInt32) luaL_optinteger(L, 5, 0);
            NewCall(L, 1);
            bool sent;
            {
                // Login asks the token provider right away, which may be a Lua function
                ActiveState active(L);
                sent = self->Login(ContextOf(L, -1), gameServerId, accessKey.CStr(), nexVersion, timeout);
            }
            RaisePendingError(L);
            return Started(L, sent);
        }

        int NgsFacadeLogout(lua_State *L)
        {
            auto *self = Check<NgsFacade>(L, 1);
            NewCall(L, 1);
            return Started(L, self->Logout(ContextOf(L, -1)));
        }

        int NgsFacadeRegisterHandler(lua_State *L)
        {
            Check<NgsFacade>(L, 1)->RegisterNotificationEventHandler(Check<LuaNotificationHandler>(L, 2));
            KeepAlive(L, 1, 2);
            return 0;
        }

        int NgsFacadeUnregisterHandler(lua_State *L)
        {
            Check<NgsFacade>(L, 1)->UnregisterNotificationEventHandler(Check<LuaNotificationHandler>(L, 2));
            Release(L, 1, 2);
            return 0;
        }

        // The facade and the protocol keep each other alive, so neither is collected while the other is still reachable
        int NgsFacadeRegisterProtocol(lua_State *L)
        {
            Check<NgsFacade>(L, 1)->RegisterProtocol(Check<ClientProtocol>(L, 2));
            KeepAlive(L, 1, 2);
            KeepAlive(L, 2, 1);
            return 0;
        }

        const luaL_Reg NgsFacadeMethods[] = {
                {"Login", NgsFacadeLogin},
                {"Logout", NgsFacadeLogout},
                {"RegisterNotificationEventHandler", NgsFacadeRegisterHandler},
                {"UnregisterNotificationEventHandler", NgsFacadeUnregisterHandler},
                {"RegisterProtocol", NgsFacadeRegisterProtocol},
                NEX_LUA_GET(NgsFacade, GetLastLoginErrorCode),
                NEX_LUA_GET(NgsFacade, IsConnected),
                NEX_LUA_GET(NgsFacade, GetPrincipalID),
                NEX_LUA_GET(NgsFacade, GetConnectionID),
                NEX_LUA_GET(NgsFacade, GetPublicURL),
                NEX_LUA_GET(NgsFacade, GetLocalURLs),
                {nullptr, nullptr},
        };

        const char *const TokenProviderKey = "nex.tokenprovider";

        // The Lua provider returns a table with the NexServiceToken fields, or nil and an optional QResult code when it has no token
        qResult LuaTokenProvider(qUnsignedInt32 gameServerId, NexServiceToken *out)
        {
            lua_State *L = CallbackState();
            if (!L)
                return qResult(QResult::Core_InitializationFailure);
            qInt32 top = lua_gettop(L);
            if (lua_getfield(L, LUA_REGISTRYINDEX, TokenProviderKey) != LUA_TFUNCTION)
            {
                lua_settop(L, top);
                return qResult(QResult::Core_InitializationFailure);
            }
            lua_pushinteger(L, gameServerId);
            qResult result(QResult::Core_InitializationFailure);
            if (ProtectedCall(L, 1, 2))
            {
                if (lua_istable(L, -2))
                {
                    lua_getfield(L, -2, "token");
                    lua_getfield(L, -3, "nexPassword");
                    lua_getfield(L, -4, "principalId");
                    lua_getfield(L, -5, "host");
                    lua_getfield(L, -6, "port");
                    out->token       = ToString(L, -5);
                    out->nexPassword = ToString(L, -4);
                    out->principalId = (qUnsignedInt32) lua_tointeger(L, -3);
                    out->host        = ToString(L, -2);
                    out->port        = (qUnsignedInt16) lua_tointeger(L, -1);
                    result           = qResult(true);
                }
                else if (lua_isinteger(L, -1))
                {
                    result = qResult((qUnsignedInt32) lua_tointeger(L, -1));
                }
            }
            lua_settop(L, top);
            return result;
        }

        // Passing nil takes the provider out again, which leaves the platform's own provider in charge
        int SetTokenProvider(lua_State *L)
        {
            bool clear = lua_isnoneornil(L, 1);
            if (!clear)
                luaL_checktype(L, 1, LUA_TFUNCTION);
            lua_settop(L, 1);
            lua_setfield(L, LUA_REGISTRYINDEX, TokenProviderKey);
            NgsFacade::SetTokenProvider(clear ? nullptr : LuaTokenProvider);
            return 0;
        }

        const luaL_Reg NoMethods[] = {{nullptr, nullptr}};

        const Field AuthenticationInfoData[] = {
                NEX_LUA_FIELD(AuthenticationInfo, token),
                NEX_LUA_FIELD(AuthenticationInfo, ngsVersion),
                NEX_LUA_FIELD(AuthenticationInfo, tokenType),
                NEX_LUA_FIELD(AuthenticationInfo, serverVersion),
                {nullptr, nullptr, nullptr},
        };

        const Field NintendoLoginDataFields[] = {
                NEX_LUA_FIELD(NintendoLoginData, token),
                {nullptr, nullptr, nullptr},
        };

        const luaL_Reg NotificationEventMethods[] = {
                NEX_LUA_GET(NotificationEvent, GetSource),
                NEX_LUA_GET(NotificationEvent, GetType),
                NEX_LUA_GET(NotificationEvent, GetMainType),
                NEX_LUA_GET(NotificationEvent, GetSubType),
                NEX_LUA_GET(NotificationEvent, GetParam1),
                NEX_LUA_GET(NotificationEvent, GetParam2),
                NEX_LUA_GET(NotificationEvent, GetParam3),
                NEX_LUA_GET(NotificationEvent, GetStringParam),
                NEX_LUA_SET(NotificationEvent, SetSource, qUnsignedInt32),
                NEX_LUA_SET(NotificationEvent, SetType, qUnsignedInt32),
                NEX_LUA_SET(NotificationEvent, SetParam1, qUnsignedInt32),
                NEX_LUA_SET(NotificationEvent, SetParam2, qUnsignedInt32),
                NEX_LUA_SET(NotificationEvent, SetParam3, qUnsignedInt32),
                NEX_LUA_SET(NotificationEvent, SetStringParam, String),
                {nullptr, nullptr},
        };

        const std::pair<const char *, lua_Integer> NotificationTypes[] = {
                {"ParticipationEvent", NotificationEvents::ParticipationEvent},
                {"OwnershipChangeEvent", NotificationEvents::OwnershipChangeEvent},
                {"GameNotificationType1", NotificationEvents::GameNotificationType1},
                {"GatheringUnregistered", NotificationEvents::GatheringUnregistered},
                {"HostChangeEvent", NotificationEvents::HostChangeEvent},
                {"SwitchGathering", NotificationEvents::SwitchGathering},
        };
    } // namespace

    void OpenAuthentication(lua_State *L)
    {
        Define<EndPoint>(L, nullptr, EndPointMethods);
        Define<ClientProtocol>(L, nullptr, ClientProtocolMethods);
        Define<TicketGrantingProtocolClient>(L, "ClientProtocol", TicketGrantingMethods);
        Define<SecureConnectionProtocolClient>(L, "ClientProtocol", SecureConnectionMethods);
        Define<NgsFacade>(L, nullptr, NgsFacadeMethods);
        Define<LuaNotificationHandler>(L, nullptr, nullptr);
        DefineData<AuthenticationInfo>(L, nullptr, NoMethods, AuthenticationInfoData);
        DefineData<NintendoLoginData>(L, nullptr, NoMethods, NintendoLoginDataFields);
        DefineData<NotificationEvent>(L, nullptr, NotificationEventMethods);

        const luaL_Reg facadeStatics[] = {{"SetTokenProvider", SetTokenProvider}, {nullptr, nullptr}};
        SetClass(L, "EndPoint", NewEndPoint);
        SetClass(L, "TicketGrantingProtocolClient", Construct<TicketGrantingProtocolClient>);
        SetClass(L, "SecureConnectionProtocolClient", Construct<SecureConnectionProtocolClient>);
        SetClass(L, "NgsFacade", Construct<NgsFacade>, facadeStatics);
        SetClass(L, "NotificationEventHandler", NewNotificationHandler);
        SetClass(L, "AuthenticationInfo", Construct<AuthenticationInfo>);
        SetClass(L, "NintendoLoginData", Construct<NintendoLoginData>);
        SetClass(L, "NotificationEvent", Construct<NotificationEvent>);
        SetConstants(L, "NotificationEvents", NotificationTypes, std::size(NotificationTypes));
    }
} // namespace nn::nex::lua
