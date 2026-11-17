#include "Binding.h"

namespace nn::nex::lua
{
    namespace
    {
        // Maps each call's address to its userdata through weak values, so a completion can find the userdata without keeping it alive
        const char *const CallsKey = "nex.calls";

        void PushCalls(lua_State *L)
        {
            if (lua_getfield(L, LUA_REGISTRYINDEX, CallsKey) == LUA_TTABLE)
                return;
            lua_pop(L, 1);
            lua_newtable(L);
            lua_createtable(L, 0, 1);
            lua_pushliteral(L, "v");
            lua_setfield(L, -2, "__mode");
            lua_setmetatable(L, -2);
            lua_pushvalue(L, -1);
            lua_setfield(L, LUA_REGISTRYINDEX, CallsKey);
        }

        // The callbacks live in the call's user value table under this key, as an array in registration order
        const char *const CallbacksKey = "callbacks";

        void OnComplete(CallContext *, LuaCall *call)
        {
            lua_State *L = CallbackState();
            if (!L)
                return;
            PushCalls(L);
            lua_pushlightuserdata(L, call);
            if (lua_rawget(L, -2) != LUA_TUSERDATA)
            {
                lua_pop(L, 2);
                return;
            }
            qInt32 self = lua_gettop(L);
            if (lua_getiuservalue(L, self, 1) == LUA_TTABLE && lua_getfield(L, -1, CallbacksKey) == LUA_TTABLE)
            {
                lua_Integer n = luaL_len(L, -1);
                for (lua_Integer i = 1; i <= n; i++)
                {
                    lua_rawgeti(L, -1, i);
                    lua_pushvalue(L, self);
                    ProtectedCall(L, 1, 0);
                }
            }
            lua_settop(L, self - 2);
        }

        LuaCall *CheckCall(lua_State *L)
        {
            return Check<LuaCall>(L, 1);
        }

        int CallGetState(lua_State *L)
        {
            return Push(L, CheckCall(L)->context.GetState());
        }

        int CallGetOutcome(lua_State *L)
        {
            return Push(L, CheckCall(L)->context.GetOutcome());
        }

        int CallIsInProgress(lua_State *L)
        {
            return Push(L, CheckCall(L)->context.IsInProgress());
        }

        int CallGetID(lua_State *L)
        {
            return Push(L, CheckCall(L)->context.GetID());
        }

        int CallWait(lua_State *L)
        {
            LuaCall *call    = CheckCall(L);
            unsigned timeout = (unsigned) luaL_optinteger(L, 2, 0xFFFFFFFF);
            bool done;
            {
                ActiveState active(L);
                done = call->context.Wait(timeout);
            }
            RaisePendingError(L);
            return Push(L, done);
        }

        int CallCancel(lua_State *L)
        {
            LuaCall *call = CheckCall(L);
            bool cancelled;
            {
                ActiveState active(L);
                cancelled = call->context.Cancel();
            }
            RaisePendingError(L);
            return Push(L, cancelled);
        }

        // Reset drops every completion callback, so we register ours again to keep Lua callbacks working on the reused call
        int CallReset(lua_State *L)
        {
            LuaCall *call = CheckCall(L);
            if (!call->context.Reset())
                return Push(L, false);
            call->context.RegisterCompletionCallback(new Callback<LuaCall *>(OnComplete, call), true);
            if (lua_getiuservalue(L, 1, 1) == LUA_TTABLE)
            {
                lua_pushnil(L);
                lua_setfield(L, -2, CallbacksKey);
            }
            return Push(L, true);
        }

        int CallInitiateCall(lua_State *L)
        {
            return Push(L, CheckCall(L)->context.InitiateCall());
        }

        int CallSignalSuccess(lua_State *L)
        {
            LuaCall *call = CheckCall(L);
            {
                ActiveState active(L);
                call->context.SignalSuccess(qResult((qUnsignedInt32) luaL_optinteger(L, 2, 0x00010001)));
            }
            RaisePendingError(L);
            return 0;
        }

        int CallSignalFailure(lua_State *L)
        {
            LuaCall *call = CheckCall(L);
            {
                ActiveState active(L);
                call->context.SignalFailure(qResult((qUnsignedInt32) luaL_optinteger(L, 2, 0x80010001)));
            }
            RaisePendingError(L);
            return 0;
        }

        int CallRegisterCompletionCallback(lua_State *L)
        {
            CheckCall(L);
            luaL_checktype(L, 2, LUA_TFUNCTION);
            if (lua_getiuservalue(L, 1, 1) != LUA_TTABLE)
            {
                lua_pop(L, 1);
                lua_newtable(L);
                lua_pushvalue(L, -1);
                lua_setiuservalue(L, 1, 1);
            }
            if (lua_getfield(L, -1, CallbacksKey) != LUA_TTABLE)
            {
                lua_pop(L, 1);
                lua_newtable(L);
                lua_pushvalue(L, -1);
                lua_setfield(L, -3, CallbacksKey);
            }
            lua_pushvalue(L, 2);
            lua_rawseti(L, -2, luaL_len(L, -2) + 1);
            return 0;
        }

        // The outputs are only meaningful once the server answered, so a call that has not succeeded returns nil and its outcome
        int CallGetResult(lua_State *L)
        {
            LuaCall *call = CheckCall(L);
            if (call->context.GetState() != CallContext::CallSuccess)
            {
                lua_pushnil(L);
                Push(L, call->context.GetOutcome());
                return 2;
            }
            if (!call->outputs)
                return 0;
            return call->outputs->Push(L);
        }

        const luaL_Reg CallMethods[] = {
                {"GetState", CallGetState},
                {"GetOutcome", CallGetOutcome},
                {"IsInProgress", CallIsInProgress},
                {"GetID", CallGetID},
                {"Wait", CallWait},
                {"Cancel", CallCancel},
                {"Reset", CallReset},
                {"InitiateCall", CallInitiateCall},
                {"SignalSuccess", CallSignalSuccess},
                {"SignalFailure", CallSignalFailure},
                {"RegisterCompletionCallback", CallRegisterCompletionCallback},
                {"GetResult", CallGetResult},
                {nullptr, nullptr},
        };

        // Destroying an in-progress context cancels it, which can run completion callbacks, so this happens with the collector's
        // thread active
        int CollectCall(lua_State *L)
        {
            ActiveState active(L);
            return CollectOnce<LuaCall>(L);
        }

        int NewProtocolCallContext(lua_State *L)
        {
            NewCall(L, 0);
            return 1;
        }

        int Dispatch(lua_State *L)
        {
            unsigned timeout = (unsigned) luaL_optinteger(L, 1, 0);
            unsigned maxJobs = (unsigned) luaL_optinteger(L, 2, 0xFFFFFFFF);
            {
                ActiveState active(L);
                Scheduler::GetInstance()->Dispatch(timeout, maxJobs);
            }
            RaisePendingError(L);
            return 0;
        }

        const std::pair<const char *, lua_Integer> CallStates[] = {
                {"Available", CallContext::Available},     {"CallInProgress", CallContext::CallInProgress}, {"CallSuccess", CallContext::CallSuccess},
                {"CallFailure", CallContext::CallFailure}, {"CallCancelled", CallContext::CallCancelled},
        };
    } // namespace

    LuaCall *NewCall(lua_State *L, int owner)
    {
        owner         = owner ? lua_absindex(L, owner) : 0;
        LuaCall *call = New<LuaCall>(L);
        call->context.RegisterCompletionCallback(new Callback<LuaCall *>(OnComplete, call), true);
        PushCalls(L);
        lua_pushlightuserdata(L, call);
        lua_pushvalue(L, -3);
        lua_rawset(L, -3);
        lua_pop(L, 1);
        if (owner)
            KeepAlive(L, -1, owner);
        return call;
    }

    void OpenRuntime(lua_State *L)
    {
        DefineClass(L, ClassName<LuaCall>::value, nullptr, CallMethods, nullptr, CollectCall);
        SetClass(L, "ProtocolCallContext", NewProtocolCallContext);
        SetFunction(L, "Dispatch", Dispatch);
        SetConstants(L, "CallContext", CallStates, std::size(CallStates));
    }
} // namespace nn::nex::lua
