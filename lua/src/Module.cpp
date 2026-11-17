#include "Binding.h"

#include <nn/nex/lua.h>

int luaopen_nex(lua_State *L)
{
    lua_newtable(L);
    nn::nex::lua::OpenCore(L);
    nn::nex::lua::OpenRuntime(L);
    nn::nex::lua::OpenAuthentication(L);
    nn::nex::lua::OpenMatchMaking(L);
    nn::nex::lua::OpenNATTraversal(L);
#if defined(NN_HAS_CRYPTO)
    nn::nex::lua::OpenCrypto(L);
#endif
#if defined(NN_HAS_PEAD)
    nn::nex::lua::OpenPead(L);
#endif
    return 1;
}
