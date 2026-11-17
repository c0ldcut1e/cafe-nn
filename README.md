# cafe-nn

cafe-nn is a reimplementation of the network middleware Nintendo shipped to Wii U developers: NEX, Pia, nn::crypto and pead. Games link these statically into their executables, so they were never system libraries that homebrew could call. This project rebuilds them as normal static libraries.

Class and function names match Nintendo's (`nn::nex::NgsFacade`, `nn::nex::MatchmakeExtensionClient`, `nn::pia::common::Scheduler` and so on). Behaviour is based on the code inside retail games. If you've seen a game's NEX code in a disassembler, the API should look familiar.

It builds for the Wii U through devkitPro/wut, and natively for Linux and Windows. A PC program can log in to a NEX server the same way the console does, and everything can be tested without a console. All of it is also usable from Lua.

## Libraries

| Library | Namespace | Status |
| --- | --- | --- |
| `nex` | `nn::nex` | Usable. PRUDP transport, RMC, Kerberos authentication, secure connection, matchmaking, NAT traversal |
| `pia` | `nn::pia` | In progress. Only `common` so far (results, module stack checker, time, addresses, jobs, schedulers) |
| `crypto` | `nn::crypto` | Usable. SHA-1, MD5, AES-128/192/256, CTR and CCM modes, random bytes |
| `pead` | `pead` | Usable. Heaps and the heap manager, strings, UTF-8/UTF-16/Shift JIS conversion, streams, threads, message queues, delegates, Base64, Random |

`pia` depends on `nex`, `pead` and `crypto`. The other three don't depend on anything, so `crypto` or `pead` can be used on their own.

## Building

Clone with `--recursive` to get the Lua submodule, or run `git submodule update --init` afterwards. You need CMake 3.25 or newer and Ninja. For the Wii U you also need devkitPro with `wiiu-dev` installed and `DEVKITPRO` set.

```sh
cmake --preset linux
cmake --build --preset linux
```

| Preset | Builds on | Output |
| --- | --- | --- |
| `linux` | Linux | Linux, debug |
| `linux-asan` | Linux | Linux, debug with AddressSanitizer and UBSan |
| `linux-release` | Linux | Linux, release |
| `windows-msvc` | Windows | Windows, built with MSVC |
| `windows-mingw` | Linux | Windows, cross-compiled with MinGW-w64 |
| `wiiu` | Anywhere devkitPro runs | Wii U |

Output goes to `build/<preset>`. Each library is its own static library (`libnex.a`, `libpia.a`, `libcrypto.a`, `libpead.a`), and the Lua bindings are `lua/libnex_lua.a`.

### Using it in your project

With CMake, add it as a subdirectory and link the libraries you need:

```cmake
set(NEX_BUILD_TESTS OFF)
add_subdirectory(cafe-nn)
target_link_libraries(my_app PRIVATE nex::nex)
```

The targets are `nex::nex`, `nex::pia`, `nex::crypto`, `nex::pead` and `nex::lua`. Set `NEX_BUILD_LUA` to `OFF` if you don't need Lua. You can also run `cmake --install build/<preset>` and use the installed headers and libraries directly.

### Tests

Desktop builds run the test suite as part of the build, so a failing test fails the build. To run the tests again without rebuilding:

```sh
ctest --preset linux
```

`windows-mingw` runs the Windows test binaries through wine, since they can't run on Linux directly. `wiiu` compiles everything but doesn't run the tests.

## Usage

Logging in to a game server:

```cpp
#include <nn/nex.h>

using namespace nn::nex;

NgsFacade facade;
ProtocolCallContext context;

if (facade.Login(&context, gameServerId, L"accesskey") && context.Wait(30000) && context.GetState() == CallContext::CallSuccess)
    printf("logged in as %u\n", facade.GetPrincipalID());
```

All protocol calls work like this. They take a call context and return straight away. The call finishes later, when `Scheduler::Dispatch` runs or when you `Wait` on the context.

### Tokens

Login needs a NEX token from the account server. On the Wii U, `NgsFacade` gets it from `nn_act` like a game does, so there's nothing to set up.

Linux and Windows have no account service, so you provide the token yourself:

```cpp
static qResult ProvideToken(qUnsignedInt32 gameServerId, NexServiceToken *out)
{
    out->token       = ...;
    out->nexPassword = ...;
    out->principalId = ...;
    out->host        = ...;
    out->port        = ...;
    return qResult(true);
}

NgsFacade::SetTokenProvider(ProvideToken);
```

cafe-nn doesn't fetch tokens on PC.

## Lua

The Lua bindings use the same class and method names as the C++ API. `luaopen_nex` opens everything as one module, with crypto and pead under `nex.crypto` and `nex.pead`:

```cpp
#include <nn/nex/lua.h>

luaL_requiref(L, "nex", luaopen_nex, 1);
lua_pop(L, 1);
```

The same login in Lua:

```lua
nex.NgsFacade.SetTokenProvider(function(gameServerId)
    return { token = ..., nexPassword = ..., principalId = ..., host = ..., port = ... }
end)

local facade = nex.NgsFacade()
local call = facade:Login(gameServerId, "accesskey")

if call:Wait(30000) and call:GetState() == nex.CallContext.CallSuccess then
    print("logged in as " .. facade:GetPrincipalID())
end
```

Callbacks (completion callbacks, notification handlers, the token provider) only run while the script is inside a call like `nex.Dispatch` or `call:Wait`. If your C++ code dispatches the scheduler itself, Lua callbacks are skipped.

Each build writes [LuaLS](https://luals.github.io/) definition files to `lua/meta`, one per library: `nex.lua`, `crypto.lua` and `pead.lua` (a library you leave out of the build gets no file). The included `.luarc.json` points the language server at that folder, so editors know the `nex` global and complete every class, method and constant.

## Contributing

Run `make format` before committing (clang-format with the repo's `.clang-format`), and make sure the tests pass on at least one desktop preset.

## Third party

[Lua 5.4](https://www.lua.org/) is a submodule in `third_party/lua`, under the MIT license.
