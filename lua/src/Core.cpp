#include "Binding.h"

#include <cstring>

namespace nn::nex::lua
{
    NEX_LUA_CLASS(MD5);
    NEX_LUA_CLASS(RC4);
    NEX_LUA_CLASS(KerberosEncryption);

    namespace
    {
        int NewDateTime(lua_State *L)
        {
            if (lua_gettop(L) <= 1)
                New<DateTime>(L, (qUnsignedInt64) luaL_optinteger(L, 1, 0));
            else
                New<DateTime>(L, (qUnsignedInt16) luaL_checkinteger(L, 1), (qUnsignedInt8) luaL_checkinteger(L, 2),
                              (qUnsignedInt8) luaL_checkinteger(L, 3), (qUnsignedInt8) luaL_optinteger(L, 4, 0),
                              (qUnsignedInt8) luaL_optinteger(L, 5, 0), (qUnsignedInt8) luaL_optinteger(L, 6, 0));
            return 1;
        }

        const luaL_Reg DateTimeMethods[] = {
                NEX_LUA_GET(DateTime, GetValue),  NEX_LUA_GET(DateTime, GetYear),
                NEX_LUA_GET(DateTime, GetMonth),  NEX_LUA_GET(DateTime, GetDay),
                NEX_LUA_GET(DateTime, GetHour),   NEX_LUA_GET(DateTime, GetMinute),
                NEX_LUA_GET(DateTime, GetSecond), NEX_LUA_GET(DateTime, IsNever),
                NEX_LUA_GET(DateTime, IsValid),   {nullptr, nullptr},
        };

        int DateTimeEq(lua_State *L)
        {
            lua_pushboolean(L, Check<DateTime>(L, 1)->GetValue() == Check<DateTime>(L, 2)->GetValue());
            return 1;
        }

        int DateTimeLt(lua_State *L)
        {
            lua_pushboolean(L, Check<DateTime>(L, 1)->GetValue() < Check<DateTime>(L, 2)->GetValue());
            return 1;
        }

        int DateTimeLe(lua_State *L)
        {
            lua_pushboolean(L, Check<DateTime>(L, 1)->GetValue() <= Check<DateTime>(L, 2)->GetValue());
            return 1;
        }

        int NewStationURL(lua_State *L)
        {
            if (lua_isnoneornil(L, 1))
                New<StationURL>(L);
            else
                New<StationURL>(L, Get<String>(L, 1));
            return 1;
        }

        int StationURLHasParameter(lua_State *L)
        {
            lua_pushboolean(L, Check<StationURL>(L, 1)->HasParameter(luaL_checkstring(L, 2)));
            return 1;
        }

        int StationURLGetParameter(lua_State *L)
        {
            StationURL *self = Check<StationURL>(L, 1);
            const char *key  = luaL_checkstring(L, 2);
            if (!self->HasParameter(key))
                return 0;
            return Push(L, self->GetParameter(key));
        }

        int StationURLSetParameter(lua_State *L)
        {
            Check<StationURL>(L, 1)->SetParameter(luaL_checkstring(L, 2), Get<String>(L, 3));
            return 0;
        }

        int StationURLParse(lua_State *L)
        {
            lua_pushboolean(L, Check<StationURL>(L, 1)->Parse(Get<String>(L, 2)));
            return 1;
        }

        int StationURLSetInetAddress(lua_State *L)
        {
            InetAddress address = *Check<InetAddress>(L, 2);
            Check<StationURL>(L, 1)->SetInetAddress(&address);
            return 0;
        }

        int StationURLToString(lua_State *L)
        {
            return Push(L, Check<StationURL>(L, 1)->GetURL());
        }

        int StationURLEq(lua_State *L)
        {
            lua_pushboolean(L, Check<StationURL>(L, 1)->GetURL() == Check<StationURL>(L, 2)->GetURL());
            return 1;
        }

        const luaL_Reg StationURLMethods[] = {
                {"Parse", StationURLParse},
                NEX_LUA_GET(StationURL, GetURL),
                NEX_LUA_GET(StationURL, GetURLType),
                NEX_LUA_SET(StationURL, SetURLType, StationURL::URLType),
                NEX_LUA_GET(StationURL, GetAddress),
                NEX_LUA_SET(StationURL, SetAddress, String),
                NEX_LUA_GET(StationURL, GetPortNumber),
                NEX_LUA_SET(StationURL, SetPortNumber, qUnsignedInt16),
                NEX_LUA_GET(StationURL, GetInetAddress),
                {"SetInetAddress", StationURLSetInetAddress},
                NEX_LUA_GET(StationURL, GetPrincipalID),
                NEX_LUA_SET(StationURL, SetPrincipalID, qUnsignedInt32),
                NEX_LUA_GET(StationURL, GetConnectionID),
                NEX_LUA_SET(StationURL, SetConnectionID, qUnsignedInt32),
                NEX_LUA_GET(StationURL, GetRVConnectionID),
                NEX_LUA_SET(StationURL, SetRVConnectionID, qUnsignedInt32),
                NEX_LUA_GET(StationURL, GetStreamID),
                NEX_LUA_SET(StationURL, SetStreamID, qUnsignedInt8),
                NEX_LUA_GET(StationURL, GetStreamType),
                NEX_LUA_SET(StationURL, SetStreamType, Stream::Type),
                NEX_LUA_GET(StationURL, GetNATMapping),
                NEX_LUA_SET(StationURL, SetNATMapping, NATProperties::MappingProperties),
                NEX_LUA_GET(StationURL, GetNATFiltering),
                NEX_LUA_SET(StationURL, SetNATFiltering, NATProperties::FilteringProperties),
                NEX_LUA_GET(StationURL, GetProbeRequestInitiation),
                NEX_LUA_SET(StationURL, SetProbeRequestInitiation, bool),
                NEX_LUA_GET(StationURL, GetType),
                NEX_LUA_SET(StationURL, SetType, qUnsignedInt32),
                NEX_LUA_GET(StationURL, IsBehindNAT),
                NEX_LUA_GET(StationURL, IsPublic),
                NEX_LUA_GET(StationURL, GetUseRelayServer),
                NEX_LUA_SET(StationURL, SetUseRelayServer, bool),
                NEX_LUA_GET(StationURL, GetRelayServerAddress),
                NEX_LUA_SET(StationURL, SetRelayServerAddress, String),
                NEX_LUA_GET(StationURL, GetRelayServerPort),
                NEX_LUA_SET(StationURL, SetRelayServerPort, qUnsignedInt16),
                NEX_LUA_GET(StationURL, GetRelayAddress),
                NEX_LUA_SET(StationURL, SetRelayAddress, String),
                NEX_LUA_GET(StationURL, GetRelayPort),
                NEX_LUA_SET(StationURL, SetRelayPort, qUnsignedInt16),
                {"HasParameter", StationURLHasParameter},
                {"GetParameter", StationURLGetParameter},
                {"SetParameter", StationURLSetParameter},
                {nullptr, nullptr},
        };

        int NewInetAddress(lua_State *L)
        {
            InetAddress *self = New<InetAddress>(L);
            if (lua_type(L, 1) == LUA_TSTRING)
            {
                if (!self->SetAddress(lua_tostring(L, 1)))
                    return luaL_argerror(L, 1, "not an IPv4 address");
            }
            else
                self->SetAddress((qUnsignedInt32) luaL_optinteger(L, 1, 0));
            self->SetPortNumber((qUnsignedInt16) luaL_optinteger(L, 2, 0));
            return 1;
        }

        int InetAddressSetAddress(lua_State *L)
        {
            InetAddress *self = Check<InetAddress>(L, 1);
            if (lua_type(L, 2) == LUA_TSTRING)
                return Push(L, self->SetAddress(lua_tostring(L, 2)));
            self->SetAddress((qUnsignedInt32) luaL_checkinteger(L, 2));
            return 0;
        }

        int InetAddressToString(lua_State *L)
        {
            InetAddress *self   = Check<InetAddress>(L, 1);
            std::string address = self->GetAddressStr().ToUtf8();
            lua_pushfstring(L, "%s:%d", address.c_str(), (qInt32) self->GetPortNumber());
            return 1;
        }

        int InetAddressEq(lua_State *L)
        {
            lua_pushboolean(L, *Check<InetAddress>(L, 1) == *Check<InetAddress>(L, 2));
            return 1;
        }

        const luaL_Reg InetAddressMethods[] = {
                NEX_LUA_GET(InetAddress, GetAddress),
                {"SetAddress", InetAddressSetAddress},
                NEX_LUA_GET(InetAddress, GetAddressStr),
                NEX_LUA_GET(InetAddress, GetPortNumber),
                NEX_LUA_SET(InetAddress, SetPortNumber, qUnsignedInt16),
                {nullptr, nullptr},
        };

        int NewResultRange(lua_State *L)
        {
            New<ResultRange>(L, (qUnsignedInt32) luaL_optinteger(L, 1, 0), (qUnsignedInt32) luaL_optinteger(L, 2, 10));
            return 1;
        }

        const luaL_Reg ResultRangeMethods[] = {
                NEX_LUA_GET(ResultRange, GetOffset),
                NEX_LUA_SET(ResultRange, SetOffset, qUnsignedInt32),
                NEX_LUA_GET(ResultRange, GetSize),
                NEX_LUA_SET(ResultRange, SetSize, qUnsignedInt32),
                {nullptr, nullptr},
        };

        int PushDigest(lua_State *L, const qUnsignedInt8 digest[16])
        {
            lua_pushlstring(L, (const char *) digest, 16);
            return 1;
        }

        int MD5Update(lua_State *L)
        {
            size_t size;
            const char *data = luaL_checklstring(L, 2, &size);
            Check<MD5>(L, 1)->Update(data, size);
            lua_settop(L, 1);
            return 1;
        }

        int MD5Final(lua_State *L)
        {
            qUnsignedInt8 digest[16];
            Check<MD5>(L, 1)->Final(digest);
            return PushDigest(L, digest);
        }

        int MD5Reset(lua_State *L)
        {
            Check<MD5>(L, 1)->Reset();
            return 0;
        }

        int MD5Hash(lua_State *L)
        {
            size_t size;
            const char *data = luaL_checklstring(L, 1, &size);
            qUnsignedInt8 digest[16];
            MD5::Hash(data, size, digest);
            return PushDigest(L, digest);
        }

        const luaL_Reg MD5Methods[] = {
                {"Update", MD5Update},
                {"Final", MD5Final},
                {"Reset", MD5Reset},
                {nullptr, nullptr},
        };

        int HmacMD5Of(lua_State *L)
        {
            size_t keySize, size;
            const char *key  = luaL_checklstring(L, 1, &keySize);
            const char *data = luaL_checklstring(L, 2, &size);
            qUnsignedInt8 digest[16];
            HmacMD5(key, keySize, data, size, digest);
            return PushDigest(L, digest);
        }

        int NewRC4(lua_State *L)
        {
            size_t size;
            const char *key = luaL_checklstring(L, 1, &size);
            New<RC4>(L, key, size);
            return 1;
        }

        int RC4SetKey(lua_State *L)
        {
            size_t size;
            const char *key = luaL_checklstring(L, 2, &size);
            Check<RC4>(L, 1)->SetKey(key, size);
            return 0;
        }

        // Lua strings are immutable, so Process returns the transformed bytes instead of working in place
        int RC4Process(lua_State *L)
        {
            RC4 *self = Check<RC4>(L, 1);
            size_t size;
            const char *data = luaL_checklstring(L, 2, &size);
            luaL_Buffer b;
            char *out = luaL_buffinitsize(L, &b, size);
            memcpy(out, data, size);
            self->Process(out, size);
            luaL_pushresultsize(&b, size);
            return 1;
        }

        const luaL_Reg RC4Methods[] = {
                {"SetKey", RC4SetKey},
                {"Process", RC4Process},
                {nullptr, nullptr},
        };

        int NewKerberosEncryption(lua_State *L)
        {
            New<KerberosEncryption>(L, Get<Buffer>(L, 1));
            return 1;
        }

        int KerberosEncrypt(lua_State *L)
        {
            return Push(L, Check<KerberosEncryption>(L, 1)->Encrypt(Get<Buffer>(L, 2)));
        }

        int KerberosDecrypt(lua_State *L)
        {
            Buffer plain;
            if (!Check<KerberosEncryption>(L, 1)->Decrypt(Get<Buffer>(L, 2), &plain))
                return 0;
            return Push(L, plain);
        }

        int KerberosDeriveKey(lua_State *L)
        {
            return Push(L, KerberosEncryption::DeriveKey((qUnsignedInt32) luaL_checkinteger(L, 1), Get<String>(L, 2)));
        }

        const luaL_Reg KerberosMethods[] = {
                {"Encrypt", KerberosEncrypt},
                {"Decrypt", KerberosDecrypt},
                {nullptr, nullptr},
        };

        int DataGetDataTypeName(lua_State *L)
        {
            lua_pushstring(L, Check<Data>(L, 1)->GetDataTypeName());
            return 1;
        }

        int DataStreamOut(lua_State *L)
        {
            Message m;
            Check<Data>(L, 1)->StreamOut(m);
            lua_pushlstring(L, (const char *) m.GetData(), m.GetSize());
            return 1;
        }

        int DataStreamIn(lua_State *L)
        {
            Data *self = Check<Data>(L, 1);
            size_t size;
            const char *bytes = luaL_checklstring(L, 2, &size);
            Message m(bytes, size);
            lua_pushboolean(L, self->StreamIn(m) && m.IsValid());
            return 1;
        }

        int DataClone(lua_State *L)
        {
            if (!PushData(L, *Check<Data>(L, 1)))
                return luaL_error(L, "%s can't be cloned", Check<Data>(L, 1)->GetDataTypeName());
            return 1;
        }

        const luaL_Reg DataMethods[] = {
                {"GetDataTypeName", DataGetDataTypeName},
                {"StreamOut", DataStreamOut},
                {"StreamIn", DataStreamIn},
                {"Clone", DataClone},
                {nullptr, nullptr},
        };

        int IsSuccess(lua_State *L)
        {
            lua_pushboolean(L, qResult((qUnsignedInt32) luaL_checkinteger(L, 1)).IsSuccess());
            return 1;
        }

        const std::pair<const char *, lua_Integer> QResults[] = {
                {"Success", 0x00010001},
                {"Core_Unknown", QResult::Core_Unknown},
                {"Core_NotImplemented", QResult::Core_NotImplemented},
                {"Core_InvalidPointer", QResult::Core_InvalidPointer},
                {"Core_OperationAborted", QResult::Core_OperationAborted},
                {"Core_Exception", QResult::Core_Exception},
                {"Core_AccessDenied", QResult::Core_AccessDenied},
                {"Core_InvalidHandle", QResult::Core_InvalidHandle},
                {"Core_InvalidIndex", QResult::Core_InvalidIndex},
                {"Core_OutOfMemory", QResult::Core_OutOfMemory},
                {"Core_InvalidArgument", QResult::Core_InvalidArgument},
                {"Core_Timeout", QResult::Core_Timeout},
                {"Core_InitializationFailure", QResult::Core_InitializationFailure},
                {"Core_CallInitiationFailure", QResult::Core_CallInitiationFailure},
                {"Core_RegistrationError", QResult::Core_RegistrationError},
                {"Core_BufferOverflow", QResult::Core_BufferOverflow},
                {"Core_InvalidLockState", QResult::Core_InvalidLockState},
                {"Core_InvalidSequence", QResult::Core_InvalidSequence},
                {"Core_SystemError", QResult::Core_SystemError},
                {"Core_Cancelled", QResult::Core_Cancelled},
                {"RendezVous_ConnectionFailure", QResult::RendezVous_ConnectionFailure},
                {"RendezVous_NotAuthenticated", QResult::RendezVous_NotAuthenticated},
                {"RendezVous_InvalidUsername", QResult::RendezVous_InvalidUsername},
                {"RendezVous_InvalidPassword", QResult::RendezVous_InvalidPassword},
                {"RendezVous_UsernameAlreadyExists", QResult::RendezVous_UsernameAlreadyExists},
                {"RendezVous_AccountDisabled", QResult::RendezVous_AccountDisabled},
                {"RendezVous_AccountExpired", QResult::RendezVous_AccountExpired},
                {"RendezVous_ConcurrentLoginDenied", QResult::RendezVous_ConcurrentLoginDenied},
                {"RendezVous_EncryptionFailure", QResult::RendezVous_EncryptionFailure},
                {"RendezVous_InvalidPID", QResult::RendezVous_InvalidPID},
                {"RendezVous_MaxConnectionsReached", QResult::RendezVous_MaxConnectionsReached},
                {"RendezVous_InvalidGID", QResult::RendezVous_InvalidGID},
                {"RendezVous_InvalidControlScriptID", QResult::RendezVous_InvalidControlScriptID},
                {"RendezVous_InvalidOperationInLiveEnvironment", QResult::RendezVous_InvalidOperationInLiveEnvironment},
                {"RendezVous_DuplicateEntry", QResult::RendezVous_DuplicateEntry},
                {"RendezVous_ControlScriptFailure", QResult::RendezVous_ControlScriptFailure},
                {"RendezVous_ClassNotFound", QResult::RendezVous_ClassNotFound},
                {"RendezVous_SessionVoid", QResult::RendezVous_SessionVoid},
                {"RendezVous_DDLMismatch", QResult::RendezVous_DDLMismatch},
                {"RendezVous_InvalidConfiguration", QResult::RendezVous_InvalidConfiguration},
                {"RendezVous_SessionFull", QResult::RendezVous_SessionFull},
                {"RendezVous_InvalidGatheringPassword", QResult::RendezVous_InvalidGatheringPassword},
                {"RendezVous_WithoutParticipationPeriod", QResult::RendezVous_WithoutParticipationPeriod},
                {"RendezVous_PersistentGatheringCreationMax", QResult::RendezVous_PersistentGatheringCreationMax},
                {"RendezVous_PersistentGatheringParticipationMax", QResult::RendezVous_PersistentGatheringParticipationMax},
                {"RendezVous_DeniedByParticipants", QResult::RendezVous_DeniedByParticipants},
                {"RendezVous_ParticipantInBlackList", QResult::RendezVous_ParticipantInBlackList},
                {"RendezVous_GameServerMaintenance", QResult::RendezVous_GameServerMaintenance},
                {"RendezVous_OperationPostpone", QResult::RendezVous_OperationPostpone},
                {"RendezVous_OutOfRatingRange", QResult::RendezVous_OutOfRatingRange},
                {"RendezVous_ConnectionDisconnected", QResult::RendezVous_ConnectionDisconnected},
                {"RendezVous_InvalidOperation", QResult::RendezVous_InvalidOperation},
                {"RendezVous_NotParticipatedGathering", QResult::RendezVous_NotParticipatedGathering},
                {"RendezVous_MatchmakeSessionUserPasswordUnmatch", QResult::RendezVous_MatchmakeSessionUserPasswordUnmatch},
                {"RendezVous_MatchmakeSessionSystemPasswordUnmatch", QResult::RendezVous_MatchmakeSessionSystemPasswordUnmatch},
                {"RendezVous_UserIsOffline", QResult::RendezVous_UserIsOffline},
                {"RendezVous_AlreadyParticipatedGathering", QResult::RendezVous_AlreadyParticipatedGathering},
                {"RendezVous_PermissionDenied", QResult::RendezVous_PermissionDenied},
                {"RendezVous_NotFriend", QResult::RendezVous_NotFriend},
                {"RendezVous_SessionClosed", QResult::RendezVous_SessionClosed},
                {"RendezVous_DatabaseTemporarilyUnavailable", QResult::RendezVous_DatabaseTemporarilyUnavailable},
                {"RendezVous_InvalidUniqueID", QResult::RendezVous_InvalidUniqueID},
                {"RendezVous_MatchmakingWithdrawn", QResult::RendezVous_MatchmakingWithdrawn},
                {"RendezVous_LimitExceeded", QResult::RendezVous_LimitExceeded},
                {"RendezVous_AccountTemporarilyDisabled", QResult::RendezVous_AccountTemporarilyDisabled},
                {"RendezVous_PartiallyServiceClosed", QResult::RendezVous_PartiallyServiceClosed},
                {"RendezVous_ConnectionDisconnectedForConcurrentLogin", QResult::RendezVous_ConnectionDisconnectedForConcurrentLogin},
                {"Transport_Unknown", QResult::Transport_Unknown},
                {"Transport_ConnectionFailure", QResult::Transport_ConnectionFailure},
                {"Transport_InvalidURL", QResult::Transport_InvalidURL},
                {"Transport_InvalidKey", QResult::Transport_InvalidKey},
                {"Transport_InvalidURLType", QResult::Transport_InvalidURLType},
                {"Transport_DuplicateEndpoint", QResult::Transport_DuplicateEndpoint},
                {"Transport_IOError", QResult::Transport_IOError},
                {"Transport_Timeout", QResult::Transport_Timeout},
                {"Transport_ConnectionReset", QResult::Transport_ConnectionReset},
                {"Transport_IncorrectRemoteAuthentication", QResult::Transport_IncorrectRemoteAuthentication},
                {"Transport_ServerRequestError", QResult::Transport_ServerRequestError},
                {"Transport_DecompressionFailure", QResult::Transport_DecompressionFailure},
                {"Transport_ReliableSendBufferFullFatal", QResult::Transport_ReliableSendBufferFullFatal},
                {"Transport_UPnPCannotInit", QResult::Transport_UPnPCannotInit},
                {"Transport_UPnPCannotAddMapping", QResult::Transport_UPnPCannotAddMapping},
                {"Transport_NatPMPCannotInit", QResult::Transport_NatPMPCannotInit},
                {"Transport_NatPMPCannotAddMapping", QResult::Transport_NatPMPCannotAddMapping},
                {"Transport_UnsupportedNAT", QResult::Transport_UnsupportedNAT},
                {"Transport_DNSError", QResult::Transport_DNSError},
                {"Transport_ProxyError", QResult::Transport_ProxyError},
                {"Transport_DataRemaining", QResult::Transport_DataRemaining},
                {"Transport_NoBuffer", QResult::Transport_NoBuffer},
                {"Transport_NotFound", QResult::Transport_NotFound},
                {"Transport_TemporaryServerError", QResult::Transport_TemporaryServerError},
                {"Transport_PermanentServerError", QResult::Transport_PermanentServerError},
                {"Transport_ServiceUnavailable", QResult::Transport_ServiceUnavailable},
                {"Transport_ReliableSendBufferFull", QResult::Transport_ReliableSendBufferFull},
                {"Transport_InvalidStation", QResult::Transport_InvalidStation},
                {"Transport_InvalidSubStreamID", QResult::Transport_InvalidSubStreamID},
                {"Transport_PacketBufferFull", QResult::Transport_PacketBufferFull},
                {"Transport_NatTraversalError", QResult::Transport_NatTraversalError},
                {"Transport_NatCheckError", QResult::Transport_NatCheckError},
        };

        const std::pair<const char *, lua_Integer> CoreConstants[] = {
                {"UnknownURLType", StationURL::UnknownURLType},
                {"prudp", StationURL::prudp},
                {"prudps", StationURL::prudps},
                {"udp", StationURL::udp},
                {"BehindNAT", StationURL::BehindNAT},
                {"Public", StationURL::Public},
        };

        const std::pair<const char *, lua_Integer> StreamTypes[] = {
                {"DO", Stream::DO},
                {"RV", Stream::RV},
                {"OldRVSec", Stream::OldRVSec},
                {"SBMGMT", Stream::SBMGMT},
                {"NAT", Stream::NAT},
                {"SessionDiscovery", Stream::SessionDiscovery},
                {"NATEcho", Stream::NATEcho},
                {"Routing", Stream::Routing},
                {"Game", Stream::Game},
                {"RVSecure", Stream::RVSecure},
                {"Relay", Stream::Relay},
        };

        const std::pair<const char *, lua_Integer> NATConstants[] = {
                {"UnknownMapping", NATProperties::UnknownMapping}, {"EIMapping", NATProperties::EIMapping},
                {"EDMapping", NATProperties::EDMapping},           {"UnknownFiltering", NATProperties::UnknownFiltering},
                {"PIFiltering", NATProperties::PIFiltering},       {"PDFiltering", NATProperties::PDFiltering},
        };

        template<size_t N>
        void SetConstants(lua_State *L, const char *table, const std::pair<const char *, lua_Integer> (&constants)[N])
        {
            lua::SetConstants(L, table, constants, N);
        }
    } // namespace

    void OpenCore(lua_State *L)
    {
        Define<Data>(L, nullptr, DataMethods);
        Define<DateTime>(L, nullptr, DateTimeMethods);
        SetMetamethod(L, "DateTime", "__eq", DateTimeEq);
        SetMetamethod(L, "DateTime", "__lt", DateTimeLt);
        SetMetamethod(L, "DateTime", "__le", DateTimeLe);
        Define<StationURL>(L, nullptr, StationURLMethods);
        SetMetamethod(L, "StationURL", "__tostring", StationURLToString);
        SetMetamethod(L, "StationURL", "__eq", StationURLEq);
        Define<InetAddress>(L, nullptr, InetAddressMethods);
        SetMetamethod(L, "InetAddress", "__tostring", InetAddressToString);
        SetMetamethod(L, "InetAddress", "__eq", InetAddressEq);
        Define<ResultRange>(L, nullptr, ResultRangeMethods);
        Define<MD5>(L, nullptr, MD5Methods);
        Define<RC4>(L, nullptr, RC4Methods);
        Define<KerberosEncryption>(L, nullptr, KerberosMethods);

        const luaL_Reg md5Statics[]      = {{"Hash", MD5Hash}, {nullptr, nullptr}};
        const luaL_Reg kerberosStatics[] = {{"DeriveKey", KerberosDeriveKey}, {nullptr, nullptr}};
        SetClass(L, "DateTime", NewDateTime);
        SetClass(L, "StationURL", NewStationURL);
        SetClass(L, "InetAddress", NewInetAddress);
        SetClass(L, "ResultRange", NewResultRange);
        SetClass(L, "MD5", Construct<MD5>, md5Statics);
        SetClass(L, "RC4", NewRC4);
        SetClass(L, "KerberosEncryption", NewKerberosEncryption, kerberosStatics);
        SetFunction(L, "HmacMD5", HmacMD5Of);

        const luaL_Reg qresultStatics[] = {{"IsSuccess", IsSuccess}, {nullptr, nullptr}};
        SetConstants(L, "QResult", QResults);
        SetFunctions(L, "QResult", qresultStatics);
        SetConstants(L, "StationURL", CoreConstants);
        SetConstants(L, "Stream", StreamTypes);
        SetConstants(L, "NATProperties", NATConstants);
    }
} // namespace nn::nex::lua
