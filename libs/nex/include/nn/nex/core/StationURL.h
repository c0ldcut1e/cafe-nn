/**
 * @file
 * @brief Station URLs say how to reach a player or server, such as prudps:/address=1.2.3.4;port=60000;PID=2;CID=1, and are what peers exchange to connect.
 */

#pragma once

#include <map>
#include <nn/nex/platform/InetAddress.h>
#include <nn/nex/types.h>

namespace nn::nex
{

    namespace NATProperties
    {
        enum MappingProperties
        {
            UnknownMapping = 0,
            EIMapping      = 1,
            EDMapping      = 2
        };
        enum FilteringProperties
        {
            UnknownFiltering = 0,
            PIFiltering      = 1,
            PDFiltering      = 2
        };
    } // namespace NATProperties

    namespace Stream
    {
        enum Type
        {
            DO               = 1,
            RV               = 2,
            OldRVSec         = 3,
            SBMGMT           = 4,
            NAT              = 5,
            SessionDiscovery = 6,
            NATEcho          = 7,
            Routing          = 8,
            Game             = 9,
            RVSecure         = 10,
            Relay            = 11
        };
    } // namespace Stream

    /**
     * @brief This parses and builds station URLs, and keeps parameters it doesn't know so they survive a round trip.
     */
    class StationURL
    {
    public:
        enum URLType
        {
            UnknownURLType = 0,
            prudp          = 1,
            prudps         = 2,
            udp            = 3
        };

        /**
         * @brief These are the bits of the type parameter, which tells peers how to reach the station.
         */
        static constexpr qUnsignedInt32 BehindNAT = 1;
        static constexpr qUnsignedInt32 Public    = 2;

        StationURL() = default;
        explicit StationURL(const String &url)
        {
            Parse(url);
        }
        StationURL(const StationURL &)            = default;
        StationURL &operator=(const StationURL &) = default;
        ~StationURL()                             = default;

        bool Parse(const String &url);
        String GetURL() const;

        URLType GetURLType() const
        {
            return m_urlType;
        }
        void SetURLType(URLType type)
        {
            m_urlType = type;
        }

        String GetAddress() const
        {
            return GetParameter("address");
        }
        void SetAddress(const String &address)
        {
            SetParameter("address", address);
        }
        qUnsignedInt16 GetPortNumber() const
        {
            return (qUnsignedInt16) GetUint("port");
        }
        void SetPortNumber(qUnsignedInt16 port)
        {
            SetUint("port", port);
        }
        InetAddress GetInetAddress() const;
        void SetInetAddress(const InetAddress *address);

        qUnsignedInt32 GetPrincipalID() const
        {
            return GetUint("PID");
        }
        void SetPrincipalID(qUnsignedInt32 pid)
        {
            SetUint("PID", pid);
        }
        qUnsignedInt32 GetConnectionID() const
        {
            return GetUint("CID");
        }
        void SetConnectionID(qUnsignedInt32 id)
        {
            SetUint("CID", id);
        }
        qUnsignedInt32 GetRVConnectionID() const
        {
            return GetUint("RVCID");
        }
        void SetRVConnectionID(qUnsignedInt32 id)
        {
            SetUint("RVCID", id);
        }
        qUnsignedInt8 GetStreamID() const
        {
            return (qUnsignedInt8) GetUint("sid");
        }
        void SetStreamID(qUnsignedInt8 id)
        {
            SetUint("sid", id);
        }
        Stream::Type GetStreamType() const
        {
            return (Stream::Type) GetUint("stream");
        }
        void SetStreamType(Stream::Type type)
        {
            SetUint("stream", type);
        }

        NATProperties::MappingProperties GetNATMapping() const
        {
            return (NATProperties::MappingProperties) GetUint("natm");
        }
        void SetNATMapping(NATProperties::MappingProperties v)
        {
            SetUint("natm", v);
        }
        NATProperties::FilteringProperties GetNATFiltering() const
        {
            return (NATProperties::FilteringProperties) GetUint("natf");
        }
        void SetNATFiltering(NATProperties::FilteringProperties v)
        {
            SetUint("natf", v);
        }
        bool GetProbeRequestInitiation() const
        {
            return GetUint("probeinit") != 0;
        }
        void SetProbeRequestInitiation(bool v)
        {
            SetUint("probeinit", v);
        }
        qUnsignedInt32 GetType() const
        {
            return GetUint("type");
        }
        void SetType(qUnsignedInt32 v)
        {
            SetUint("type", v);
        }
        bool IsBehindNAT() const
        {
            return GetType() & BehindNAT;
        }
        bool IsPublic() const
        {
            return GetType() & Public;
        }

        bool GetUseRelayServer() const
        {
            return GetUint("R") != 0;
        }
        void SetUseRelayServer(bool v)
        {
            SetUint("R", v);
        }
        String GetRelayServerAddress() const
        {
            return GetParameter("Rsa");
        }
        void SetRelayServerAddress(const String &v)
        {
            SetParameter("Rsa", v);
        }
        qUnsignedInt16 GetRelayServerPort() const
        {
            return (qUnsignedInt16) GetUint("Rsp");
        }
        void SetRelayServerPort(qUnsignedInt16 v)
        {
            SetUint("Rsp", v);
        }
        String GetRelayAddress() const
        {
            return GetParameter("Ra");
        }
        void SetRelayAddress(const String &v)
        {
            SetParameter("Ra", v);
        }
        qUnsignedInt16 GetRelayPort() const
        {
            return (qUnsignedInt16) GetUint("Rp");
        }
        void SetRelayPort(qUnsignedInt16 v)
        {
            SetUint("Rp", v);
        }

        bool HasParameter(const char *key) const
        {
            return m_params.count(key) != 0;
        }
        String GetParameter(const char *key) const;
        void SetParameter(const char *key, const String &value)
        {
            m_params[key] = value;
        }

    private:
        qUnsignedInt32 GetUint(const char *key) const;
        void SetUint(const char *key, qUnsignedInt32 v);

        URLType m_urlType = prudp;
        std::map<std::string, String> m_params;
    };

} // namespace nn::nex
