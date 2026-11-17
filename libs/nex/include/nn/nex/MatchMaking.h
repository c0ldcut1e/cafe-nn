/**
 * @file
 * @brief These are the matchmaking data classes and the MatchMaking and MatchmakeExtension protocol clients. They run over the secure connection after login and are how players find, create and join sessions.
 */

#pragma once

#include <map>
#include <nn/nex/Authentication.h>

namespace nn::nex
{

    /**
     * @brief This says who may join a session.
     */
    enum MatchmakeSystemType
    {
        MatchmakeSystemTypeInvalid             = 0,
        MatchmakeSystemTypeAnybody             = 1,
        MatchmakeSystemTypeFriend              = 2,
        MatchmakeSystemTypeFriendInvite        = 3,
        MatchmakeSystemTypePersistentGathering = 6
    };

    /**
     * @brief This says how AutoMatchmake ranks the sessions that match.
     */
    enum MatchmakeSelectionMethod
    {
        MatchmakeSelectionMethodRandom                        = 0,
        MatchmakeSelectionMethodNearestNeighbor               = 1,
        MatchmakeSelectionMethodBroadenRange                  = 2,
        MatchmakeSelectionMethodProgressScore                 = 3,
        MatchmakeSelectionMethodBroadenRangeWithProgressScore = 4,
        MatchmakeSelectionMethodScoreBased                    = 5
    };

    namespace PersistentGatheringType
    {
        enum PersistentGatheringType
        {
            Open           = 0,
            PasswordLocked = 1,
            Official       = 2
        };
    } // namespace PersistentGatheringType

    /**
     * @brief This is a free-form map of named values that newer matchmaking calls send alongside sessions.
     */
    class MatchmakeParam : public Data
    {
    public:
        std::map<String, Variant> params;

        const char *GetDataTypeName() const override
        {
            return "MatchmakeParam";
        }
        Data *Clone() const override
        {
            return new MatchmakeParam(*this);
        }
        void StreamOut(Message &m) const override;
        bool StreamIn(Message &m) override;
    };

    /**
     * @brief This is the base of every group of players the server tracks. Its ID, often called the gid, is what most matchmaking calls take.
     */
    class Gathering : public Data
    {
    public:
        Gathering()           = default;
        ~Gathering() override = default;

        qUnsignedInt32 GetID() const
        {
            return m_id;
        }
        qUnsignedInt32 GetOwnerPID() const
        {
            return m_ownerPid;
        }
        qUnsignedInt32 GetHostPID() const
        {
            return m_hostPid;
        }
        qUnsignedInt16 GetMinParticipants() const
        {
            return m_minParticipants;
        }
        qUnsignedInt16 GetMaxParticipants() const
        {
            return m_maxParticipants;
        }
        qUnsignedInt32 GetParticipationPolicy() const
        {
            return m_participationPolicy;
        }
        qUnsignedInt32 GetPolicyArgument() const
        {
            return m_policyArgument;
        }
        qUnsignedInt32 GetFlags() const
        {
            return m_flags;
        }
        qUnsignedInt32 GetState() const
        {
            return m_state;
        }
        const String &GetDescription() const
        {
            return m_description;
        }

        void SetID(qUnsignedInt32 v)
        {
            m_id = v;
        }
        void SetMinParticipants(qUnsignedInt16 v)
        {
            m_minParticipants = v;
        }
        void SetMaxParticipants(qUnsignedInt16 v)
        {
            m_maxParticipants = v;
        }
        void SetParticipationPolicy(qUnsignedInt32 v)
        {
            m_participationPolicy = v;
        }
        void SetPolicyArgument(qUnsignedInt32 v)
        {
            m_policyArgument = v;
        }
        void SetFlags(qUnsignedInt32 v)
        {
            m_flags = v;
        }
        void SetDescription(const String &v)
        {
            m_description = v;
        }

        const char *GetDataTypeName() const override
        {
            return "Gathering";
        }
        Data *Clone() const override
        {
            return new Gathering(*this);
        }
        void StreamOut(Message &m) const override;
        bool StreamIn(Message &m) override;

    protected:
        qUnsignedInt32 m_id                  = 0;
        qUnsignedInt32 m_ownerPid            = 0;
        qUnsignedInt32 m_hostPid             = 0;
        qUnsignedInt16 m_minParticipants     = 0;
        qUnsignedInt16 m_maxParticipants     = 0;
        qUnsignedInt32 m_participationPolicy = 0;
        qUnsignedInt32 m_policyArgument      = 0;
        qUnsignedInt32 m_flags               = 0;
        qUnsignedInt32 m_state               = 0;
        String m_description;
    };

    /**
     * @brief This is a joinable game session with the attributes and game mode that searches match on.
     */
    class MatchmakeSession : public Gathering
    {
    public:
        MatchmakeSession()
        {
            Reset();
        }

        void Reset();
        qUnsignedInt32 GetAttribute(qUnsignedInt32 index) const
        {
            return index < m_attributes.size() ? m_attributes[index] : 0;
        }
        void SetAttribute(qUnsignedInt32 index, qUnsignedInt32 value);
        void SetGameMode(qUnsignedInt32 v)
        {
            m_gameMode = v;
        }
        void SetMatchmakeSystemType(MatchmakeSystemType type, qUnsignedInt32 argument = 0);
        void SetOpenParticipation(bool v)
        {
            m_openParticipation = v;
        }
        void SetApplicationBuffer(const qVector<qByte> &v)
        {
            m_applicationBuffer = Buffer(v.data(), v.size());
        }
        void SetProgressScore(qByte v)
        {
            m_progressScore = v;
        }
        void SetUserPassword(const String &v)
        {
            m_userPassword = v;
        }
        void SetMatchmakeParam(const MatchmakeParam &v)
        {
            m_matchmakeParam = v;
        }

        qUnsignedInt32 GetGameMode() const
        {
            return m_gameMode;
        }
        bool GetOpenParticipation() const
        {
            return m_openParticipation;
        }
        qUnsignedInt32 GetMatchmakeSystemType() const
        {
            return m_matchmakeSystemType;
        }
        const Buffer &GetApplicationBuffer() const
        {
            return m_applicationBuffer;
        }
        qUnsignedInt32 GetParticipationCount() const
        {
            return m_participationCount;
        }
        qByte GetProgressScore() const
        {
            return m_progressScore;
        }
        const Buffer &GetSessionKey() const
        {
            return m_sessionKey;
        }
        const qVector<qUnsignedInt32> &GetAttributes() const
        {
            return m_attributes;
        }
        const DateTime &GetStartedTime() const
        {
            return m_startedTime;
        }

        const char *GetDataTypeName() const override
        {
            return "MatchmakeSession";
        }
        Data *Clone() const override
        {
            return new MatchmakeSession(*this);
        }
        void StreamOut(Message &m) const override;
        bool StreamIn(Message &m) override;

    private:
        qUnsignedInt32 m_gameMode;
        qVector<qUnsignedInt32> m_attributes;
        bool m_openParticipation;
        qUnsignedInt32 m_matchmakeSystemType;
        Buffer m_applicationBuffer;
        qUnsignedInt32 m_participationCount;
        qByte m_progressScore;
        Buffer m_sessionKey;
        qUnsignedInt32 m_option0;
        MatchmakeParam m_matchmakeParam;
        DateTime m_startedTime;
        String m_userPassword;
        qUnsignedInt32 m_referGid;
        bool m_userPasswordEnabled;
        bool m_systemPasswordEnabled;
    };

    /**
     * @brief This is a search filter for BrowseMatchmakeSession and AutoMatchmake. Each attribute is a value or a range written as a string, and an empty string matches anything.
     */
    class MatchmakeSessionSearchCriteria : public Data
    {
    public:
        MatchmakeSessionSearchCriteria()
        {
            Reset();
        }

        void Reset();
        void SetAttribute(qUnsignedInt32 index, const qVector<qUnsignedInt32> &values);
        void SetAttributeWithRange(qUnsignedInt32 index, qUnsignedInt32 min, qUnsignedInt32 max);
        void SetGameMode(qUnsignedInt32 v);
        void SetMinParticipants(qUnsignedInt16 v);
        void SetMinParticipantsWithRange(qUnsignedInt16 min, qUnsignedInt16 max);
        void SetMaxParticipants(qUnsignedInt16 v);
        void SetMaxParticipantsWithRange(qUnsignedInt16 min, qUnsignedInt16 max);
        void SetMatchmakeSystemType(MatchmakeSystemType v);
        void SetVacantOnly(bool v)
        {
            m_vacantOnly = v;
        }
        void SetExcludeLocked(bool v)
        {
            m_excludeLocked = v;
        }
        void SetExcludeNonHostPid(bool v)
        {
            m_excludeNonHostPid = v;
        }
        void SetExcludeUserPasswordSet(bool v)
        {
            m_excludeUserPasswordSet = v;
        }
        void SetSelectionMethod(MatchmakeSelectionMethod method, const MatchmakeParam &param);

        const char *GetDataTypeName() const override
        {
            return "MatchmakeSessionSearchCriteria";
        }
        Data *Clone() const override
        {
            return new MatchmakeSessionSearchCriteria(*this);
        }
        void StreamOut(Message &m) const override;

    private:
        qVector<String> m_attribs;
        String m_gameMode;
        String m_minParticipants;
        String m_maxParticipants;
        String m_matchmakeSystemType;
        bool m_vacantOnly;
        bool m_excludeLocked;
        bool m_excludeNonHostPid;
        qUnsignedInt32 m_selectionMethod;
        qUnsignedInt16 m_vacantParticipants;
        MatchmakeParam m_matchmakeParam;
        bool m_excludeUserPasswordSet;
        bool m_excludeSystemPasswordSet;
        qUnsignedInt32 m_referGid;
    };

    /**
     * @brief This is what CreateMatchmakeSession takes, a session plus the options that only matter at creation.
     */
    class CreateMatchmakeSessionParam : public Data
    {
    public:
        MatchmakeSession sourceSession;
        qVector<qUnsignedInt32> additionalParticipants;
        qUnsignedInt32 gidForParticipationCheck = 0;
        qUnsignedInt32 option                   = 0;
        String joinMessage;
        qUnsignedInt16 participationCount = 1;

        const char *GetDataTypeName() const override
        {
            return "CreateMatchmakeSessionParam";
        }
        Data *Clone() const override
        {
            return new CreateMatchmakeSessionParam(*this);
        }
        void StreamOut(Message &m) const override;
    };

    /**
     * @brief This is what JoinMatchmakeSession takes, a gid plus the join message and password.
     */
    class JoinMatchmakeSessionParam : public Data
    {
    public:
        JoinMatchmakeSessionParam()
        {
            Reset();
        }
        void Reset();

        qUnsignedInt32 gid;
        qVector<qUnsignedInt32> additionalParticipants;
        qUnsignedInt32 gidForParticipationCheck;
        qUnsignedInt32 option;
        qByte behavior;
        String userPassword;
        String systemPassword;
        String joinMessage;
        qUnsignedInt16 participationCount;

        const char *GetDataTypeName() const override
        {
            return "JoinMatchmakeSessionParam";
        }
        Data *Clone() const override
        {
            return new JoinMatchmakeSessionParam(*this);
        }
        void StreamOut(Message &m) const override;
    };

    /**
     * @brief This is what AutoMatchmake takes. The server joins the best match among the criteria, or creates the session given when nothing matches.
     */
    class AutoMatchmakeParam : public Data
    {
    public:
        MatchmakeSession sourceSession;
        qVector<qUnsignedInt32> additionalParticipants;
        qUnsignedInt32 gidForParticipationCheck = 0;
        qUnsignedInt32 option                   = 0;
        String joinMessage;
        qUnsignedInt16 participationCount = 1;
        qVector<MatchmakeSessionSearchCriteria> searchCriteria;
        qVector<qUnsignedInt32> targetGids;

        const char *GetDataTypeName() const override
        {
            return "AutoMatchmakeParam";
        }
        Data *Clone() const override
        {
            return new AutoMatchmakeParam(*this);
        }
        void StreamOut(Message &m) const override;
    };

    /**
     * @brief This is what UpdateMatchmakeSession takes, and its modification flags say which session fields change.
     */
    class UpdateMatchmakeSessionParam : public Data
    {
    public:
        qUnsignedInt32 gid              = 0;
        qUnsignedInt32 modificationFlag = 0;
        qVector<qUnsignedInt32> attributes;
        bool openParticipation = false;
        Buffer applicationBuffer;
        qByte progressScore = 0;
        MatchmakeParam matchmakeParam;
        DateTime startedTime;
        String userPassword;
        qUnsignedInt32 gameMode = 0;
        String description;
        qUnsignedInt16 minParticipants     = 0;
        qUnsignedInt16 maxParticipants     = 0;
        qUnsignedInt32 matchmakeSystemType = 0;
        qUnsignedInt32 participationPolicy = 0;
        qUnsignedInt32 policyArgument      = 0;
        String codeword;

        const char *GetDataTypeName() const override
        {
            return "UpdateMatchmakeSessionParam";
        }
        Data *Clone() const override
        {
            return new UpdateMatchmakeSessionParam(*this);
        }
        void StreamOut(Message &m) const override;
    };

    /**
     * @brief This is a gathering that outlives its sessions, such as a community. Sessions are created inside it.
     */
    class PersistentGathering : public Gathering
    {
    public:
        PersistentGathering()
        {
            Reset();
        }

        void Reset();
        qUnsignedInt32 GetAttribute(qUnsignedInt32 index) const
        {
            return index < m_attribs.size() ? m_attribs[index] : 0;
        }
        void SetAttribute(qUnsignedInt32 index, qUnsignedInt32 value);
        PersistentGatheringType::PersistentGatheringType GetPersistentGatheringType() const
        {
            return (PersistentGatheringType::PersistentGatheringType) m_communityType;
        }
        void SetPersistentGatheringType(PersistentGatheringType::PersistentGatheringType v)
        {
            m_communityType = v;
        }
        void SetPassword(const String &v)
        {
            m_password = v;
        }
        void SetApplicationBuffer(const qVector<qByte> &v)
        {
            m_applicationBuffer = Buffer(v.data(), v.size());
        }
        void SetParticipationPeriod(const DateTime &start, const DateTime &end)
        {
            m_startDate = start;
            m_endDate   = end;
        }

        const Buffer &GetApplicationBuffer() const
        {
            return m_applicationBuffer;
        }
        qUnsignedInt32 GetMatchmakeSessionCount() const
        {
            return m_matchmakeSessionCount;
        }
        qUnsignedInt32 GetParticipationCount() const
        {
            return m_participationCount;
        }

        const char *GetDataTypeName() const override
        {
            return "PersistentGathering";
        }
        Data *Clone() const override
        {
            return new PersistentGathering(*this);
        }
        void StreamOut(Message &m) const override;
        bool StreamIn(Message &m) override;

    private:
        qUnsignedInt32 m_communityType;
        String m_password;
        qVector<qUnsignedInt32> m_attribs;
        Buffer m_applicationBuffer;
        DateTime m_startDate;
        DateTime m_endDate;
        qUnsignedInt32 m_matchmakeSessionCount;
        qUnsignedInt32 m_participationCount;
    };

    /**
     * @brief This pairs a player with the gathering they are in, as GetPlayingSession returns it.
     */
    class PlayingSession : public Data
    {
    public:
        qUnsignedInt32 GetPrincipalID() const
        {
            return m_principalId;
        }
        const MatchmakeSession *GetMatchmakeSession() const;

        const char *GetDataTypeName() const override
        {
            return "PlayingSession";
        }
        Data *Clone() const override
        {
            return new PlayingSession(*this);
        }
        bool StreamIn(Message &m) override;

    private:
        qUnsignedInt32 m_principalId = 0;
        AnyObjectHolder<Gathering, String> m_gathering;
    };

    /**
     * @brief This is the client for the original matchmaking protocol. It also carries EndParticipation, which the server keeps under protocol 50.
     */
    class MatchMakingClient : public ClientProtocol
    {
    public:
        MatchMakingClient() : ClientProtocol(21), m_ext(50)
        {}

        void Bind(EndPoint *endPoint) override
        {
            ClientProtocol::Bind(endPoint);
            m_ext.Bind(endPoint);
        }

        /**
         * @brief Deletes a gathering we own. Protocol 21 method 2.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param gid The gathering ID.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool UnregisterGathering(ProtocolCallContext *context, qUnsignedInt32 gid);

        /**
         * @brief Protocol 21 method 21.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param gid The gathering ID.
         * @param gathering Receives the gathering, whatever its class. Filled in when context completes, so it has to stay alive until then.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool FindByID(ProtocolCallContext *context, qUnsignedInt32 gid, AnyObjectHolder<Gathering, String> *gathering);

        /**
         * @brief Returns the gatherings owned by ownerPid. Protocol 21 method 22.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param ownerPid The owner's PID.
         * @param range The paging window.
         * @param gatherings Receives the gatherings found. Filled in when context completes, so it has to stay alive until then.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool FindByOwner(ProtocolCallContext *context, qUnsignedInt32 ownerPid, const ResultRange &range,
                         qList<AnyObjectHolder<Gathering, String>> *gatherings);

        /**
         * @brief Returns the host's station URLs for a session, which is what a joining player connects to. Protocol 21 method 41.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param gid The gathering ID.
         * @param urls Receives the host's station URLs. Filled in when context completes, so it has to stay alive until then.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool GetSessionURLs(ProtocolCallContext *context, qUnsignedInt32 gid, qList<StationURL> *urls);

        /**
         * @brief Makes us the session host, and the owner too when isMigrateOwner is set. Protocol 21 method 42.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param gid The gathering ID.
         * @param isMigrateOwner Whether ownership moves to us as well.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool UpdateSessionHost(ProtocolCallContext *context, qUnsignedInt32 gid, bool isMigrateOwner);

        /**
         * @brief Hands ownership to the first of the candidates the server accepts. Protocol 21 method 44.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param gid The gathering ID.
         * @param candidates PIDs in order of preference.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool MigrateGatheringOwnership(ProtocolCallContext *context, qUnsignedInt32 gid, const qList<qUnsignedInt32> &candidates);

        /**
         * @brief Leaves a gathering and shows the message to the remaining participants. Protocol 50 method 1.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param gid The gathering ID.
         * @param message A message shown to the other participants.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool EndParticipation(ProtocolCallContext *context, qUnsignedInt32 gid, const String &message);

    private:
        class Extension : public ClientProtocol
        {
        public:
            explicit Extension(qUnsignedInt16 id) : ClientProtocol(id)
            {}
            using ClientProtocol::Call;
        };
        Extension m_ext;
    };

    /**
     * @brief This is the client for the extended matchmaking protocol that most Wii U games use to create, find and join sessions.
     */
    class MatchmakeExtensionClient : public ClientProtocol
    {
    public:
        MatchmakeExtensionClient() : ClientProtocol(109)
        {}

        /**
         * @brief Stops new players from joining a session we own. Protocol 109 method 1.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param gid The gathering ID.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool CloseParticipation(ProtocolCallContext *context, qUnsignedInt32 gid);

        /**
         * @brief Lets new players join a session again. Protocol 109 method 2.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param gid The gathering ID.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool OpenParticipation(ProtocolCallContext *context, qUnsignedInt32 gid);

        /**
         * @brief Returns the sessions that match the criteria, without joining any. Protocol 109 method 4.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param criteria The filter to match against.
         * @param range The paging window.
         * @param gatherings Receives the gatherings found. Filled in when context completes, so it has to stay alive until then.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool BrowseMatchmakeSession(ProtocolCallContext *context, const MatchmakeSessionSearchCriteria &criteria, const ResultRange &range,
                                    qList<AnyObjectHolder<Gathering, String>> *gatherings);

        /**
         * @brief Sets one attribute of a session we own. Protocol 109 method 8.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param gid The gathering ID.
         * @param index The attribute index, from 0 to 5.
         * @param value The new attribute value.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool ModifyMatchmakeSessionAttribute(ProtocolCallContext *context, qUnsignedInt32 gid, qUnsignedInt32 index, qUnsignedInt32 value);

        /**
         * @brief Publishes our own status, which friends read with GetFriendNotificationData. Protocol 109 method 9.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param type A game-defined notification type.
         * @param param1 A game-defined value.
         * @param param2 A game-defined value.
         * @param strParam A game-defined string.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool UpdateNotificationData(ProtocolCallContext *context, qUnsignedInt32 type, qUnsignedInt32 param1, qUnsignedInt32 param2, String strParam);

        /**
         * @brief Replaces the game-defined data stored with a session. Protocol 109 method 11.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param gid The gathering ID.
         * @param buffer The new application data.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool UpdateApplicationBuffer(ProtocolCallContext *context, qUnsignedInt32 gid, qVector<qByte> buffer);

        /**
         * @brief We take exactly six attributes because the server keeps a fixed array of six per session. Protocol 109 method 12.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param gid The gathering ID.
         * @param attributes Points at six values.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool UpdateMatchmakeSessionAttribute(ProtocolCallContext *context, qUnsignedInt32 gid, qUnsignedInt32 *attributes);

        /**
         * @brief Returns the status our friends published with UpdateNotificationData, filtered by type. Protocol 109 method 13.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param events Receives the friends' notifications. Filled in when context completes, so it has to stay alive until then.
         * @param types The notification types to return.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool GetFriendNotificationData(ProtocolCallContext *context, qList<NotificationEvent> *events, const qList<qUnsignedInt32> &types);

        /**
         * @brief Returns the session each of the given players is in, which is how a game joins a friend. Protocol 109 method 16.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param pids The players to look up.
         * @param sessions Receives one entry for each player in a session. Filled in when context completes, so it has to stay alive until then.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool GetPlayingSession(ProtocolCallContext *context, const qList<qUnsignedInt32> &pids, qList<PlayingSession> *sessions);

        /**
         * @brief Protocol 109 method 17.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param gathering The gathering to create. Its ID is ignored.
         * @param gid Receives the new gathering's ID. Filled in when context completes, so it has to stay alive until then.
         * @param message A message shown to the other participants.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool CreatePersistentGathering(ProtocolCallContext *context, const PersistentGathering &gathering, qUnsignedInt32 *gid,
                                       const String &message);

        /**
         * @brief Protocol 109 method 18.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param gathering The gathering with its new fields. Its ID picks which one to update.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool UpdatePersistentGathering(ProtocolCallContext *context, const PersistentGathering &gathering);

        /**
         * @brief Protocol 109 method 19.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param gid The gathering ID.
         * @param message A message shown to the other participants.
         * @param password The password for a PasswordLocked gathering, otherwise empty.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool JoinPersistentGathering(ProtocolCallContext *context, qUnsignedInt32 gid, const String &message, const String &password);

        /**
         * @brief Protocol 109 method 20.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param gids The gatherings to look up.
         * @param gatherings Receives the gatherings found. Filled in when context completes, so it has to stay alive until then.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool FindPersistentGatheringByGatheringId(ProtocolCallContext *context, const qList<qUnsignedInt32> &gids,
                                                  qList<PersistentGathering> *gatherings);

        /**
         * @brief Returns the gatherings the server operator created, optionally only those open to join. Protocol 109 method 21.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param isAvailableOnly Whether to leave out gatherings that are full or closed.
         * @param range The paging window.
         * @param gatherings Receives the gatherings found. Filled in when context completes, so it has to stay alive until then.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool FindOfficialPersistentGathering(ProtocolCallContext *context, bool isAvailableOnly, const ResultRange &range,
                                             qList<PersistentGathering> *gatherings);

        /**
         * @brief Returns the persistent gatherings pid belongs to. Protocol 109 method 22.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param pid The player whose gatherings to return.
         * @param range The paging window.
         * @param gatherings Receives the gatherings found. Filled in when context completes, so it has to stay alive until then.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool FindPersistentGatheringByParticipant(ProtocolCallContext *context, qUnsignedInt32 pid, const ResultRange &range,
                                                  qList<PersistentGathering> *gatherings);

        /**
         * @brief Nintendo's signature takes no context, so we fire this call on a context we own. Reports how far the session's game has progressed, which AutoMatchmake uses to skip nearly finished sessions. Protocol 109 method 34.
         *
         * @param gid The gathering ID.
         * @param score Progress from 0 to 100.
         * @return False when the call could not be sent.
         */
        bool UpdateProgressScore(qUnsignedInt32 gid, qByte score);

        /**
         * @brief Makes the server generate a password that joining players then need. Protocol 109 method 36.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param gid The gathering ID.
         * @param password Receives the new password. Filled in when context completes, so it has to stay alive until then.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool GenerateMatchmakeSessionSystemPassword(ProtocolCallContext *context, qUnsignedInt32 gid, String *password);

        /**
         * @brief Protocol 109 method 37.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param gid The gathering ID.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool ClearMatchmakeSessionSystemPassword(ProtocolCallContext *context, qUnsignedInt32 gid);

        /**
         * @brief Creates a session with us as owner and host, and returns it with the gid filled in. Protocol 109 method 38.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param param The session and options.
         * @param session Receives the session as the server stored it. Filled in when context completes, so it has to stay alive until then.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool CreateMatchmakeSession(ProtocolCallContext *context, const CreateMatchmakeSessionParam &param, MatchmakeSession *session);

        /**
         * @brief Joins a known session by gid and returns it. Protocol 109 method 39.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param param The session and options.
         * @param session Receives the session as the server stored it. Filled in when context completes, so it has to stay alive until then.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool JoinMatchmakeSession(ProtocolCallContext *context, const JoinMatchmakeSessionParam &param, MatchmakeSession *session);

        /**
         * @brief Joins the best matching session, or creates one when nothing matches, and returns it. Protocol 109 method 40.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param param The session and options.
         * @param session Receives the session as the server stored it. Filled in when context completes, so it has to stay alive until then.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool AutoMatchmake(ProtocolCallContext *context, const AutoMatchmakeParam &param, MatchmakeSession *session);

        /**
         * @brief Protocol 109 method 41.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param gid The gathering ID.
         * @param session Receives the session as the server stored it. Filled in when context completes, so it has to stay alive until then.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool FindMatchmakeSessionByGatheringIdDetail(ProtocolCallContext *context, qUnsignedInt32 gid, MatchmakeSession *session);

        /**
         * @brief Protocol 109 method 44.
         *
         * @param context Tracks the call. It completes on the thread that calls Scheduler::Dispatch.
         * @param param The session and options.
         * @return False when the call could not be sent. The result of a sent call arrives through context.
         */
        bool UpdateMatchmakeSession(ProtocolCallContext *context, const UpdateMatchmakeSessionParam &param);

        /**
         * @brief Sets the key that scrambles gids into the codes players share. The game sets it once, before converting any codes.
         *
         * @param key The game's code key.
         */
        static void SetPersistentGatheringCodeKey(const String &key);
        static void ClearPersistentGatheringCodeKey();
        static qUnsignedInt64 GatheringIdToPersistentGatheringCode(qUnsignedInt32 gid);
        static qUnsignedInt32 PersistentGatheringCodeToGatheringId(qUnsignedInt64 code);

    private:
        ProtocolCallContext m_progressContext;
    };

} // namespace nn::nex
