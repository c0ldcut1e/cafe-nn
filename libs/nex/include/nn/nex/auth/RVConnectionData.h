/**
 * @file
 * @brief The authentication server returns this with the ticket. It holds the secure server's station URL and the server time.
 */

#pragma once

#include <nn/nex/core/DateTime.h>
#include <nn/nex/core/StationURL.h>
#include <nn/nex/rmc/Message.h>
#include <nn/nex/types.h>
#include <vector>

namespace nn::nex
{

    /**
     * @brief The authentication server returns this with the ticket. It holds the secure server's station URL and the server time.
     */
    class RVConnectionData
    {
    public:
        const StationURL &GetRegularProtocols() const
        {
            return m_regular;
        }
        const StationURL &GetSpecialProtocolsURL() const
        {
            return m_special;
        }
        const qList<qByte> &GetSpecialProtocols() const
        {
            return m_specialProtocols;
        }
        const DateTime &GetCurrentUTCTime() const
        {
            return m_currentTime;
        }
        bool Extract(Message &m);

    private:
        StationURL m_regular;
        qList<qByte> m_specialProtocols;
        StationURL m_special;
        DateTime m_currentTime;
    };

} // namespace nn::nex
