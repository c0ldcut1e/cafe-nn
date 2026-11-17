local relay = nex.NATTraversalRelayClient()
relay:Init()
local probed
relay:SetProbeHandler(function(self, urls) probed = urls end)
relay:SetProbeHandler(nil)
assert(not pcall(relay.SetProbeHandler, relay, 5))
collectgarbage()
