#include "Binding.h"
#include <nn/nex/matchmaking/AutoMatchmakeParam.h>
#include <nn/nex/matchmaking/CreateMatchmakeSessionParam.h>
#include <nn/nex/matchmaking/Gathering.h>
#include <nn/nex/matchmaking/JoinMatchmakeSessionParam.h>
#include <nn/nex/matchmaking/MatchMakingClient.h>
#include <nn/nex/matchmaking/MatchmakeExtensionClient.h>
#include <nn/nex/matchmaking/MatchmakeSession.h>
#include <nn/nex/matchmaking/MatchmakeSessionSearchCriteria.h>
#include <nn/nex/matchmaking/PersistentGathering.h>
#include <nn/nex/matchmaking/PlayingSession.h>
#include <nn/nex/matchmaking/UpdateMatchmakeSessionParam.h>
#include <nn/nex/rmc/Variant.h>

namespace nn::nex::lua
{
    NEX_LUA_CLASS(Gathering);
    NEX_LUA_CLASS(MatchmakeSession);
    NEX_LUA_CLASS(MatchmakeSessionSearchCriteria);
    NEX_LUA_CLASS(CreateMatchmakeSessionParam);
    NEX_LUA_CLASS(JoinMatchmakeSessionParam);
    NEX_LUA_CLASS(AutoMatchmakeParam);
    NEX_LUA_CLASS(UpdateMatchmakeSessionParam);
    NEX_LUA_CLASS(PersistentGathering);
    NEX_LUA_CLASS(PlayingSession);
    NEX_LUA_CLASS(MatchMakingClient);
    NEX_LUA_CLASS(MatchmakeExtensionClient);

    int Push(lua_State *L, const Variant &v)
    {
        switch (v.GetType())
        {
            case Variant::Int64:
                lua_pushinteger(L, (lua_Integer) v.GetInt64Value());
                return 1;
            case Variant::UInt64:
                lua_pushinteger(L, (lua_Integer) v.GetUInt64Value());
                return 1;
            case Variant::Double:
                lua_pushnumber(L, v.GetDoubleValue());
                return 1;
            case Variant::Bool:
                lua_pushboolean(L, v.GetBoolValue());
                return 1;
            case Variant::StringType:
                return Push(L, v.GetStringValue());
            case Variant::DateTimeType:
                return Push(L, v.GetDateTimeValue());
            default:
                lua_pushnil(L);
                return 1;
        }
    }

    // MatchmakeParam is a plain table in Lua, keyed by parameter name
    int Push(lua_State *L, const MatchmakeParam &v)
    {
        lua_createtable(L, 0, (qInt32) v.params.size());
        for (const auto &[key, value] : v.params)
        {
            Push(L, value);
            lua_setfield(L, -2, key.ToUtf8().c_str());
        }
        return 1;
    }

    int Push(lua_State *L, const AnyObjectHolder<Gathering, String> &v)
    {
        if (!v.Get() || !PushData(L, *v.Get()))
            lua_pushnil(L);
        return 1;
    }

    namespace
    {
        // Lua integers become signed Variants, since a Lua script has no way to say which of the two integer types it meant
        Variant ReadVariant(lua_State *L, int index)
        {
            switch (lua_type(L, index))
            {
                case LUA_TBOOLEAN:
                    return Variant(lua_toboolean(L, index) != 0);
                case LUA_TNUMBER:
                    if (lua_isinteger(L, index))
                        return Variant((qInt64) lua_tointeger(L, index));
                    return Variant((double) lua_tonumber(L, index));
                case LUA_TSTRING:
                    return Variant(ToString(L, index));
                default:
                    return Variant(Get<DateTime>(L, index));
            }
        }
    } // namespace

    MatchmakeParam Reader<MatchmakeParam>::Read(lua_State *L, int index)
    {
        MatchmakeParam param;
        if (lua_isnoneornil(L, index))
            return param;
        luaL_checktype(L, index, LUA_TTABLE);
        index = lua_absindex(L, index);
        lua_pushnil(L);
        while (lua_next(L, index))
        {
            if (lua_type(L, -2) != LUA_TSTRING)
                luaL_error(L, "MatchmakeParam keys have to be strings");
            param.params[ToString(L, -2)] = ReadVariant(L, -1);
            lua_pop(L, 1);
        }
        return param;
    }

    namespace
    {
        qVector<qByte> ReadBytes(lua_State *L, int index)
        {
            size_t size;
            const char *data = luaL_checklstring(L, index, &size);
            return qVector<qByte>(data, data + size);
        }

        const luaL_Reg GatheringMethods[] = {
                NEX_LUA_GET(Gathering, GetID),
                NEX_LUA_GET(Gathering, GetOwnerPID),
                NEX_LUA_GET(Gathering, GetHostPID),
                NEX_LUA_GET(Gathering, GetMinParticipants),
                NEX_LUA_GET(Gathering, GetMaxParticipants),
                NEX_LUA_GET(Gathering, GetParticipationPolicy),
                NEX_LUA_GET(Gathering, GetPolicyArgument),
                NEX_LUA_GET(Gathering, GetFlags),
                NEX_LUA_GET(Gathering, GetState),
                NEX_LUA_GET(Gathering, GetDescription),
                NEX_LUA_SET(Gathering, SetID, qUnsignedInt32),
                NEX_LUA_SET(Gathering, SetMinParticipants, qUnsignedInt16),
                NEX_LUA_SET(Gathering, SetMaxParticipants, qUnsignedInt16),
                NEX_LUA_SET(Gathering, SetParticipationPolicy, qUnsignedInt32),
                NEX_LUA_SET(Gathering, SetPolicyArgument, qUnsignedInt32),
                NEX_LUA_SET(Gathering, SetFlags, qUnsignedInt32),
                NEX_LUA_SET(Gathering, SetDescription, String),
                {nullptr, nullptr},
        };

        int SessionGetAttribute(lua_State *L)
        {
            return Push(L, Check<MatchmakeSession>(L, 1)->GetAttribute(Get<qUnsignedInt32>(L, 2)));
        }

        int SessionSetAttribute(lua_State *L)
        {
            Check<MatchmakeSession>(L, 1)->SetAttribute(Get<qUnsignedInt32>(L, 2), Get<qUnsignedInt32>(L, 3));
            return 0;
        }

        int SessionSetMatchmakeSystemType(lua_State *L)
        {
            Check<MatchmakeSession>(L, 1)->SetMatchmakeSystemType(Get<MatchmakeSystemType>(L, 2), (qUnsignedInt32) luaL_optinteger(L, 3, 0));
            return 0;
        }

        int SessionSetApplicationBuffer(lua_State *L)
        {
            Check<MatchmakeSession>(L, 1)->SetApplicationBuffer(ReadBytes(L, 2));
            return 0;
        }

        int SessionReset(lua_State *L)
        {
            Check<MatchmakeSession>(L, 1)->Reset();
            return 0;
        }

        const luaL_Reg MatchmakeSessionMethods[] = {
                {"Reset", SessionReset},
                {"GetAttribute", SessionGetAttribute},
                {"SetAttribute", SessionSetAttribute},
                {"SetMatchmakeSystemType", SessionSetMatchmakeSystemType},
                {"SetApplicationBuffer", SessionSetApplicationBuffer},
                NEX_LUA_SET(MatchmakeSession, SetGameMode, qUnsignedInt32),
                NEX_LUA_SET(MatchmakeSession, SetOpenParticipation, bool),
                NEX_LUA_SET(MatchmakeSession, SetProgressScore, qByte),
                NEX_LUA_SET(MatchmakeSession, SetUserPassword, String),
                NEX_LUA_SET(MatchmakeSession, SetMatchmakeParam, MatchmakeParam),
                NEX_LUA_GET(MatchmakeSession, GetGameMode),
                NEX_LUA_GET(MatchmakeSession, GetOpenParticipation),
                NEX_LUA_GET(MatchmakeSession, GetMatchmakeSystemType),
                NEX_LUA_GET(MatchmakeSession, GetApplicationBuffer),
                NEX_LUA_GET(MatchmakeSession, GetParticipationCount),
                NEX_LUA_GET(MatchmakeSession, GetProgressScore),
                NEX_LUA_GET(MatchmakeSession, GetSessionKey),
                NEX_LUA_GET(MatchmakeSession, GetAttributes),
                NEX_LUA_GET(MatchmakeSession, GetStartedTime),
                {nullptr, nullptr},
        };

        int CriteriaReset(lua_State *L)
        {
            Check<MatchmakeSessionSearchCriteria>(L, 1)->Reset();
            return 0;
        }

        // A number matches one value and a table matches any value in it
        int CriteriaSetAttribute(lua_State *L)
        {
            auto *self = Check<MatchmakeSessionSearchCriteria>(L, 1);
            auto index = Get<qUnsignedInt32>(L, 2);
            if (lua_istable(L, 3))
                self->SetAttribute(index, Get<qVector<qUnsignedInt32>>(L, 3));
            else
                self->SetAttribute(index, qVector<qUnsignedInt32>{Get<qUnsignedInt32>(L, 3)});
            return 0;
        }

        int CriteriaSetAttributeWithRange(lua_State *L)
        {
            Check<MatchmakeSessionSearchCriteria>(L, 1)->SetAttributeWithRange(Get<qUnsignedInt32>(L, 2), Get<qUnsignedInt32>(L, 3),
                                                                               Get<qUnsignedInt32>(L, 4));
            return 0;
        }

        int CriteriaSetMinParticipantsWithRange(lua_State *L)
        {
            Check<MatchmakeSessionSearchCriteria>(L, 1)->SetMinParticipantsWithRange(Get<qUnsignedInt16>(L, 2), Get<qUnsignedInt16>(L, 3));
            return 0;
        }

        int CriteriaSetMaxParticipantsWithRange(lua_State *L)
        {
            Check<MatchmakeSessionSearchCriteria>(L, 1)->SetMaxParticipantsWithRange(Get<qUnsignedInt16>(L, 2), Get<qUnsignedInt16>(L, 3));
            return 0;
        }

        int CriteriaSetSelectionMethod(lua_State *L)
        {
            Check<MatchmakeSessionSearchCriteria>(L, 1)->SetSelectionMethod(Get<MatchmakeSelectionMethod>(L, 2), Get<MatchmakeParam>(L, 3));
            return 0;
        }

        const luaL_Reg SearchCriteriaMethods[] = {
                {"Reset", CriteriaReset},
                {"SetAttribute", CriteriaSetAttribute},
                {"SetAttributeWithRange", CriteriaSetAttributeWithRange},
                {"SetMinParticipantsWithRange", CriteriaSetMinParticipantsWithRange},
                {"SetMaxParticipantsWithRange", CriteriaSetMaxParticipantsWithRange},
                {"SetSelectionMethod", CriteriaSetSelectionMethod},
                NEX_LUA_SET(MatchmakeSessionSearchCriteria, SetGameMode, qUnsignedInt32),
                NEX_LUA_SET(MatchmakeSessionSearchCriteria, SetMinParticipants, qUnsignedInt16),
                NEX_LUA_SET(MatchmakeSessionSearchCriteria, SetMaxParticipants, qUnsignedInt16),
                NEX_LUA_SET(MatchmakeSessionSearchCriteria, SetMatchmakeSystemType, MatchmakeSystemType),
                NEX_LUA_SET(MatchmakeSessionSearchCriteria, SetVacantOnly, bool),
                NEX_LUA_SET(MatchmakeSessionSearchCriteria, SetExcludeLocked, bool),
                NEX_LUA_SET(MatchmakeSessionSearchCriteria, SetExcludeNonHostPid, bool),
                NEX_LUA_SET(MatchmakeSessionSearchCriteria, SetExcludeUserPasswordSet, bool),
                {nullptr, nullptr},
        };

        const luaL_Reg NoMethods[] = {{nullptr, nullptr}};

        // Struct fields are read and written by copy, so changing a session on its own does nothing until it is assigned back
        const Field CreateParamFields[] = {
                NEX_LUA_FIELD(CreateMatchmakeSessionParam, sourceSession),
                NEX_LUA_FIELD(CreateMatchmakeSessionParam, additionalParticipants),
                NEX_LUA_FIELD(CreateMatchmakeSessionParam, gidForParticipationCheck),
                NEX_LUA_FIELD(CreateMatchmakeSessionParam, option),
                NEX_LUA_FIELD(CreateMatchmakeSessionParam, joinMessage),
                NEX_LUA_FIELD(CreateMatchmakeSessionParam, participationCount),
                {nullptr, nullptr, nullptr},
        };

        int JoinParamReset(lua_State *L)
        {
            Check<JoinMatchmakeSessionParam>(L, 1)->Reset();
            return 0;
        }

        const luaL_Reg JoinParamMethods[] = {
                {"Reset", JoinParamReset},
                {nullptr, nullptr},
        };

        const Field JoinParamFields[] = {
                NEX_LUA_FIELD(JoinMatchmakeSessionParam, gid),
                NEX_LUA_FIELD(JoinMatchmakeSessionParam, additionalParticipants),
                NEX_LUA_FIELD(JoinMatchmakeSessionParam, gidForParticipationCheck),
                NEX_LUA_FIELD(JoinMatchmakeSessionParam, option),
                NEX_LUA_FIELD(JoinMatchmakeSessionParam, behavior),
                NEX_LUA_FIELD(JoinMatchmakeSessionParam, userPassword),
                NEX_LUA_FIELD(JoinMatchmakeSessionParam, systemPassword),
                NEX_LUA_FIELD(JoinMatchmakeSessionParam, joinMessage),
                NEX_LUA_FIELD(JoinMatchmakeSessionParam, participationCount),
                {nullptr, nullptr, nullptr},
        };

        const Field AutoParamFields[] = {
                NEX_LUA_FIELD(AutoMatchmakeParam, sourceSession),
                NEX_LUA_FIELD(AutoMatchmakeParam, additionalParticipants),
                NEX_LUA_FIELD(AutoMatchmakeParam, gidForParticipationCheck),
                NEX_LUA_FIELD(AutoMatchmakeParam, option),
                NEX_LUA_FIELD(AutoMatchmakeParam, joinMessage),
                NEX_LUA_FIELD(AutoMatchmakeParam, participationCount),
                NEX_LUA_FIELD(AutoMatchmakeParam, searchCriteria),
                NEX_LUA_FIELD(AutoMatchmakeParam, targetGids),
                {nullptr, nullptr, nullptr},
        };

        const Field UpdateParamFields[] = {
                NEX_LUA_FIELD(UpdateMatchmakeSessionParam, gid),
                NEX_LUA_FIELD(UpdateMatchmakeSessionParam, modificationFlag),
                NEX_LUA_FIELD(UpdateMatchmakeSessionParam, attributes),
                NEX_LUA_FIELD(UpdateMatchmakeSessionParam, openParticipation),
                NEX_LUA_FIELD(UpdateMatchmakeSessionParam, applicationBuffer),
                NEX_LUA_FIELD(UpdateMatchmakeSessionParam, progressScore),
                NEX_LUA_FIELD(UpdateMatchmakeSessionParam, matchmakeParam),
                NEX_LUA_FIELD(UpdateMatchmakeSessionParam, startedTime),
                NEX_LUA_FIELD(UpdateMatchmakeSessionParam, userPassword),
                NEX_LUA_FIELD(UpdateMatchmakeSessionParam, gameMode),
                NEX_LUA_FIELD(UpdateMatchmakeSessionParam, description),
                NEX_LUA_FIELD(UpdateMatchmakeSessionParam, minParticipants),
                NEX_LUA_FIELD(UpdateMatchmakeSessionParam, maxParticipants),
                NEX_LUA_FIELD(UpdateMatchmakeSessionParam, matchmakeSystemType),
                NEX_LUA_FIELD(UpdateMatchmakeSessionParam, participationPolicy),
                NEX_LUA_FIELD(UpdateMatchmakeSessionParam, policyArgument),
                NEX_LUA_FIELD(UpdateMatchmakeSessionParam, codeword),
                {nullptr, nullptr, nullptr},
        };

        int PersistentGetAttribute(lua_State *L)
        {
            return Push(L, Check<PersistentGathering>(L, 1)->GetAttribute(Get<qUnsignedInt32>(L, 2)));
        }

        int PersistentSetAttribute(lua_State *L)
        {
            Check<PersistentGathering>(L, 1)->SetAttribute(Get<qUnsignedInt32>(L, 2), Get<qUnsignedInt32>(L, 3));
            return 0;
        }

        int PersistentSetApplicationBuffer(lua_State *L)
        {
            Check<PersistentGathering>(L, 1)->SetApplicationBuffer(ReadBytes(L, 2));
            return 0;
        }

        int PersistentSetParticipationPeriod(lua_State *L)
        {
            Check<PersistentGathering>(L, 1)->SetParticipationPeriod(Get<DateTime>(L, 2), Get<DateTime>(L, 3));
            return 0;
        }

        int PersistentReset(lua_State *L)
        {
            Check<PersistentGathering>(L, 1)->Reset();
            return 0;
        }

        const luaL_Reg PersistentGatheringMethods[] = {
                {"Reset", PersistentReset},
                {"GetAttribute", PersistentGetAttribute},
                {"SetAttribute", PersistentSetAttribute},
                {"SetApplicationBuffer", PersistentSetApplicationBuffer},
                {"SetParticipationPeriod", PersistentSetParticipationPeriod},
                NEX_LUA_GET(PersistentGathering, GetPersistentGatheringType),
                NEX_LUA_SET(PersistentGathering, SetPersistentGatheringType, PersistentGatheringType::PersistentGatheringType),
                NEX_LUA_SET(PersistentGathering, SetPassword, String),
                NEX_LUA_GET(PersistentGathering, GetApplicationBuffer),
                NEX_LUA_GET(PersistentGathering, GetMatchmakeSessionCount),
                NEX_LUA_GET(PersistentGathering, GetParticipationCount),
                {nullptr, nullptr},
        };

        int PlayingSessionGetMatchmakeSession(lua_State *L)
        {
            const MatchmakeSession *session = Check<PlayingSession>(L, 1)->GetMatchmakeSession();
            if (!session)
            {
                lua_pushnil(L);
                return 1;
            }
            return Push(L, *session);
        }

        const luaL_Reg PlayingSessionMethods[] = {
                NEX_LUA_GET(PlayingSession, GetPrincipalID),
                {"GetMatchmakeSession", PlayingSessionGetMatchmakeSession},
                {nullptr, nullptr},
        };

        template<typename T>
        T *Self(lua_State *L)
        {
            return Check<T>(L, 1);
        }

        // Calls that only take a gid and return nothing but their result
        template<typename T, bool (T::*Method)(ProtocolCallContext *, qUnsignedInt32)>
        int GidCall(lua_State *L)
        {
            T *self  = Self<T>(L);
            auto gid = Get<qUnsignedInt32>(L, 2);
            NewCall(L, 1);
            return Started(L, (self->*Method)(ContextOf(L, -1), gid));
        }

        using MM  = MatchMakingClient;
        using MME = MatchmakeExtensionClient;

        int MMFindByID(lua_State *L)
        {
            MM *self  = Self<MM>(L);
            auto gid  = Get<qUnsignedInt32>(L, 2);
            auto &out = NewCallWith<AnyObjectHolder<Gathering, String>>(L, 1);
            return Started(L, self->FindByID(ContextOf(L, -1), gid, &std::get<0>(out)));
        }

        int MMFindByOwner(lua_State *L)
        {
            MM *self   = Self<MM>(L);
            auto pid   = Get<qUnsignedInt32>(L, 2);
            auto range = Get<ResultRange>(L, 3);
            auto &out  = NewCallWith<qList<AnyObjectHolder<Gathering, String>>>(L, 1);
            return Started(L, self->FindByOwner(ContextOf(L, -1), pid, range, &std::get<0>(out)));
        }

        int MMGetSessionURLs(lua_State *L)
        {
            MM *self  = Self<MM>(L);
            auto gid  = Get<qUnsignedInt32>(L, 2);
            auto &out = NewCallWith<qList<StationURL>>(L, 1);
            return Started(L, self->GetSessionURLs(ContextOf(L, -1), gid, &std::get<0>(out)));
        }

        int MMUpdateSessionHost(lua_State *L)
        {
            MM *self     = Self<MM>(L);
            auto gid     = Get<qUnsignedInt32>(L, 2);
            bool migrate = lua_toboolean(L, 3) != 0;
            NewCall(L, 1);
            return Started(L, self->UpdateSessionHost(ContextOf(L, -1), gid, migrate));
        }

        int MMMigrateGatheringOwnership(lua_State *L)
        {
            MM *self        = Self<MM>(L);
            auto gid        = Get<qUnsignedInt32>(L, 2);
            auto candidates = Get<qList<qUnsignedInt32>>(L, 3);
            NewCall(L, 1);
            return Started(L, self->MigrateGatheringOwnership(ContextOf(L, -1), gid, candidates));
        }

        int MMEndParticipation(lua_State *L)
        {
            MM *self       = Self<MM>(L);
            auto gid       = Get<qUnsignedInt32>(L, 2);
            String message = lua_isnoneornil(L, 3) ? String() : Get<String>(L, 3);
            NewCall(L, 1);
            return Started(L, self->EndParticipation(ContextOf(L, -1), gid, message));
        }

        const luaL_Reg MatchMakingMethods[] = {
                {"UnregisterGathering", GidCall<MM, &MM::UnregisterGathering>},
                {"FindByID", MMFindByID},
                {"FindByOwner", MMFindByOwner},
                {"GetSessionURLs", MMGetSessionURLs},
                {"UpdateSessionHost", MMUpdateSessionHost},
                {"MigrateGatheringOwnership", MMMigrateGatheringOwnership},
                {"EndParticipation", MMEndParticipation},
                {nullptr, nullptr},
        };

        int MMEBrowseMatchmakeSession(lua_State *L)
        {
            MME *self      = Self<MME>(L);
            auto *criteria = Check<MatchmakeSessionSearchCriteria>(L, 2);
            auto range     = Get<ResultRange>(L, 3);
            auto &out      = NewCallWith<qList<AnyObjectHolder<Gathering, String>>>(L, 1);
            return Started(L, self->BrowseMatchmakeSession(ContextOf(L, -1), *criteria, range, &std::get<0>(out)));
        }

        int MMEModifyMatchmakeSessionAttribute(lua_State *L)
        {
            MME *self  = Self<MME>(L);
            auto gid   = Get<qUnsignedInt32>(L, 2);
            auto index = Get<qUnsignedInt32>(L, 3);
            auto value = Get<qUnsignedInt32>(L, 4);
            NewCall(L, 1);
            return Started(L, self->ModifyMatchmakeSessionAttribute(ContextOf(L, -1), gid, index, value));
        }

        int MMEUpdateNotificationData(lua_State *L)
        {
            MME *self   = Self<MME>(L);
            auto type   = Get<qUnsignedInt32>(L, 2);
            auto param1 = Get<qUnsignedInt32>(L, 3);
            auto param2 = Get<qUnsignedInt32>(L, 4);
            auto str    = lua_isnoneornil(L, 5) ? String() : Get<String>(L, 5);
            NewCall(L, 1);
            return Started(L, self->UpdateNotificationData(ContextOf(L, -1), type, param1, param2, str));
        }

        int MMEUpdateApplicationBuffer(lua_State *L)
        {
            MME *self   = Self<MME>(L);
            auto gid    = Get<qUnsignedInt32>(L, 2);
            auto buffer = ReadBytes(L, 3);
            NewCall(L, 1);
            return Started(L, self->UpdateApplicationBuffer(ContextOf(L, -1), gid, buffer));
        }

        // The server keeps a fixed array of six attributes per session, so we take exactly six
        int MMEUpdateMatchmakeSessionAttribute(lua_State *L)
        {
            MME *self  = Self<MME>(L);
            auto gid   = Get<qUnsignedInt32>(L, 2);
            auto attrs = Get<qVector<qUnsignedInt32>>(L, 3);
            luaL_argcheck(L, attrs.size() == 6, 3, "expected 6 attributes");
            NewCall(L, 1);
            return Started(L, self->UpdateMatchmakeSessionAttribute(ContextOf(L, -1), gid, attrs.data()));
        }

        int MMEGetFriendNotificationData(lua_State *L)
        {
            MME *self  = Self<MME>(L);
            auto types = Get<qList<qUnsignedInt32>>(L, 2);
            auto &out  = NewCallWith<qList<NotificationEvent>>(L, 1);
            return Started(L, self->GetFriendNotificationData(ContextOf(L, -1), &std::get<0>(out), types));
        }

        int MMEGetPlayingSession(lua_State *L)
        {
            MME *self = Self<MME>(L);
            auto pids = Get<qList<qUnsignedInt32>>(L, 2);
            auto &out = NewCallWith<qList<PlayingSession>>(L, 1);
            return Started(L, self->GetPlayingSession(ContextOf(L, -1), pids, &std::get<0>(out)));
        }

        int MMECreatePersistentGathering(lua_State *L)
        {
            MME *self       = Self<MME>(L);
            auto *gathering = Check<PersistentGathering>(L, 2);
            String message  = lua_isnoneornil(L, 3) ? String() : Get<String>(L, 3);
            auto &out       = NewCallWith<qUnsignedInt32>(L, 1);
            return Started(L, self->CreatePersistentGathering(ContextOf(L, -1), *gathering, &std::get<0>(out), message));
        }

        int MMEUpdatePersistentGathering(lua_State *L)
        {
            MME *self       = Self<MME>(L);
            auto *gathering = Check<PersistentGathering>(L, 2);
            NewCall(L, 1);
            return Started(L, self->UpdatePersistentGathering(ContextOf(L, -1), *gathering));
        }

        int MMEJoinPersistentGathering(lua_State *L)
        {
            MME *self       = Self<MME>(L);
            auto gid        = Get<qUnsignedInt32>(L, 2);
            String message  = lua_isnoneornil(L, 3) ? String() : Get<String>(L, 3);
            String password = lua_isnoneornil(L, 4) ? String() : Get<String>(L, 4);
            NewCall(L, 1);
            return Started(L, self->JoinPersistentGathering(ContextOf(L, -1), gid, message, password));
        }

        int MMEFindPersistentGatheringByGatheringId(lua_State *L)
        {
            MME *self = Self<MME>(L);
            auto gids = Get<qList<qUnsignedInt32>>(L, 2);
            auto &out = NewCallWith<qList<PersistentGathering>>(L, 1);
            return Started(L, self->FindPersistentGatheringByGatheringId(ContextOf(L, -1), gids, &std::get<0>(out)));
        }

        int MMEFindOfficialPersistentGathering(lua_State *L)
        {
            MME *self      = Self<MME>(L);
            bool available = lua_toboolean(L, 2) != 0;
            auto range     = Get<ResultRange>(L, 3);
            auto &out      = NewCallWith<qList<PersistentGathering>>(L, 1);
            return Started(L, self->FindOfficialPersistentGathering(ContextOf(L, -1), available, range, &std::get<0>(out)));
        }

        int MMEFindPersistentGatheringByParticipant(lua_State *L)
        {
            MME *self  = Self<MME>(L);
            auto pid   = Get<qUnsignedInt32>(L, 2);
            auto range = Get<ResultRange>(L, 3);
            auto &out  = NewCallWith<qList<PersistentGathering>>(L, 1);
            return Started(L, self->FindPersistentGatheringByParticipant(ContextOf(L, -1), pid, range, &std::get<0>(out)));
        }

        // This one has no context of its own because the client sends it and drops the result
        int MMEUpdateProgressScore(lua_State *L)
        {
            MME *self = Self<MME>(L);
            return Push(L, self->UpdateProgressScore(Get<qUnsignedInt32>(L, 2), Get<qByte>(L, 3)));
        }

        int MMEGenerateMatchmakeSessionSystemPassword(lua_State *L)
        {
            MME *self = Self<MME>(L);
            auto gid  = Get<qUnsignedInt32>(L, 2);
            auto &out = NewCallWith<String>(L, 1);
            return Started(L, self->GenerateMatchmakeSessionSystemPassword(ContextOf(L, -1), gid, &std::get<0>(out)));
        }

        // Session calls share this shape, so the param type and method are template arguments
        template<typename P, bool (MME::*Method)(ProtocolCallContext *, const P &, MatchmakeSession *)>
        int SessionCall(lua_State *L)
        {
            MME *self = Self<MME>(L);
            P *param  = Check<P>(L, 2);
            auto &out = NewCallWith<MatchmakeSession>(L, 1);
            return Started(L, (self->*Method)(ContextOf(L, -1), *param, &std::get<0>(out)));
        }

        int MMEFindMatchmakeSessionByGatheringIdDetail(lua_State *L)
        {
            MME *self = Self<MME>(L);
            auto gid  = Get<qUnsignedInt32>(L, 2);
            auto &out = NewCallWith<MatchmakeSession>(L, 1);
            return Started(L, self->FindMatchmakeSessionByGatheringIdDetail(ContextOf(L, -1), gid, &std::get<0>(out)));
        }

        int MMEUpdateMatchmakeSession(lua_State *L)
        {
            MME *self   = Self<MME>(L);
            auto *param = Check<UpdateMatchmakeSessionParam>(L, 2);
            NewCall(L, 1);
            return Started(L, self->UpdateMatchmakeSession(ContextOf(L, -1), *param));
        }

        const luaL_Reg MatchmakeExtensionMethods[] = {
                {"CloseParticipation", GidCall<MME, &MME::CloseParticipation>},
                {"OpenParticipation", GidCall<MME, &MME::OpenParticipation>},
                {"BrowseMatchmakeSession", MMEBrowseMatchmakeSession},
                {"ModifyMatchmakeSessionAttribute", MMEModifyMatchmakeSessionAttribute},
                {"UpdateNotificationData", MMEUpdateNotificationData},
                {"UpdateApplicationBuffer", MMEUpdateApplicationBuffer},
                {"UpdateMatchmakeSessionAttribute", MMEUpdateMatchmakeSessionAttribute},
                {"GetFriendNotificationData", MMEGetFriendNotificationData},
                {"GetPlayingSession", MMEGetPlayingSession},
                {"CreatePersistentGathering", MMECreatePersistentGathering},
                {"UpdatePersistentGathering", MMEUpdatePersistentGathering},
                {"JoinPersistentGathering", MMEJoinPersistentGathering},
                {"FindPersistentGatheringByGatheringId", MMEFindPersistentGatheringByGatheringId},
                {"FindOfficialPersistentGathering", MMEFindOfficialPersistentGathering},
                {"FindPersistentGatheringByParticipant", MMEFindPersistentGatheringByParticipant},
                {"UpdateProgressScore", MMEUpdateProgressScore},
                {"GenerateMatchmakeSessionSystemPassword", MMEGenerateMatchmakeSessionSystemPassword},
                {"ClearMatchmakeSessionSystemPassword", GidCall<MME, &MME::ClearMatchmakeSessionSystemPassword>},
                {"CreateMatchmakeSession", SessionCall<CreateMatchmakeSessionParam, &MME::CreateMatchmakeSession>},
                {"JoinMatchmakeSession", SessionCall<JoinMatchmakeSessionParam, &MME::JoinMatchmakeSession>},
                {"AutoMatchmake", SessionCall<AutoMatchmakeParam, &MME::AutoMatchmake>},
                {"FindMatchmakeSessionByGatheringIdDetail", MMEFindMatchmakeSessionByGatheringIdDetail},
                {"UpdateMatchmakeSession", MMEUpdateMatchmakeSession},
                {nullptr, nullptr},
        };

        int SetPersistentGatheringCodeKey(lua_State *L)
        {
            MME::SetPersistentGatheringCodeKey(Get<String>(L, 1));
            return 0;
        }

        int ClearPersistentGatheringCodeKey(lua_State *)
        {
            MME::ClearPersistentGatheringCodeKey();
            return 0;
        }

        int GatheringIdToPersistentGatheringCode(lua_State *L)
        {
            return Push(L, (lua_Integer) MME::GatheringIdToPersistentGatheringCode(Get<qUnsignedInt32>(L, 1)));
        }

        int PersistentGatheringCodeToGatheringId(lua_State *L)
        {
            return Push(L, MME::PersistentGatheringCodeToGatheringId((qUnsignedInt64) luaL_checkinteger(L, 1)));
        }

        const std::pair<const char *, lua_Integer> SystemTypes[] = {
                {"Invalid", MatchmakeSystemTypeInvalid},
                {"Anybody", MatchmakeSystemTypeAnybody},
                {"Friend", MatchmakeSystemTypeFriend},
                {"FriendInvite", MatchmakeSystemTypeFriendInvite},
                {"PersistentGathering", MatchmakeSystemTypePersistentGathering},
        };

        const std::pair<const char *, lua_Integer> SelectionMethods[] = {
                {"Random", MatchmakeSelectionMethodRandom},
                {"NearestNeighbor", MatchmakeSelectionMethodNearestNeighbor},
                {"BroadenRange", MatchmakeSelectionMethodBroadenRange},
                {"ProgressScore", MatchmakeSelectionMethodProgressScore},
                {"BroadenRangeWithProgressScore", MatchmakeSelectionMethodBroadenRangeWithProgressScore},
                {"ScoreBased", MatchmakeSelectionMethodScoreBased},
        };

        const std::pair<const char *, lua_Integer> PersistentGatheringTypes[] = {
                {"Open", PersistentGatheringType::Open},
                {"PasswordLocked", PersistentGatheringType::PasswordLocked},
                {"Official", PersistentGatheringType::Official},
        };
    } // namespace

    void OpenMatchMaking(lua_State *L)
    {
        DefineData<Gathering>(L, nullptr, GatheringMethods);
        DefineData<MatchmakeSession>(L, "Gathering", MatchmakeSessionMethods);
        DefineData<PersistentGathering>(L, "Gathering", PersistentGatheringMethods);
        DefineData<MatchmakeSessionSearchCriteria>(L, nullptr, SearchCriteriaMethods);
        DefineData<CreateMatchmakeSessionParam>(L, nullptr, NoMethods, CreateParamFields);
        DefineData<JoinMatchmakeSessionParam>(L, nullptr, JoinParamMethods, JoinParamFields);
        DefineData<AutoMatchmakeParam>(L, nullptr, NoMethods, AutoParamFields);
        DefineData<UpdateMatchmakeSessionParam>(L, nullptr, NoMethods, UpdateParamFields);
        DefineData<PlayingSession>(L, nullptr, PlayingSessionMethods);
        Define<MatchMakingClient>(L, "ClientProtocol", MatchMakingMethods);
        Define<MatchmakeExtensionClient>(L, "ClientProtocol", MatchmakeExtensionMethods);

        SetClass(L, "Gathering", Construct<Gathering>);
        SetClass(L, "MatchmakeSession", Construct<MatchmakeSession>);
        SetClass(L, "PersistentGathering", Construct<PersistentGathering>);
        SetClass(L, "MatchmakeSessionSearchCriteria", Construct<MatchmakeSessionSearchCriteria>);
        SetClass(L, "CreateMatchmakeSessionParam", Construct<CreateMatchmakeSessionParam>);
        SetClass(L, "JoinMatchmakeSessionParam", Construct<JoinMatchmakeSessionParam>);
        SetClass(L, "AutoMatchmakeParam", Construct<AutoMatchmakeParam>);
        SetClass(L, "UpdateMatchmakeSessionParam", Construct<UpdateMatchmakeSessionParam>);
        SetClass(L, "PlayingSession", Construct<PlayingSession>);
        SetClass(L, "MatchMakingClient", Construct<MatchMakingClient>);

        const luaL_Reg extensionStatics[] = {
                {"SetPersistentGatheringCodeKey", SetPersistentGatheringCodeKey},
                {"ClearPersistentGatheringCodeKey", ClearPersistentGatheringCodeKey},
                {"GatheringIdToPersistentGatheringCode", GatheringIdToPersistentGatheringCode},
                {"PersistentGatheringCodeToGatheringId", PersistentGatheringCodeToGatheringId},
                {nullptr, nullptr},
        };
        SetClass(L, "MatchmakeExtensionClient", Construct<MatchmakeExtensionClient>, extensionStatics);
        SetConstants(L, "MatchmakeSystemType", SystemTypes, std::size(SystemTypes));
        SetConstants(L, "MatchmakeSelectionMethod", SelectionMethods, std::size(SelectionMethods));
        SetConstants(L, "PersistentGatheringType", PersistentGatheringTypes, std::size(PersistentGatheringTypes));
    }
} // namespace nn::nex::lua
