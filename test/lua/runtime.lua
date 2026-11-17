local call = nex.ProtocolCallContext()
assert(call:GetState() == nex.CallContext.Available)

local seen = {}
call:RegisterCompletionCallback(function(c) seen[#seen + 1] = c:GetState() end)
assert(call:InitiateCall())
assert(call:IsInProgress())
call:SignalSuccess()
assert(call:GetState() == nex.CallContext.CallSuccess)
assert(#seen == 1 and seen[1] == nex.CallContext.CallSuccess)

assert(call:Reset())
assert(call:GetState() == nex.CallContext.Available)
assert(call:InitiateCall())
call:SignalFailure(nex.QResult.Core_Unknown)
local result, code = call:GetResult()
assert(result == nil and code == nex.QResult.Core_Unknown)
assert(#seen == 1, "Reset drops callbacks registered before it")

-- An error thrown in a callback comes back out of the binding call that ran it
local failing = nex.ProtocolCallContext()
failing:RegisterCompletionCallback(function() error("boom") end)
failing:InitiateCall()
local ok, message = pcall(failing.SignalSuccess, failing)
assert(not ok and tostring(message):find("boom"))

nex.Dispatch(0)
