/**
 * @file
 * @brief These are the basic NEX value types: qResult, String, Buffer, qBuffer, qList, qVector and DateTime, plus the result codes.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <list>
#include <string>
#include <vector>

namespace nn::nex
{

    using qByte          = uint8_t;
    using qUnsignedInt8  = uint8_t;
    using qUnsignedInt16 = uint16_t;
    using qUnsignedInt32 = uint32_t;
    using qUnsignedInt64 = uint64_t;
    using qInt8          = int8_t;
    using qInt16         = int16_t;
    using qInt32         = int32_t;
    using qInt64         = int64_t;
    using qChar          = wchar_t;

    /**
     * @brief This holds a NEX result code where bit 31 set means failure, the same layout the servers send in RMC responses.
     */
    class qResult
    {
    public:
        qResult() : m_code(0x00010001)
        {}
        qResult(bool success) : m_code(success ? 0x00010001 : 0x80010001)
        {}
        explicit qResult(qUnsignedInt32 code) : m_code(code)
        {}

        bool IsSuccess() const
        {
            return (m_code & 0x80000000) == 0;
        }
        operator bool() const
        {
            return IsSuccess();
        }
        qUnsignedInt32 GetReturnCode() const
        {
            return m_code;
        }

    private:
        qUnsignedInt32 m_code;
    };

    /**
     * @brief These are the result codes the servers return, with the failure bit already set.
     */
    namespace QResult
    {
        inline constexpr qUnsignedInt32 Core_Unknown                                        = 0x80010001;
        inline constexpr qUnsignedInt32 Core_NotImplemented                                 = 0x80010002;
        inline constexpr qUnsignedInt32 Core_InvalidPointer                                 = 0x80010003;
        inline constexpr qUnsignedInt32 Core_OperationAborted                               = 0x80010004;
        inline constexpr qUnsignedInt32 Core_Exception                                      = 0x80010005;
        inline constexpr qUnsignedInt32 Core_AccessDenied                                   = 0x80010006;
        inline constexpr qUnsignedInt32 Core_InvalidHandle                                  = 0x80010007;
        inline constexpr qUnsignedInt32 Core_InvalidIndex                                   = 0x80010008;
        inline constexpr qUnsignedInt32 Core_OutOfMemory                                    = 0x80010009;
        inline constexpr qUnsignedInt32 Core_InvalidArgument                                = 0x8001000A;
        inline constexpr qUnsignedInt32 Core_Timeout                                        = 0x8001000B;
        inline constexpr qUnsignedInt32 Core_InitializationFailure                          = 0x8001000C;
        inline constexpr qUnsignedInt32 Core_CallInitiationFailure                          = 0x8001000D;
        inline constexpr qUnsignedInt32 Core_RegistrationError                              = 0x8001000E;
        inline constexpr qUnsignedInt32 Core_BufferOverflow                                 = 0x8001000F;
        inline constexpr qUnsignedInt32 Core_InvalidLockState                               = 0x80010010;
        inline constexpr qUnsignedInt32 Core_InvalidSequence                                = 0x80010011;
        inline constexpr qUnsignedInt32 Core_SystemError                                    = 0x80010012;
        inline constexpr qUnsignedInt32 Core_Cancelled                                      = 0x80010013;
        inline constexpr qUnsignedInt32 RendezVous_ConnectionFailure                        = 0x80030001;
        inline constexpr qUnsignedInt32 RendezVous_NotAuthenticated                         = 0x80030002;
        inline constexpr qUnsignedInt32 RendezVous_InvalidUsername                          = 0x80030064;
        inline constexpr qUnsignedInt32 RendezVous_InvalidPassword                          = 0x80030065;
        inline constexpr qUnsignedInt32 RendezVous_UsernameAlreadyExists                    = 0x80030066;
        inline constexpr qUnsignedInt32 RendezVous_AccountDisabled                          = 0x80030067;
        inline constexpr qUnsignedInt32 RendezVous_AccountExpired                           = 0x80030068;
        inline constexpr qUnsignedInt32 RendezVous_ConcurrentLoginDenied                    = 0x80030069;
        inline constexpr qUnsignedInt32 RendezVous_EncryptionFailure                        = 0x8003006A;
        inline constexpr qUnsignedInt32 RendezVous_InvalidPID                               = 0x8003006B;
        inline constexpr qUnsignedInt32 RendezVous_MaxConnectionsReached                    = 0x8003006C;
        inline constexpr qUnsignedInt32 RendezVous_InvalidGID                               = 0x8003006D;
        inline constexpr qUnsignedInt32 RendezVous_InvalidControlScriptID                   = 0x8003006E;
        inline constexpr qUnsignedInt32 RendezVous_InvalidOperationInLiveEnvironment        = 0x8003006F;
        inline constexpr qUnsignedInt32 RendezVous_DuplicateEntry                           = 0x80030070;
        inline constexpr qUnsignedInt32 RendezVous_ControlScriptFailure                     = 0x80030071;
        inline constexpr qUnsignedInt32 RendezVous_ClassNotFound                            = 0x80030072;
        inline constexpr qUnsignedInt32 RendezVous_SessionVoid                              = 0x80030073;
        inline constexpr qUnsignedInt32 RendezVous_DDLMismatch                              = 0x80030075;
        inline constexpr qUnsignedInt32 RendezVous_InvalidConfiguration                     = 0x80030076;
        inline constexpr qUnsignedInt32 RendezVous_SessionFull                              = 0x800300C8;
        inline constexpr qUnsignedInt32 RendezVous_InvalidGatheringPassword                 = 0x800300C9;
        inline constexpr qUnsignedInt32 RendezVous_WithoutParticipationPeriod               = 0x800300CA;
        inline constexpr qUnsignedInt32 RendezVous_PersistentGatheringCreationMax           = 0x800300CB;
        inline constexpr qUnsignedInt32 RendezVous_PersistentGatheringParticipationMax      = 0x800300CC;
        inline constexpr qUnsignedInt32 RendezVous_DeniedByParticipants                     = 0x800300CD;
        inline constexpr qUnsignedInt32 RendezVous_ParticipantInBlackList                   = 0x800300CE;
        inline constexpr qUnsignedInt32 RendezVous_GameServerMaintenance                    = 0x800300CF;
        inline constexpr qUnsignedInt32 RendezVous_OperationPostpone                        = 0x800300D0;
        inline constexpr qUnsignedInt32 RendezVous_OutOfRatingRange                         = 0x800300D1;
        inline constexpr qUnsignedInt32 RendezVous_ConnectionDisconnected                   = 0x800300D2;
        inline constexpr qUnsignedInt32 RendezVous_InvalidOperation                         = 0x800300D3;
        inline constexpr qUnsignedInt32 RendezVous_NotParticipatedGathering                 = 0x800300D4;
        inline constexpr qUnsignedInt32 RendezVous_MatchmakeSessionUserPasswordUnmatch      = 0x800300D5;
        inline constexpr qUnsignedInt32 RendezVous_MatchmakeSessionSystemPasswordUnmatch    = 0x800300D6;
        inline constexpr qUnsignedInt32 RendezVous_UserIsOffline                            = 0x800300D7;
        inline constexpr qUnsignedInt32 RendezVous_AlreadyParticipatedGathering             = 0x800300D8;
        inline constexpr qUnsignedInt32 RendezVous_PermissionDenied                         = 0x800300D9;
        inline constexpr qUnsignedInt32 RendezVous_NotFriend                                = 0x800300DA;
        inline constexpr qUnsignedInt32 RendezVous_SessionClosed                            = 0x800300DB;
        inline constexpr qUnsignedInt32 RendezVous_DatabaseTemporarilyUnavailable           = 0x800300DC;
        inline constexpr qUnsignedInt32 RendezVous_InvalidUniqueID                          = 0x800300DD;
        inline constexpr qUnsignedInt32 RendezVous_MatchmakingWithdrawn                     = 0x800300DE;
        inline constexpr qUnsignedInt32 RendezVous_LimitExceeded                            = 0x800300DF;
        inline constexpr qUnsignedInt32 RendezVous_AccountTemporarilyDisabled               = 0x800300E0;
        inline constexpr qUnsignedInt32 RendezVous_PartiallyServiceClosed                   = 0x800300E1;
        inline constexpr qUnsignedInt32 RendezVous_ConnectionDisconnectedForConcurrentLogin = 0x800300E2;
        inline constexpr qUnsignedInt32 Transport_Unknown                                   = 0x80050001;
        inline constexpr qUnsignedInt32 Transport_ConnectionFailure                         = 0x80050002;
        inline constexpr qUnsignedInt32 Transport_InvalidURL                                = 0x80050003;
        inline constexpr qUnsignedInt32 Transport_InvalidKey                                = 0x80050004;
        inline constexpr qUnsignedInt32 Transport_InvalidURLType                            = 0x80050005;
        inline constexpr qUnsignedInt32 Transport_DuplicateEndpoint                         = 0x80050006;
        inline constexpr qUnsignedInt32 Transport_IOError                                   = 0x80050007;
        inline constexpr qUnsignedInt32 Transport_Timeout                                   = 0x80050008;
        inline constexpr qUnsignedInt32 Transport_ConnectionReset                           = 0x80050009;
        inline constexpr qUnsignedInt32 Transport_IncorrectRemoteAuthentication             = 0x8005000A;
        inline constexpr qUnsignedInt32 Transport_ServerRequestError                        = 0x8005000B;
        inline constexpr qUnsignedInt32 Transport_DecompressionFailure                      = 0x8005000C;
        inline constexpr qUnsignedInt32 Transport_ReliableSendBufferFullFatal               = 0x8005000D;
        inline constexpr qUnsignedInt32 Transport_UPnPCannotInit                            = 0x8005000E;
        inline constexpr qUnsignedInt32 Transport_UPnPCannotAddMapping                      = 0x8005000F;
        inline constexpr qUnsignedInt32 Transport_NatPMPCannotInit                          = 0x80050010;
        inline constexpr qUnsignedInt32 Transport_NatPMPCannotAddMapping                    = 0x80050011;
        inline constexpr qUnsignedInt32 Transport_UnsupportedNAT                            = 0x80050013;
        inline constexpr qUnsignedInt32 Transport_DNSError                                  = 0x80050014;
        inline constexpr qUnsignedInt32 Transport_ProxyError                                = 0x80050015;
        inline constexpr qUnsignedInt32 Transport_DataRemaining                             = 0x80050016;
        inline constexpr qUnsignedInt32 Transport_NoBuffer                                  = 0x80050017;
        inline constexpr qUnsignedInt32 Transport_NotFound                                  = 0x80050018;
        inline constexpr qUnsignedInt32 Transport_TemporaryServerError                      = 0x80050019;
        inline constexpr qUnsignedInt32 Transport_PermanentServerError                      = 0x8005001A;
        inline constexpr qUnsignedInt32 Transport_ServiceUnavailable                        = 0x8005001B;
        inline constexpr qUnsignedInt32 Transport_ReliableSendBufferFull                    = 0x8005001C;
        inline constexpr qUnsignedInt32 Transport_InvalidStation                            = 0x8005001D;
        inline constexpr qUnsignedInt32 Transport_InvalidSubStreamID                        = 0x8005001E;
        inline constexpr qUnsignedInt32 Transport_PacketBufferFull                          = 0x8005001F;
        inline constexpr qUnsignedInt32 Transport_NatTraversalError                         = 0x80050020;
        inline constexpr qUnsignedInt32 Transport_NatCheckError                             = 0x80050021;
    } // namespace QResult

    class String
    {
    public:
        String() = default;
        String(const qChar *s) : m_str(s ? s : L"")
        {}
        String(const char *s)
        {
            Assign(s);
        }

        const qChar *CStr() const
        {
            return m_str.c_str();
        }
        size_t GetLength() const
        {
            return m_str.size();
        }
        bool IsEmpty() const
        {
            return m_str.empty();
        }

        /**
         * @brief NEX strings travel as UTF-8, so we convert here instead of in every serializer.
         */
        std::string ToUtf8() const;
        void Assign(const char *utf8);

        bool operator==(const String &o) const
        {
            return m_str == o.m_str;
        }
        bool operator<(const String &o) const
        {
            return m_str < o.m_str;
        }

    private:
        std::wstring m_str;
    };

    /**
     * @brief This is a byte buffer that travels with a 32 bit length.
     */
    class Buffer
    {
    public:
        Buffer() = default;
        Buffer(const void *data, size_t size) : m_data((const qByte *) data, (const qByte *) data + size)
        {}

        const qByte *GetContentPtr() const
        {
            return m_data.data();
        }
        qByte *GetContentPtr()
        {
            return m_data.data();
        }
        size_t GetContentSize() const
        {
            return m_data.size();
        }
        void Resize(size_t size)
        {
            m_data.resize(size);
        }
        void Append(const void *data, size_t size)
        {
            m_data.insert(m_data.end(), (const qByte *) data, (const qByte *) data + size);
        }
        void Clear()
        {
            m_data.clear();
        }

    private:
        std::vector<qByte> m_data;
    };

    /**
     * @brief qBuffer differs from Buffer only on the wire, where it uses a 16 bit length.
     */
    class qBuffer : public Buffer
    {
    public:
        using Buffer::Buffer;
    };

    template<typename T>

    /**
     * @brief These are std::list and std::vector with the NEX names, and both travel as a 32 bit count followed by the elements.
     */
    class qList : public std::list<T>
    {
    public:
        using std::list<T>::list;
    };
    template<typename T>
    class qVector : public std::vector<T>
    {
    public:
        using std::vector<T>::vector;
    };

    /**
     * @brief This packs the date into the same bit fields the servers use: year, month, day, hour, minute and second from high to low.
     */
    class DateTime
    {
    public:
        DateTime() : m_value(0)
        {}
        explicit DateTime(qUnsignedInt64 value) : m_value(value)
        {}
        DateTime(qUnsignedInt16 year, qUnsignedInt8 month, qUnsignedInt8 day, qUnsignedInt8 hour, qUnsignedInt8 minute, qUnsignedInt8 second)
            : m_value(((qUnsignedInt64) year << 26) | ((qUnsignedInt64) month << 22) | ((qUnsignedInt64) day << 17) | ((qUnsignedInt64) hour << 12) |
                      ((qUnsignedInt64) minute << 6) | second)
        {}

        unsigned GetYear() const
        {
            return (unsigned) (m_value >> 26);
        }
        unsigned GetMonth() const
        {
            return (unsigned) (m_value >> 22) & 0xF;
        }
        unsigned GetDay() const
        {
            return (unsigned) (m_value >> 17) & 0x1F;
        }
        unsigned GetHour() const
        {
            return (unsigned) (m_value >> 12) & 0x1F;
        }
        unsigned GetMinute() const
        {
            return (unsigned) (m_value >> 6) & 0x3F;
        }
        unsigned GetSecond() const
        {
            return (unsigned) m_value & 0x3F;
        }
        qUnsignedInt64 GetValue() const
        {
            return m_value;
        }
        bool IsNever() const
        {
            return m_value == 0;
        }
        bool IsValid() const
        {
            return GetMonth() >= 1 && GetMonth() <= 12 && GetDay() >= 1 && GetDay() <= 31;
        }
        qUnsignedInt64 ToEpochTime() const;

        static const DateTime Never;

    private:
        qUnsignedInt64 m_value;
    };

} // namespace nn::nex
