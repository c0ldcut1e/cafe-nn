#include <cstdio>
#include <lauxlib.h>
#include <lualib.h>
#include <nn/nex/lua.h>

static int Traceback(lua_State *L)
{
    luaL_traceback(L, L, lua_tostring(L, 1), 1);
    return 1;
}

// Runs one Lua test script with the nex module loaded as the global "nex". The script fails the test by raising an error.
int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "usage: %s <script.lua> [args...]\n", argv[0]);
        return 2;
    }

    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    luaL_requiref(L, "nex", luaopen_nex, 1);
    lua_pop(L, 1);

    // Arguments after the script go in the global "arg", like the standalone interpreter does
    lua_createtable(L, argc - 2, 0);
    for (int i = 2; i < argc; i++)
    {
        lua_pushstring(L, argv[i]);
        lua_rawseti(L, -2, i - 1);
    }
    lua_setglobal(L, "arg");

    int status = luaL_loadfile(L, argv[1]);
    if (status == LUA_OK)
    {
        lua_pushcfunction(L, Traceback);
        lua_insert(L, -2);
        status = lua_pcall(L, 0, 0, -2);
    }
    if (status != LUA_OK)
        fprintf(stderr, "%s\n", lua_tostring(L, -1));

    lua_close(L);
    return status == LUA_OK ? 0 : 1;
}
