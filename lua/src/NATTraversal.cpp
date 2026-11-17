#include "Binding.h"

namespace nn::nex::lua
{
    NEX_LUA_CLASS(NATTraversalRelayClient);

    namespace
    {
        using Relay = NATTraversalRelayClient;

        // The probe handler finds its client's userdata through the tracked table, so the client doesn't keep itself alive
        int RelayInit(lua_State *L)
        {
            Relay *self = Check<Relay>(L, 1);
            self->Init();
            self->SetProbeHandler(
                    [self](const qList<StationURL> &urls)
                    {
                        lua_State *L = CallbackState();
                        if (!L || !PushTracked(L, self))
                            return;
                        if (GetUserValue(L, -1, "probe") == LUA_TFUNCTION)
                        {
                            lua_insert(L, -2);
                            Push(L, urls);
                            ProtectedCall(L, 2, 0);
                        }
                        else
                        {
                            lua_pop(L, 2);
                        }
                    });
            return 0;
        }

        // The handler gets the client and the list of URLs to probe
        int RelaySetProbeHandler(lua_State *L)
        {
            Check<Relay>(L, 1);
            if (!lua_isnil(L, 2))
                luaL_checktype(L, 2, LUA_TFUNCTION);
            SetUserValue(L, 1, "probe", 2);
            return 0;
        }

        int RelayRequestProbeInitiationExt(lua_State *L)
        {
            Relay *self  = Check<Relay>(L, 1);
            auto targets = Get<qList<StationURL>>(L, 2);
            auto source  = Get<StationURL>(L, 3);
            NewCall(L, 1);
            return Started(L, self->CallRequestProbeInitiationExt(ContextOf(L, -1), targets, source));
        }

        int RelayReportNATTraversalResult(lua_State *L)
        {
            Relay *self = Check<Relay>(L, 1);
            auto cid    = Get<qUnsignedInt32>(L, 2);
            bool result = lua_toboolean(L, 3) != 0;
            auto rtt    = Get<qUnsignedInt32>(L, 4);
            NewCall(L, 1);
            return Started(L, self->CallReportNATTraversalResult(ContextOf(L, -1), cid, result, rtt));
        }

        int RelayReportNATProperties(lua_State *L)
        {
            Relay *self    = Check<Relay>(L, 1);
            auto mapping   = Get<qUnsignedInt32>(L, 2);
            auto filtering = Get<qUnsignedInt32>(L, 3);
            auto rtt       = Get<qUnsignedInt32>(L, 4);
            NewCall(L, 1);
            return Started(L, self->CallReportNATProperties(ContextOf(L, -1), mapping, filtering, rtt));
        }

        int RelayGetRelaySignatureKey(lua_State *L)
        {
            Relay *self = Check<Relay>(L, 1);
            auto &out   = NewCallWith<qInt32, DateTime, String, qUnsignedInt16, qInt32, qUnsignedInt32>(L, 1);
            return Started(L, self->CallGetRelaySignatureKey(ContextOf(L, -1), &std::get<0>(out), &std::get<1>(out), &std::get<2>(out),
                                                             &std::get<3>(out), &std::get<4>(out), &std::get<5>(out)));
        }

        const luaL_Reg RelayMethods[] = {
                {"Init", RelayInit},
                {"SetProbeHandler", RelaySetProbeHandler},
                {"CallRequestProbeInitiationExt", RelayRequestProbeInitiationExt},
                {"CallReportNATTraversalResult", RelayReportNATTraversalResult},
                {"CallReportNATProperties", RelayReportNATProperties},
                {"CallGetRelaySignatureKey", RelayGetRelaySignatureKey},
                {nullptr, nullptr},
        };

        int NewRelay(lua_State *L)
        {
            New<Relay>(L);
            Track(L, -1);
            return 1;
        }
    } // namespace

    void OpenNATTraversal(lua_State *L)
    {
        Define<Relay>(L, "ClientProtocol", RelayMethods);
        SetClass(L, "NATTraversalRelayClient", NewRelay);
    }
} // namespace nn::nex::lua
