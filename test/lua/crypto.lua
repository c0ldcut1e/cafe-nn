local crypto = nex.crypto

local function hex(s)
    return (s:gsub(".", function(c) return ("%02x"):format(c:byte()) end))
end

local function bytes(h)
    return (h:gsub("..", function(b) return string.char(tonumber(b, 16)) end))
end

local function range(first, count)
    local t = {}
    for i = 0, count - 1 do
        t[#t + 1] = string.char(first + i)
    end
    return table.concat(t)
end

assert(hex(crypto.CalculateSha1("abc")) == "a9993e364706816aba3e25717850c26c9cd0d89d")
assert(hex(crypto.CalculateMd5("abc")) == "900150983cd24fb0d6963f7d28e17f72")

local sha = crypto.Sha1Context()
sha:Update("a")
sha:Update("bc")
assert(hex(sha:GetHash()) == "a9993e364706816aba3e25717850c26c9cd0d89d")
assert(sha:GetHashSize() == 20)
sha:Initialize()
assert(hex(sha:GetHash()) == "da39a3ee5e6b4b0d3255bfef95601890afd80709")

local md5 = crypto.Md5Context()
md5:Update("abc")
assert(md5:GetHashLength() == 3)
assert(hex(md5:GetHash()) == "900150983cd24fb0d6963f7d28e17f72")

local plain = bytes("00112233445566778899aabbccddeeff")
local aes = crypto.Aes128(range(0, 16))
assert(hex(aes:Encrypt(plain)) == "69c4e0d86a7b0430d8cdb78070b4c55a")
assert(aes:Decrypt(aes:Encrypt(plain)) == plain)
assert(aes:GetKeySize() == 16 and aes:GetBlockSize() == 16)
assert(hex(crypto.Aes192(range(0, 24)):Encrypt(plain)) == "dda97ca4864cdfe06eaf70a0ec0d7191")
assert(hex(crypto.Aes256(range(0, 32)):Encrypt(plain)) == "8ea2b7ca516745bfeafc49904b496089")
assert(not aes:SetKey("short"))
assert(not pcall(function() return crypto.Aes128("short") end))
assert(not pcall(aes.Encrypt, aes, "not a block"))

local ctrKey = crypto.Aes128(bytes("2b7e151628aed2a6abf7158809cf4f3c"))
local iv = bytes("f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff")
local message = bytes("6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e51")
local ctr = crypto.CtrEncrypt(ctrKey, iv, message)
assert(hex(ctr) == "874d6191b620e3261bef6864990db6ce9806f66b7970fdff8617187bb9fffdff")
assert(crypto.CtrDecrypt(ctrKey, iv, ctr) == message)
assert(not pcall(crypto.CtrEncrypt, ctrKey, "short", message))
---@diagnostic disable-next-line: param-type-mismatch
assert(not pcall(crypto.CtrEncrypt, "not a cipher", iv, message))

local ccmKey = crypto.Aes128(range(0x40, 16))
local cipher, mac = crypto.CcmEncrypt(ccmKey, range(0x10, 7), range(0x20, 4), range(0, 8), 4)
assert(hex(cipher) == "7162015b" and hex(mac) == "4dac255d")
assert(crypto.CcmDecrypt(ccmKey, range(0x10, 7), cipher, range(0, 8), mac) == range(0x20, 4))
assert(crypto.CcmDecrypt(ccmKey, range(0x10, 7), cipher, range(0, 9), mac) == nil)
assert(not pcall(crypto.CcmEncrypt, ccmKey, "short", "data"))

local a, b = crypto.GenerateRandomBytes(32), crypto.GenerateRandomBytes(32)
assert(#a == 32 and a ~= b)
assert(crypto.GenerateRandomBytes(0) == "")
assert(not pcall(crypto.GenerateRandomBytes, -1))
