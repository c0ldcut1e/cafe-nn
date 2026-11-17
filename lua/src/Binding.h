#pragma once

#include <lauxlib.h>
#include <lua.h>
#include <memory>
#include <new>
#include <nn/nex.h>
#include <tuple>
#include <type_traits>

namespace nn::nex::lua
{
    template<typename T>
    struct ClassName;

#define NEX_LUA_CLASS(T)                                                                                                                             \
    template<>                                                                                                                                       \
    struct ClassName<T>                                                                                                                              \
    {                                                                                                                                                \
        static constexpr const char *value = #T;                                                                                                     \
    }

    NEX_LUA_CLASS(Data);
    NEX_LUA_CLASS(DateTime);
    NEX_LUA_CLASS(StationURL);
    NEX_LUA_CLASS(InetAddress);
    NEX_LUA_CLASS(ResultRange);
    NEX_LUA_CLASS(EndPoint);
    NEX_LUA_CLASS(ClientProtocol);
    NEX_LUA_CLASS(NotificationEvent);

    struct Field
    {
        const char *name;
        lua_CFunction get;
        lua_CFunction set;
    };

    void DefineClass(lua_State *L, const char *name, const char *base, const luaL_Reg *methods, const Field *fields, lua_CFunction gc);
    void *CheckClass(lua_State *L, int index, const char *name);
    void *TestClass(lua_State *L, int index, const char *name);

    // Callbacks run on the Lua thread that is currently inside a binding function such as Dispatch or Wait. This is null when C++
    // dispatched the scheduler itself, and Lua callbacks are skipped then because no Lua thread is running to call them on.
    lua_State *CallbackState();

    class ActiveState
    {
    public:
        explicit ActiveState(lua_State *L);
        ~ActiveState();

    private:
        lua_State *m_previous;
    };

    // Errors raised inside callbacks can't cross the C++ frames that called them, so we keep the first one and raise it once control is
    // back in a binding function
    void SetPendingError(lua_State *L);
    void RaisePendingError(lua_State *L);

    bool ProtectedCall(lua_State *L, int args, int results);

    template<typename T>
    T *Check(lua_State *L, int index)
    {
        // Every bound class has single inheritance from its root, so a pointer to the object is also a pointer to each of its bases
        return static_cast<T *>(CheckClass(L, index, ClassName<T>::value));
    }

    template<typename T>
    T *Test(lua_State *L, int index)
    {
        return static_cast<T *>(TestClass(L, index, ClassName<T>::value));
    }

    template<typename T, typename... A>
    T *New(lua_State *L, A &&...args)
    {
        void *p   = lua_newuserdatauv(L, sizeof(T), 1);
        T *object = new (p) T(std::forward<A>(args)...);
        luaL_setmetatable(L, ClassName<T>::value);
        return object;
    }

    template<typename T>
    int Collect(lua_State *L)
    {
        Check<T>(L, 1)->~T();
        return 0;
    }

    // A finalizer can run twice when an object is resurrected, so we clear the metatable after destroying it
    template<typename T>
    int CollectOnce(lua_State *L)
    {
        if (TestClass(L, 1, ClassName<T>::value))
        {
            static_cast<T *>(lua_touserdata(L, 1))->~T();
            lua_pushnil(L);
            lua_setmetatable(L, 1);
        }
        return 0;
    }

    template<typename T>
    void Define(lua_State *L, const char *base, const luaL_Reg *methods, const Field *fields = nullptr)
    {
        DefineClass(L, ClassName<T>::value, base, methods, fields, std::is_trivially_destructible_v<T> ? nullptr : &CollectOnce<T>);
    }

    template<typename T>
    int Construct(lua_State *L)
    {
        New<T>(L);
        return 1;
    }

    // The object a userdata keeps alive goes in its first user value table, keyed by the object itself
    void KeepAlive(lua_State *L, int owner, int value);
    void Release(lua_State *L, int owner, int value);

    // C++ callbacks only have the object's address, so tracked userdata can be found from it through a weak table that doesn't keep
    // them alive. PushTracked returns false once the userdata was collected.
    void Track(lua_State *L, int index);
    bool PushTracked(lua_State *L, const void *object);

    // Pushes the value stored under key in the user value table of the userdata at index, or nil
    int GetUserValue(lua_State *L, int index, const char *key);
    void SetUserValue(lua_State *L, int index, const char *key, int value);

    String ToString(lua_State *L, int index);

    int Push(lua_State *L, bool v);
    int Push(lua_State *L, double v);
    int Push(lua_State *L, const char *v);
    int Push(lua_State *L, const String &v);
    int Push(lua_State *L, const Buffer &v);
    int Push(lua_State *L, qResult v);
    int Push(lua_State *L, const DateTime &v);
    int Push(lua_State *L, const StationURL &v);
    int Push(lua_State *L, const InetAddress &v);
    int Push(lua_State *L, const Variant &v);
    int Push(lua_State *L, const MatchmakeParam &v);
    int Push(lua_State *L, const ResultRange &v);
    int Push(lua_State *L, const RVConnectionData &v);
    int Push(lua_State *L, const AnyObjectHolder<Gathering, String> &v);

    template<typename T, std::enable_if_t<std::is_integral_v<T> || std::is_enum_v<T>, qInt32> = 0>
    int Push(lua_State *L, T v)
    {
        if constexpr (std::is_same_v<T, bool>)
            lua_pushboolean(L, v);
        else
            lua_pushinteger(L, (lua_Integer) v);
        return 1;
    }

    // Data classes are pushed as a copy of the exact type, so every one needs a ClassName and a Define
    template<typename T, std::enable_if_t<std::is_base_of_v<Data, T>, qInt32> = 0>
    int Push(lua_State *L, const T &v)
    {
        New<T>(L, v);
        return 1;
    }

    template<typename C>
    int PushList(lua_State *L, const C &list)
    {
        lua_createtable(L, (qInt32) list.size(), 0);
        lua_Integer i = 1;
        for (const auto &e : list)
        {
            Push(L, e);
            lua_rawseti(L, -2, i++);
        }
        return 1;
    }

    template<typename T>
    int Push(lua_State *L, const qList<T> &v)
    {
        return PushList(L, v);
    }

    template<typename T>
    int Push(lua_State *L, const qVector<T> &v)
    {
        return PushList(L, v);
    }

    template<typename T, typename = void>
    struct Reader
    {
        static T Read(lua_State *L, int index)
        {
            return *Check<T>(L, index);
        }
    };

    template<typename T>
    T Get(lua_State *L, int index)
    {
        return Reader<T>::Read(L, index);
    }

    template<typename T>
    struct Reader<T, std::enable_if_t<std::is_integral_v<T> || std::is_enum_v<T>>>
    {
        static T Read(lua_State *L, int index)
        {
            if constexpr (std::is_same_v<T, bool>)
                return lua_toboolean(L, index) != 0;
            else
                return (T) luaL_checkinteger(L, index);
        }
    };

    template<>
    struct Reader<String>
    {
        static String Read(lua_State *L, int index)
        {
            luaL_checkstring(L, index);
            return ToString(L, index);
        }
    };

    template<>
    struct Reader<Buffer>
    {
        static Buffer Read(lua_State *L, int index)
        {
            size_t size;
            const char *data = luaL_checklstring(L, index, &size);
            return Buffer(data, size);
        }
    };

    template<>
    struct Reader<DateTime>
    {
        static DateTime Read(lua_State *L, int index);
    };

    template<>
    struct Reader<StationURL>
    {
        static StationURL Read(lua_State *L, int index);
    };

    template<>
    struct Reader<MatchmakeParam>
    {
        static MatchmakeParam Read(lua_State *L, int index);
    };

    template<>
    struct Reader<ResultRange>
    {
        static ResultRange Read(lua_State *L, int index);
    };

    template<typename C>
    C ReadList(lua_State *L, int index)
    {
        luaL_checktype(L, index, LUA_TTABLE);
        index = lua_absindex(L, index);
        C list;
        lua_Integer n = luaL_len(L, index);
        for (lua_Integer i = 1; i <= n; i++)
        {
            lua_geti(L, index, i);
            list.push_back(Get<typename C::value_type>(L, -1));
            lua_pop(L, 1);
        }
        return list;
    }

    template<typename T>
    struct Reader<qList<T>>
    {
        static qList<T> Read(lua_State *L, int index)
        {
            return ReadList<qList<T>>(L, index);
        }
    };

    template<typename T>
    struct Reader<qVector<T>>
    {
        static qVector<T> Read(lua_State *L, int index)
        {
            return ReadList<qVector<T>>(L, index);
        }
    };

    // The values an RMC call fills in, kept with its context so they outlive the Lua call that started it
    class Outputs
    {
    public:
        virtual ~Outputs()                   = default;
        virtual int Push(lua_State *L) const = 0;
    };

    template<typename... T>
    class OutputsOf : public Outputs
    {
    public:
        std::tuple<T...> values;

        int Push(lua_State *L) const override
        {
            return std::apply(
                    [L](const auto &...v)
                    {
                        return (0 + ... + lua::Push(L, v));
                    },
                    values);
        }
    };

    // The context is declared last so it is destroyed first, since destroying it cancels a call that may still write the outputs
    struct LuaCall
    {
        std::unique_ptr<Outputs> outputs;
        ProtocolCallContext context;
    };

    template<>
    struct ClassName<LuaCall>
    {
        static constexpr const char *value = "ProtocolCallContext";
    };

    // Pushes a new ProtocolCallContext that keeps the userdata at owner alive until it is collected
    LuaCall *NewCall(lua_State *L, int owner);

    template<typename... T>
    std::tuple<T...> &NewCallWith(lua_State *L, int owner)
    {
        LuaCall *call = NewCall(L, owner);
        auto *out     = new OutputsOf<T...>();
        call->outputs.reset(out);
        return out->values;
    }

    // Leaves the call on the stack and adds whether it was sent, since a call that could not be sent has already failed
    inline int Started(lua_State *L, bool sent)
    {
        lua_pushboolean(L, sent);
        return 2;
    }

    inline ProtocolCallContext *ContextOf(lua_State *L, int index)
    {
        return &static_cast<LuaCall *>(lua_touserdata(L, index))->context;
    }

    void OpenCore(lua_State *L);
    void OpenRuntime(lua_State *L);
    void OpenAuthentication(lua_State *L);
    void OpenMatchMaking(lua_State *L);
    void OpenNATTraversal(lua_State *L);
    void OpenCrypto(lua_State *L);
    void OpenPead(lua_State *L);

    // Each Open function adds its constructors and namespaces to the module table on top of the stack
    void SetFunction(lua_State *L, const char *name, lua_CFunction f);
    void SetFunctions(lua_State *L, const char *table, const luaL_Reg *functions);
    void SetConstants(lua_State *L, const char *table, const std::pair<const char *, lua_Integer> *constants, size_t count);

    // A class table calls ctor when called and holds statics as plain functions, so nex.MD5() and nex.MD5.Hash() both work
    void SetClass(lua_State *L, const char *name, lua_CFunction ctor, const luaL_Reg *statics = nullptr);

    // Sets a metamethod such as __eq on a class that Define already registered
    void SetMetamethod(lua_State *L, const char *className, const char *event, lua_CFunction f);

    // Clone has to push the most derived class, so each Data subclass registers how to push a copy of itself under its type name
    using DataPusher = void (*)(lua_State *L, const Data &data);
    void RegisterData(const char *typeName, DataPusher pusher);
    bool PushData(lua_State *L, const Data &data);

    template<typename T>
    void DefineData(lua_State *L, const char *base, const luaL_Reg *methods, const Field *fields = nullptr)
    {
        Define<T>(L, base ? base : "Data", methods, fields);
        RegisterData(T().GetDataTypeName(),
                     [](lua_State *L, const Data &data)
                     {
                         New<T>(L, static_cast<const T &>(data));
                     });
    }
} // namespace nn::nex::lua

#define NEX_LUA_GET(T, M)                                                                                                                            \
    {#M, [](lua_State *L) -> int                                                                                                                     \
     {                                                                                                                                               \
         return ::nn::nex::lua::Push(L, ::nn::nex::lua::Check<T>(L, 1)->M());                                                                        \
     }}

#define NEX_LUA_SET(T, M, A)                                                                                                                         \
    {#M, [](lua_State *L) -> int                                                                                                                     \
     {                                                                                                                                               \
         T *self = ::nn::nex::lua::Check<T>(L, 1);                                                                                                   \
         self->M(::nn::nex::lua::Get<A>(L, 2));                                                                                                      \
         return 0;                                                                                                                                   \
     }}

#define NEX_LUA_FIELD(T, F)                                                                                                                          \
    {#F,                                                                                                                                             \
     [](lua_State *L) -> int                                                                                                                         \
     {                                                                                                                                               \
         return ::nn::nex::lua::Push(L, ::nn::nex::lua::Check<T>(L, 1)->F);                                                                          \
     },                                                                                                                                              \
     [](lua_State *L) -> int                                                                                                                         \
     {                                                                                                                                               \
         T *self = ::nn::nex::lua::Check<T>(L, 1);                                                                                                   \
         self->F = ::nn::nex::lua::Get<std::remove_cv_t<decltype(T::F)>>(L, 3);                                                                      \
         return 0;                                                                                                                                   \
     }}
