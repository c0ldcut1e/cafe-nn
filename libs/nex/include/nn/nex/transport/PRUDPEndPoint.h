/**
 * @file
 * @brief This is the reliable UDP transport under every NEX connection, with the handshake, retransmission, acknowledgement and RC4 stream encryption.
 */

#pragma once

#include <functional>
#include <map>
#include <nn/nex/core/Buffer.h>
#include <nn/nex/crypto/MD5.h>
#include <nn/nex/crypto/RC4.h>
#include <nn/nex/platform/SocketDriver.h>
#include <nn/nex/runtime/CallContext.h>
#include <nn/nex/runtime/Scheduler.h>
#include <nn/nex/transport/PRUDPMessageV1.h>

namespace nn::nex
{

    /**
     * @brief This is one client connection to a NEX server over PRUDP v1, with reliable ordered delivery on substream 0.
     */
    class PRUDPEndPoint : public SchedulerTask
    {
    public:
        using ReceiveHandler    = std::function<void(Buffer &)>;
        using DisconnectHandler = std::function<void(qResult)>;

        enum State
        {
            Closed,
            SynSent,
            ConnectSent,
            Connected
        };

        PRUDPEndPoint();
        ~PRUDPEndPoint() override;
        PRUDPEndPoint(const PRUDPEndPoint &)            = delete;
        PRUDPEndPoint &operator=(const PRUDPEndPoint &) = delete;

        void SetAccessKey(const char *accessKey)
        {
            m_sendContext.SetAccessKey(accessKey);
        }

        /**
         * @brief A null session key means the authentication server, which uses the fixed key every client knows.
         *
         * @param context Completes when the handshake finishes or times out.
         * @param server The server's station URL.
         * @param connectPayload The Kerberos ticket and check value for the secure server, empty for the authentication server.
         * @param sessionKey The key from the ticket, or null for the authentication server.
         * @return Whether the connection attempt started.
         */
        qResult Connect(CallContext *context, const InetAddress &server, const Buffer &connectPayload, const Buffer *sessionKey);
        qResult Send(const Buffer &payload);
        void Disconnect();
        void Close();

        State GetState() const
        {
            return m_state;
        }
        bool IsConnected() const
        {
            return m_state == Connected;
        }
        const Buffer &GetConnectResponse() const
        {
            return m_connectResponse;
        }
        const InetAddress &GetServerAddress() const
        {
            return m_server;
        }
        InetAddress GetLocalAddress() const
        {
            return m_socket.GetLocalAddress();
        }

        void SetReceiveHandler(ReceiveHandler handler)
        {
            m_onReceive = std::move(handler);
        }
        void SetDisconnectHandler(DisconnectHandler handler)
        {
            m_onDisconnect = std::move(handler);
        }

        void Poll(TimeValue now) override;

    private:
        struct Pending
        {
            Buffer datagram;
            TimeValue nextSend;
            qUnsignedInt32 tries;
        };

        void HandlePacket(PRUDPPacket &packet, TimeValue now);
        void HandleAck(const PRUDPPacket &packet);
        void HandleReliableData(PRUDPPacket &packet);
        void SendSyn();
        void SendConnect();
        void SendAck(const PRUDPPacket &packet);
        void SendPing(qUnsignedInt16 flags, qUnsignedInt16 sequence);
        void Transmit(const Buffer &datagram);
        PRUDPPacket NewPacket(PacketType type, qUnsignedInt16 flags) const;
        void Fail(qResult result);

        SocketDriver m_socket;
        InetAddress m_server;
        State m_state                 = Closed;
        CallContext *m_connectContext = nullptr;

        PRUDPSignatureContext m_sendContext;
        Buffer m_connectPayload;
        Buffer m_connectResponse;
        Buffer m_sessionKey;
        bool m_secure                       = false;
        qUnsignedInt8 m_clientSignature[16] = {};
        qUnsignedInt8 m_sessionId           = 0;
        qUnsignedInt8 m_serverSessionId     = 0;
        qUnsignedInt32 m_minorVersion       = 2;

        RC4 m_sendCipher;
        RC4 m_receiveCipher;
        qUnsignedInt16 m_nextSequence     = 2;
        qUnsignedInt16 m_expectedSequence = 1;
        qUnsignedInt16 m_pingSequence     = 0;
        qUnsignedInt8 m_nextFragment      = 0;
        std::map<qUnsignedInt16, Pending> m_unacknowledged;
        std::map<qUnsignedInt16, PRUDPPacket> m_outOfOrder;
        Buffer m_reassembly;

        TimeValue m_handshakeDeadline   = 0;
        qUnsignedInt32 m_handshakeTries = 0;
        TimeValue m_lastReceived        = 0;
        TimeValue m_lastSent            = 0;

        ReceiveHandler m_onReceive;
        DisconnectHandler m_onDisconnect;
    };

} // namespace nn::nex
