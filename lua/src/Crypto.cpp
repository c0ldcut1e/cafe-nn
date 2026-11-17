#include "Binding.h"

#include <nn/crypto.h>
#include <string>

namespace nn::nex::lua
{
    // The crypto classes live in nn::crypto, so we name them by hand instead of through NEX_LUA_CLASS
    template<>
    struct ClassName<crypto::Sha1Context>
    {
        static constexpr const char *value = "Sha1Context";
    };
    template<>
    struct ClassName<crypto::Md5Context>
    {
        static constexpr const char *value = "Md5Context";
    };
    template<>
    struct ClassName<crypto::Aes128>
    {
        static constexpr const char *value = "Aes128";
    };
    template<>
    struct ClassName<crypto::Aes192>
    {
        static constexpr const char *value = "Aes192";
    };
    template<>
    struct ClassName<crypto::Aes256>
    {
        static constexpr const char *value = "Aes256";
    };

    namespace
    {
        const qUnsignedInt8 *CheckBytes(lua_State *L, int index, size_t *size)
        {
            return reinterpret_cast<const qUnsignedInt8 *>(luaL_checklstring(L, index, size));
        }

        const qUnsignedInt8 *OptBytes(lua_State *L, int index, size_t *size)
        {
            return reinterpret_cast<const qUnsignedInt8 *>(luaL_optlstring(L, index, "", size));
        }

        int PushBytes(lua_State *L, const void *data, size_t size)
        {
            lua_pushlstring(L, static_cast<const char *>(data), size);
            return 1;
        }

        template<typename T>
        int HashInitialize(lua_State *L)
        {
            Check<T>(L, 1)->Initialize();
            return 0;
        }

        template<typename T>
        int HashUpdate(lua_State *L)
        {
            size_t size;
            const qUnsignedInt8 *data = CheckBytes(L, 2, &size);
            Check<T>(L, 1)->Update(data, size);
            return 0;
        }

        template<typename T>
        int HashFinalize(lua_State *L)
        {
            Check<T>(L, 1)->Finalize();
            return 0;
        }

        template<typename T>
        int HashGetHashSize(lua_State *L)
        {
            return Push(L, (qUnsignedInt32) Check<T>(L, 1)->GetHashSize());
        }

        template<typename T>
        int HashGetHash(lua_State *L)
        {
            qUnsignedInt8 hash[T::HashSize];
            Check<T>(L, 1)->GetHash(hash);
            return PushBytes(L, hash, sizeof hash);
        }

        int Md5GetHashLength(lua_State *L)
        {
            return Push(L, Check<crypto::Md5Context>(L, 1)->GetHashLength());
        }

        const luaL_Reg Sha1Methods[] = {
                {"Initialize", HashInitialize<crypto::Sha1Context>}, {"Update", HashUpdate<crypto::Sha1Context>},
                {"Finalize", HashFinalize<crypto::Sha1Context>},     {"GetHashSize", HashGetHashSize<crypto::Sha1Context>},
                {"GetHash", HashGetHash<crypto::Sha1Context>},       {nullptr, nullptr},
        };

        const luaL_Reg Md5Methods[] = {
                {"Initialize", HashInitialize<crypto::Md5Context>},
                {"Update", HashUpdate<crypto::Md5Context>},
                {"Finalize", HashFinalize<crypto::Md5Context>},
                {"GetHashSize", HashGetHashSize<crypto::Md5Context>},
                {"GetHash", HashGetHash<crypto::Md5Context>},
                {"GetHashLength", Md5GetHashLength},
                {nullptr, nullptr},
        };

        // Any of the three AES classes works wherever a block cipher is expected
        const crypto::BlockCipher *CheckCipher(lua_State *L, int index)
        {
            if (auto *aes = Test<crypto::Aes128>(L, index))
                return aes;
            if (auto *aes = Test<crypto::Aes192>(L, index))
                return aes;
            if (auto *aes = Test<crypto::Aes256>(L, index))
                return aes;
            luaL_typeerror(L, index, "Aes128, Aes192 or Aes256");
            return nullptr;
        }

        template<typename T>
        int AesGetBlockSize(lua_State *L)
        {
            return Push(L, (qUnsignedInt32) Check<T>(L, 1)->GetBlockSize());
        }

        template<typename T>
        int AesGetKeySize(lua_State *L)
        {
            return Push(L, (qUnsignedInt32) Check<T>(L, 1)->GetKeySize());
        }

        template<typename T>
        int AesSetKey(lua_State *L)
        {
            T *self = Check<T>(L, 1);
            size_t size;
            const qUnsignedInt8 *key = CheckBytes(L, 2, &size);
            return Push(L, self->SetKey(key, size));
        }

        // encrypt picks the direction, since both take one block and give one back
        template<typename T, bool encrypt>
        int AesProcess(lua_State *L)
        {
            T *self = Check<T>(L, 1);
            size_t size;
            const qUnsignedInt8 *src = CheckBytes(L, 2, &size);
            luaL_argcheck(L, size == T::BlockSize, 2, "expected one 16 byte block");
            qUnsignedInt8 block[T::BlockSize];
            if (encrypt)
                self->Encrypt(block, src);
            else
                self->Decrypt(block, src);
            return PushBytes(L, block, sizeof block);
        }

        template<typename T>
        const luaL_Reg AesMethods[] = {
                {"GetBlockSize", AesGetBlockSize<T>}, {"GetKeySize", AesGetKeySize<T>},  {"SetKey", AesSetKey<T>},
                {"Encrypt", AesProcess<T, true>},     {"Decrypt", AesProcess<T, false>}, {nullptr, nullptr},
        };

        // The key is optional so a script can create the cipher first and call SetKey later
        template<typename T>
        int NewAes(lua_State *L)
        {
            T *self = New<T>(L);
            if (!lua_isnoneornil(L, 1))
            {
                size_t size;
                const qUnsignedInt8 *key = CheckBytes(L, 1, &size);
                if (!self->SetKey(key, size))
                    return luaL_argerror(L, 1, "wrong key size");
            }
            return 1;
        }

        int CalculateSha1(lua_State *L)
        {
            size_t size;
            const qUnsignedInt8 *data = CheckBytes(L, 1, &size);
            qUnsignedInt8 hash[crypto::Sha1Context::HashSize];
            crypto::CalculateSha1(hash, data, size);
            return PushBytes(L, hash, sizeof hash);
        }

        int CalculateMd5(lua_State *L)
        {
            size_t size;
            const qUnsignedInt8 *data = CheckBytes(L, 1, &size);
            qUnsignedInt8 hash[crypto::Md5Context::HashSize];
            crypto::CalculateMd5(hash, data, size);
            return PushBytes(L, hash, sizeof hash);
        }

        int GenerateRandomBytes(lua_State *L)
        {
            lua_Integer size = luaL_checkinteger(L, 1);
            luaL_argcheck(L, size >= 0, 1, "size can't be negative");
            luaL_Buffer b;
            void *out = luaL_buffinitsize(L, &b, (size_t) size);
            crypto::GenerateRandomBytes(out, (size_t) size);
            luaL_pushresultsize(&b, (size_t) size);
            return 1;
        }

        // The CTR and CCM helpers take the cipher object, so scripts run the same modes the C++ API offers in one call each
        template<typename Mode>
        int Ctr(lua_State *L)
        {
            const crypto::BlockCipher *cipher = CheckCipher(L, 1);
            size_t ivSize, size;
            const qUnsignedInt8 *iv  = CheckBytes(L, 2, &ivSize);
            const qUnsignedInt8 *src = CheckBytes(L, 3, &size);
            std::string out(size, '\0');
            Mode mode;
            if (!mode.Initialize(cipher, iv, ivSize))
                return luaL_argerror(L, 2, "IV must be 16 bytes");
            mode.Update(out.data(), out.size(), src, size);
            return PushBytes(L, out.data(), out.size());
        }

        int CcmEncrypt(lua_State *L)
        {
            const crypto::BlockCipher *cipher = CheckCipher(L, 1);
            size_t nonceSize, size, adataSize;
            const qUnsignedInt8 *nonce = CheckBytes(L, 2, &nonceSize);
            const qUnsignedInt8 *src   = CheckBytes(L, 3, &size);
            const qUnsignedInt8 *adata = OptBytes(L, 4, &adataSize);
            lua_Integer macSize        = luaL_optinteger(L, 5, 16);
            luaL_argcheck(L, macSize >= 4 && macSize <= 16, 5, "MAC size must be 4 to 16");

            crypto::CcmEncryptor ccm;
            if (!ccm.Initialize(cipher, nonce, nonceSize, adataSize, size, (size_t) macSize))
                return luaL_error(L, "CCM rejected the nonce, MAC size or data length");
            std::string out(size, '\0');
            qUnsignedInt8 mac[16];
            ccm.EncryptAndGenerate(out.data(), mac, out.size(), src, size, adata, adataSize, (size_t) macSize);
            PushBytes(L, out.data(), out.size());
            PushBytes(L, mac, (size_t) macSize);
            return 2;
        }

        // A forged or corrupted message gives nil, so nothing unverified ever reaches the script
        int CcmDecrypt(lua_State *L)
        {
            const crypto::BlockCipher *cipher = CheckCipher(L, 1);
            size_t nonceSize, size, adataSize, macSize;
            const qUnsignedInt8 *nonce = CheckBytes(L, 2, &nonceSize);
            const qUnsignedInt8 *src   = CheckBytes(L, 3, &size);
            const qUnsignedInt8 *adata = OptBytes(L, 4, &adataSize);
            const qUnsignedInt8 *mac   = CheckBytes(L, 5, &macSize);

            crypto::CcmDecryptor ccm;
            if (!ccm.Initialize(cipher, nonce, nonceSize, adataSize, size, macSize))
                return luaL_error(L, "CCM rejected the nonce, MAC size or data length");
            std::string out(size, '\0');
            if (!ccm.DecryptAndVerify(out.data(), out.size(), src, size, adata, adataSize, mac, macSize))
            {
                lua_pushnil(L);
                return 1;
            }
            return PushBytes(L, out.data(), out.size());
        }

        const luaL_Reg Functions[] = {
                {"CalculateSha1", CalculateSha1},
                {"CalculateMd5", CalculateMd5},
                {"GenerateRandomBytes", GenerateRandomBytes},
                {"CtrEncrypt", Ctr<crypto::CtrEncryptor<16>>},
                {"CtrDecrypt", Ctr<crypto::CtrDecryptor<16>>},
                {"CcmEncrypt", CcmEncrypt},
                {"CcmDecrypt", CcmDecrypt},
                {nullptr, nullptr},
        };

        template<typename T>
        void DefineAes(lua_State *L)
        {
            Define<T>(L, nullptr, AesMethods<T>);
            SetClass(L, ClassName<T>::value, NewAes<T>);
        }
    } // namespace

    void OpenCrypto(lua_State *L)
    {
        Define<crypto::Sha1Context>(L, nullptr, Sha1Methods);
        Define<crypto::Md5Context>(L, nullptr, Md5Methods);

        lua_newtable(L);
        SetClass(L, "Sha1Context", Construct<crypto::Sha1Context>);
        SetClass(L, "Md5Context", Construct<crypto::Md5Context>);
        DefineAes<crypto::Aes128>(L);
        DefineAes<crypto::Aes192>(L);
        DefineAes<crypto::Aes256>(L);
        luaL_setfuncs(L, Functions, 0);
        lua_setfield(L, -2, "crypto");
    }
} // namespace nn::nex::lua
