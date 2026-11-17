/**
 * @file
 * @brief Lua bindings for the whole library. Scripts get the same class and method names as the C++ API, and RMC calls return a
 * ProtocolCallContext that completes as the scheduler is dispatched.
 */

#pragma once

struct lua_State;

/**
 * @brief Builds the nex module table, so a host can pass this to luaL_requiref as the opener for "nex".
 *
 * @param L The state to open the module in. Completion callbacks, notification handlers and the token provider only run while a script
 * is inside a call such as Scheduler.Dispatch or ProtocolCallContext:Wait. When C++ dispatches the scheduler on its own, they are skipped.
 * @return Always 1, for the module table left on the stack.
 */
int luaopen_nex(lua_State *L);
