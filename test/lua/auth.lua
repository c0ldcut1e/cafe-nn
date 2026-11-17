local facade = nex.NgsFacade()
assert(not facade:IsConnected())

local info = nex.AuthenticationInfo()
info.token = "abc"
assert(info.token == "abc")

local event = nex.NotificationEvent()
event:SetType(nex.NotificationEvents.GatheringUnregistered)
assert(event:GetType() == nex.NotificationEvents.GatheringUnregistered)

local calls = 0
local handler = nex.NotificationEventHandler(function() calls = calls + 1 end)
facade:RegisterNotificationEventHandler(handler)

-- Without a server the provider is the first thing login reaches, and its error code is the login result
nex.NgsFacade.SetTokenProvider(function() return nil, nex.QResult.Core_AccessDenied end)
local call = facade:Login(0x101, "00000000")
assert(call:GetState() == nex.CallContext.CallFailure)
assert(call:GetOutcome() == nex.QResult.Core_AccessDenied)
nex.NgsFacade.SetTokenProvider(nil)

local endpoint = nex.EndPoint()
assert(not endpoint:IsConnected())

-- A finished login frees its job on the next dispatch
nex.Dispatch(0)
