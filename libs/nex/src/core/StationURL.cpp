#include <cstdlib>
#include <nn/nex/StationURL.h>

namespace nn::nex
{

    namespace
    {

        const char *const kSchemes[] = {"", "prudp", "prudps", "udp"};

    }

    // Station URLs look like "prudps:/address=1.2.3.4;port=60000;PID=2", with parameters in any order
    bool StationURL::Parse(const String &url)
    {
        std::string s = url.ToUtf8();
        m_params.clear();
        size_t sep = s.find(":/");
        if (sep == std::string::npos)
            return false;
        std::string scheme = s.substr(0, sep);
        m_urlType          = UnknownURLType;
        for (qInt32 i = prudp; i <= udp; i++)
            if (scheme == kSchemes[i])
                m_urlType = (URLType) i;
        size_t pos = sep + 2;
        while (pos < s.size())
        {
            size_t end = s.find(';', pos);
            if (end == std::string::npos)
                end = s.size();
            std::string kv = s.substr(pos, end - pos);
            size_t eq      = kv.find('=');
            if (eq != std::string::npos)
                m_params[kv.substr(0, eq)] = String(kv.substr(eq + 1).c_str());
            pos = end + 1;
        }
        return m_urlType != UnknownURLType;
    }

    String StationURL::GetURL() const
    {
        std::string s = std::string(kSchemes[m_urlType]) + ":/";
        bool first    = true;
        for (const auto &[key, value] : m_params)
        {
            if (!first)
                s += ';';
            s += key + "=" + value.ToUtf8();
            first = false;
        }
        return String(s.c_str());
    }

    InetAddress StationURL::GetInetAddress() const
    {
        InetAddress address;
        address.SetAddress(GetAddress().ToUtf8().c_str());
        address.SetPortNumber(GetPortNumber());
        return address;
    }

    void StationURL::SetInetAddress(const InetAddress *address)
    {
        SetAddress(address->GetAddressStr());
        SetPortNumber(address->GetPortNumber());
    }

    String StationURL::GetParameter(const char *key) const
    {
        auto it = m_params.find(key);
        return it == m_params.end() ? String() : it->second;
    }

    qUnsignedInt32 StationURL::GetUint(const char *key) const
    {
        auto it = m_params.find(key);
        return it == m_params.end() ? 0 : (qUnsignedInt32) strtoul(it->second.ToUtf8().c_str(), nullptr, 10);
    }

    void StationURL::SetUint(const char *key, qUnsignedInt32 v)
    {
        m_params[key] = String(std::to_string(v).c_str());
    }

} // namespace nn::nex
