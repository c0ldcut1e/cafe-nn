local session = nex.MatchmakeSession()
session:SetGameMode(3)
session:SetMaxParticipants(8)
session:SetAttribute(0, 42)
session:SetDescription("lua room")
session:SetMatchmakeParam({ level = 5, ratio = 0.5, ranked = true, name = "x" })
session:SetApplicationBuffer("\1\2\3")
assert(session:GetGameMode() == 3)
assert(session:GetMaxParticipants() == 8)
assert(session:GetAttribute(0) == 42)
assert(session:GetDescription() == "lua room")
assert(session:GetDataTypeName() == "MatchmakeSession")

-- A session survives streaming out and back in, including its param map
local copy = nex.MatchmakeSession()
assert(copy:StreamIn(session:StreamOut()))
assert(copy:GetDescription() == "lua room")
assert(copy:GetApplicationBuffer() == "\1\2\3")
local copyParams = copy:Clone()
assert(copyParams:GetAttribute(0) == 42)

local criteria = nex.MatchmakeSessionSearchCriteria()
criteria:SetAttribute(0, { 1, 2, 3 })
criteria:SetAttributeWithRange(1, 10, 20)
criteria:SetMatchmakeSystemType(nex.MatchmakeSystemType.Anybody)
criteria:SetSelectionMethod(nex.MatchmakeSelectionMethod.Random)
assert(#criteria:StreamOut() > 0)

local create = nex.CreateMatchmakeSessionParam()
create.sourceSession = session
create.joinMessage = "hi"
assert(create.sourceSession:GetGameMode() == 3)
assert(create.joinMessage == "hi")

-- Fields hand out copies, so changing one does nothing until it's assigned back
local nested = create.sourceSession
nested:SetGameMode(9)
assert(create.sourceSession:GetGameMode() == 3)
create.sourceSession = nested
assert(create.sourceSession:GetGameMode() == 9)

local join = nex.JoinMatchmakeSessionParam()
join.gid = 77
assert(join.gid == 77)
---@diagnostic disable-next-line: inject-field
assert(not pcall(function() join.nope = 1 end))

local persistent = nex.PersistentGathering()
persistent:SetPersistentGatheringType(nex.PersistentGatheringType.PasswordLocked)
persistent:SetDescription("community")
assert(persistent:GetPersistentGatheringType() == nex.PersistentGatheringType.PasswordLocked)
assert(persistent:GetDescription() == "community")

local client = nex.MatchmakeExtensionClient()
assert(not pcall(client.UpdateMatchmakeSessionAttribute, client, 1, { 1, 2 }))
assert(nex.MatchmakeExtensionClient.PersistentGatheringCodeToGatheringId ~= nil)
