#include "Binding.h"

#include <map>
#include <string>

namespace nn::nex::lua
{
    namespace
    {
        lua_State *g_active = nullptr;

        const char *const PendingErrorKey = "nex.pending";

        std::map<std::string, DataPusher> &DataPushers()
        {
            static std::map<std::string, DataPusher> pushers;
            return pushers;
        }

        int Index(lua_State *L)
        {
            lua_getmetatable(L, 1);
            lua_getfield(L, -1, "__methods");
            lua_pushvalue(L, 2);
            if (lua_rawget(L, -2) != LUA_TNIL)
                return 1;
            lua_pop(L, 2);
            lua_getfield(L, -1, "__fields");
            lua_pushvalue(L, 2);
            if (lua_rawget(L, -2) != LUA_TTABLE)
                return 0;
            lua_rawgeti(L, -1, 1);
            lua_pushvalue(L, 1);
            lua_call(L, 1, 1);
            return 1;
        }

        int NewIndex(lua_State *L)
        {
            lua_getmetatable(L, 1);
            lua_getfield(L, -1, "__fields");
            lua_pushvalue(L, 2);
            if (lua_rawget(L, -2) != LUA_TTABLE)
                return luaL_error(L, "%s has no field '%s'", luaL_typename(L, 1), lua_tostring(L, 2));
            lua_rawgeti(L, -1, 2);
            lua_pushvalue(L, 1);
            lua_pushvalue(L, 2);
            lua_pushvalue(L, 3);
            lua_call(L, 3, 0);
            return 0;
        }

        // Entries already in to win, so a class overrides the methods it inherits
        void Inherit(lua_State *L, int to, int from)
        {
            to   = lua_absindex(L, to);
            from = lua_absindex(L, from);
            lua_pushnil(L);
            while (lua_next(L, from))
            {
                lua_pushvalue(L, -2);
                if (lua_rawget(L, to) == LUA_TNIL)
                {
                    lua_pop(L, 1);
                    lua_pushvalue(L, -2);
                    lua_pushvalue(L, -2);
                    lua_rawset(L, to);
                }
                else
                    lua_pop(L, 1);
                lua_pop(L, 1);
            }
        }

        int ToStringOf(lua_State *L)
        {
            lua_getmetatable(L, 1);
            lua_getfield(L, -1, "__name");
            lua_pushfstring(L, "%s: %p", lua_tostring(L, -1), lua_touserdata(L, 1));
            return 1;
        }

        int CallClass(lua_State *L)
        {
            lua_getfield(L, 1, "__new");
            lua_replace(L, 1);
            lua_call(L, lua_gettop(L) - 1, LUA_MULTRET);
            return lua_gettop(L);
        }
    } // namespace

    void DefineClass(lua_State *L, const char *name, const char *base, const luaL_Reg *methods, const Field *fields, lua_CFunction gc)
    {
        luaL_newmetatable(L, name);
        qInt32 mt = lua_gettop(L);

        lua_newtable(L);
        if (methods)
            luaL_setfuncs(L, methods, 0);
        lua_setfield(L, mt, "__methods");

        lua_newtable(L);
        for (const Field *f = fields; f && f->name; f++)
        {
            lua_createtable(L, 2, 0);
            lua_pushcfunction(L, f->get);
            lua_rawseti(L, -2, 1);
            lua_pushcfunction(L, f->set);
            lua_rawseti(L, -2, 2);
            lua_setfield(L, -2, f->name);
        }
        lua_setfield(L, mt, "__fields");

        lua_newtable(L);
        lua_pushboolean(L, 1);
        lua_setfield(L, -2, name);
        lua_setfield(L, mt, "__is");

        if (base)
        {
            if (luaL_getmetatable(L, base) != LUA_TTABLE)
                luaL_error(L, "%s is defined before its base %s", name, base);
            for (const char *key : {"__methods", "__fields", "__is"})
            {
                lua_getfield(L, mt, key);
                lua_getfield(L, -2, key);
                Inherit(L, -2, -1);
                lua_pop(L, 2);
            }
            // Metamethods like __eq and __tostring are inherited too, unless the class set its own
            for (const char *key : {"__eq", "__tostring", "__lt", "__le"})
            {
                if (lua_getfield(L, -1, key) != LUA_TNIL)
                    lua_setfield(L, mt, key);
                else
                    lua_pop(L, 1);
            }
            lua_pop(L, 1);
        }

        lua_pushcfunction(L, Index);
        lua_setfield(L, mt, "__index");
        lua_pushcfunction(L, NewIndex);
        lua_setfield(L, mt, "__newindex");
        if (lua_getfield(L, mt, "__tostring") == LUA_TNIL)
        {
            lua_pushcfunction(L, ToStringOf);
            lua_setfield(L, mt, "__tostring");
        }
        lua_pop(L, 1);
        if (gc)
        {
            lua_pushcfunction(L, gc);
            lua_setfield(L, mt, "__gc");
        }
        lua_pop(L, 1);
    }

    void *TestClass(lua_State *L, int index, const char *name)
    {
        void *p = lua_touserdata(L, index);
        if (!p || !lua_getmetatable(L, index))
            return nullptr;
        bool is = false;
        if (lua_getfield(L, -1, "__is") == LUA_TTABLE)
        {
            is = lua_getfield(L, -1, name) == LUA_TBOOLEAN;
            lua_pop(L, 1);
        }
        lua_pop(L, 2);
        return is ? p : nullptr;
    }

    void *CheckClass(lua_State *L, int index, const char *name)
    {
        void *p = TestClass(L, index, name);
        if (!p)
            luaL_typeerror(L, index, name);
        return p;
    }

    lua_State *CallbackState()
    {
        return g_active;
    }

    ActiveState::ActiveState(lua_State *L) : m_previous(g_active)
    {
        g_active = L;
    }

    ActiveState::~ActiveState()
    {
        g_active = m_previous;
    }

    void SetPendingError(lua_State *L)
    {
        if (lua_getfield(L, LUA_REGISTRYINDEX, PendingErrorKey) == LUA_TNIL)
        {
            lua_pushvalue(L, -2);
            lua_setfield(L, LUA_REGISTRYINDEX, PendingErrorKey);
        }
        lua_pop(L, 2);
    }

    void RaisePendingError(lua_State *L)
    {
        if (lua_getfield(L, LUA_REGISTRYINDEX, PendingErrorKey) == LUA_TNIL)
        {
            lua_pop(L, 1);
            return;
        }
        lua_pushnil(L);
        lua_setfield(L, LUA_REGISTRYINDEX, PendingErrorKey);
        lua_error(L);
    }

    bool ProtectedCall(lua_State *L, int args, int results)
    {
        if (lua_pcall(L, args, results, 0) == LUA_OK)
            return true;
        SetPendingError(L);
        return false;
    }

    namespace
    {
        void PushUserValueTable(lua_State *L, int index);
    } // namespace

    void KeepAlive(lua_State *L, int owner, int value)
    {
        owner = lua_absindex(L, owner);
        value = lua_absindex(L, value);
        PushUserValueTable(L, owner);
        lua_pushvalue(L, value);
        lua_pushboolean(L, 1);
        lua_rawset(L, -3);
        lua_pop(L, 1);
    }

    void Release(lua_State *L, int owner, int value)
    {
        owner = lua_absindex(L, owner);
        value = lua_absindex(L, value);
        if (lua_getiuservalue(L, owner, 1) == LUA_TTABLE)
        {
            lua_pushvalue(L, value);
            lua_pushnil(L);
            lua_rawset(L, -3);
        }
        lua_pop(L, 1);
    }

    namespace
    {
        const char *const TrackedKey = "nex.tracked";

        void PushTrackedTable(lua_State *L)
        {
            if (lua_getfield(L, LUA_REGISTRYINDEX, TrackedKey) == LUA_TTABLE)
                return;
            lua_pop(L, 1);
            lua_newtable(L);
            lua_createtable(L, 0, 1);
            lua_pushliteral(L, "v");
            lua_setfield(L, -2, "__mode");
            lua_setmetatable(L, -2);
            lua_pushvalue(L, -1);
            lua_setfield(L, LUA_REGISTRYINDEX, TrackedKey);
        }

        void PushUserValueTable(lua_State *L, int index)
        {
            if (lua_getiuservalue(L, index, 1) != LUA_TTABLE)
            {
                lua_pop(L, 1);
                lua_newtable(L);
                lua_pushvalue(L, -1);
                lua_setiuservalue(L, index, 1);
            }
        }
    } // namespace

    void Track(lua_State *L, int index)
    {
        index = lua_absindex(L, index);
        PushTrackedTable(L);
        lua_pushlightuserdata(L, lua_touserdata(L, index));
        lua_pushvalue(L, index);
        lua_rawset(L, -3);
        lua_pop(L, 1);
    }

    bool PushTracked(lua_State *L, const void *object)
    {
        PushTrackedTable(L);
        lua_pushlightuserdata(L, const_cast<void *>(object));
        if (lua_rawget(L, -2) != LUA_TUSERDATA)
        {
            lua_pop(L, 2);
            return false;
        }
        lua_remove(L, -2);
        return true;
    }

    int GetUserValue(lua_State *L, int index, const char *key)
    {
        if (lua_getiuservalue(L, index, 1) != LUA_TTABLE)
            return LUA_TNIL;
        qInt32 type = lua_getfield(L, -1, key);
        lua_remove(L, -2);
        return type;
    }

    void SetUserValue(lua_State *L, int index, const char *key, int value)
    {
        index = lua_absindex(L, index);
        value = lua_absindex(L, value);
        PushUserValueTable(L, index);
        lua_pushvalue(L, value);
        lua_setfield(L, -2, key);
        lua_pop(L, 1);
    }

    String ToString(lua_State *L, int index)
    {
        String s;
        s.Assign(lua_tostring(L, index));
        return s;
    }

    int Push(lua_State *L, bool v)
    {
        lua_pushboolean(L, v);
        return 1;
    }

    int Push(lua_State *L, double v)
    {
        lua_pushnumber(L, v);
        return 1;
    }

    int Push(lua_State *L, const char *v)
    {
        lua_pushstring(L, v);
        return 1;
    }

    int Push(lua_State *L, const String &v)
    {
        std::string utf8 = v.ToUtf8();
        lua_pushlstring(L, utf8.data(), utf8.size());
        return 1;
    }

    int Push(lua_State *L, const Buffer &v)
    {
        lua_pushlstring(L, (const char *) v.GetContentPtr(), v.GetContentSize());
        return 1;
    }

    int Push(lua_State *L, qResult v)
    {
        lua_pushinteger(L, v.GetReturnCode());
        return 1;
    }

    int Push(lua_State *L, const DateTime &v)
    {
        New<DateTime>(L, v);
        return 1;
    }

    int Push(lua_State *L, const StationURL &v)
    {
        New<StationURL>(L, v);
        return 1;
    }

    int Push(lua_State *L, const InetAddress &v)
    {
        New<InetAddress>(L, v);
        return 1;
    }

    int Push(lua_State *L, const ResultRange &v)
    {
        New<ResultRange>(L, v);
        return 1;
    }

    DateTime Reader<DateTime>::Read(lua_State *L, int index)
    {
        if (lua_isinteger(L, index))
            return DateTime((qUnsignedInt64) lua_tointeger(L, index));
        return *Check<DateTime>(L, index);
    }

    StationURL Reader<StationURL>::Read(lua_State *L, int index)
    {
        if (lua_type(L, index) == LUA_TSTRING)
            return StationURL(ToString(L, index));
        return *Check<StationURL>(L, index);
    }

    ResultRange Reader<ResultRange>::Read(lua_State *L, int index)
    {
        if (lua_isnoneornil(L, index))
            return ResultRange();
        return *Check<ResultRange>(L, index);
    }

    void SetFunction(lua_State *L, const char *name, lua_CFunction f)
    {
        lua_pushcfunction(L, f);
        lua_setfield(L, -2, name);
    }

    namespace
    {
        void GetOrCreateTable(lua_State *L, const char *name)
        {
            if (lua_getfield(L, -1, name) != LUA_TTABLE)
            {
                lua_pop(L, 1);
                lua_newtable(L);
                lua_pushvalue(L, -1);
                lua_setfield(L, -3, name);
            }
        }
    } // namespace

    void SetFunctions(lua_State *L, const char *table, const luaL_Reg *functions)
    {
        GetOrCreateTable(L, table);
        luaL_setfuncs(L, functions, 0);
        lua_pop(L, 1);
    }

    void SetConstants(lua_State *L, const char *table, const std::pair<const char *, lua_Integer> *constants, size_t count)
    {
        GetOrCreateTable(L, table);
        for (size_t i = 0; i < count; i++)
        {
            lua_pushinteger(L, constants[i].second);
            lua_setfield(L, -2, constants[i].first);
        }
        lua_pop(L, 1);
    }

    void SetClass(lua_State *L, const char *name, lua_CFunction ctor, const luaL_Reg *statics)
    {
        GetOrCreateTable(L, name);
        if (ctor)
        {
            lua_pushcfunction(L, ctor);
            lua_setfield(L, -2, "__new");
            lua_createtable(L, 0, 1);
            lua_pushcfunction(L, CallClass);
            lua_setfield(L, -2, "__call");
            lua_setmetatable(L, -2);
        }
        if (statics)
            luaL_setfuncs(L, statics, 0);
        lua_pop(L, 1);
    }

    void SetMetamethod(lua_State *L, const char *className, const char *event, lua_CFunction f)
    {
        luaL_getmetatable(L, className);
        lua_pushcfunction(L, f);
        lua_setfield(L, -2, event);
        lua_pop(L, 1);
    }

    void RegisterData(const char *typeName, DataPusher pusher)
    {
        DataPushers()[typeName] = pusher;
    }

    bool PushData(lua_State *L, const Data &data)
    {
        auto it = DataPushers().find(data.GetDataTypeName());
        if (it == DataPushers().end())
            return false;
        it->second(L, data);
        return true;
    }
} // namespace nn::nex::lua
