#include <nn/nex/platform/RandomNumberGenerator.h>
#include <nn/nex/transport/PRUDPEndPoint.h>

namespace nn::nex
{

    namespace
    {

        constexpr qUnsignedInt8 ClientPort = 0xAF;
        constexpr qUnsignedInt8 ServerPort = 0xA1;
        constexpr size_t FragmentSize      = 1300;
        constexpr TimeValue ResendInterval = 1000;
        constexpr qUnsignedInt32 MaxTries  = 10;
        constexpr TimeValue PingInterval   = 5000;
        constexpr TimeValue IdleTimeout    = 30000;

        const char AuthenticationKey[] = "CD&ML";

        // Sequence numbers wrap at 16 bits, so ordering compares the signed distance instead of the raw values
        bool SequenceBefore(qUnsignedInt16 a, qUnsignedInt16 b)
        {
            return (qInt16) (a - b) < 0;
        }

    } // namespace

    PRUDPEndPoint::PRUDPEndPoint() = default;

    PRUDPEndPoint::~PRUDPEndPoint()
    {
        m_onDisconnect = nullptr;
        m_onReceive    = nullptr;
        Close();
    }

    qResult PRUDPEndPoint::Connect(CallContext *context, const InetAddress &server, const Buffer &connectPayload, const Buffer *sessionKey)
    {
        if (m_state != Closed)
            return qResult(QResult::Core_InvalidSequence);
        qResult opened = m_socket.Open();
        if (!opened)
            return opened;

        RandomNumberGenerator rng;
        m_server         = server;
        m_connectContext = context;
        m_connectPayload = connectPayload;
        m_secure         = sessionKey != nullptr;
        m_sessionKey     = sessionKey ? *sessionKey : Buffer();
        rng.Fill(m_clientSignature, sizeof(m_clientSignature));
        m_sessionId        = (qUnsignedInt8) rng.GetRandomNumber();
        m_nextSequence     = 2;
        m_expectedSequence = 1;
        m_pingSequence     = 0;
        m_unacknowledged.clear();
        m_outOfOrder.clear();
        m_reassembly.Clear();
        m_sendContext.sessionKey = Buffer();
        memset(m_sendContext.connectionSignature, 0, 16);

        if (m_secure)
        {
            m_sendCipher.SetKey(m_sessionKey.GetContentPtr(), m_sessionKey.GetContentSize());
            m_receiveCipher.SetKey(m_sessionKey.GetContentPtr(), m_sessionKey.GetContentSize());
        }
        else
        {
            m_sendCipher.SetKey(AuthenticationKey, sizeof(AuthenticationKey) - 1);
            m_receiveCipher.SetKey(AuthenticationKey, sizeof(AuthenticationKey) - 1);
        }

        m_lastReceived   = Time::GetTime();
        m_state          = SynSent;
        m_handshakeTries = 0;
        Scheduler::GetInstance()->RegisterTask(this);
        SendSyn();
        return qResult(true);
    }

    PRUDPPacket PRUDPEndPoint::NewPacket(PacketType type, qUnsignedInt16 flags) const
    {
        PRUDPPacket packet;
        packet.sourcePort      = ClientPort;
        packet.destinationPort = ServerPort;
        packet.type            = type;
        packet.flags           = flags;
        packet.sessionId       = m_sessionId;
        packet.minorVersion    = m_minorVersion;
        return packet;
    }

    void PRUDPEndPoint::Transmit(const Buffer &datagram)
    {
        m_socket.SendTo(m_server, datagram.GetContentPtr(), datagram.GetContentSize());
        m_lastSent = Time::GetTime();
    }

    void PRUDPEndPoint::SendSyn()
    {
        PRUDPPacket packet          = NewPacket(PacketType::Syn, PacketFlag::NeedAck);
        packet.sequenceId           = 0;
        PRUDPSignatureContext empty = m_sendContext;
        empty.sessionKey            = Buffer();
        memset(empty.connectionSignature, 0, 16);
        Transmit(PRUDPMessageV1::Encode(packet, empty));
        m_handshakeDeadline = Time::GetTime() + ResendInterval;
        m_handshakeTries++;
    }

    void PRUDPEndPoint::SendConnect()
    {
        PRUDPPacket packet = NewPacket(PacketType::Connect, PacketFlag::Reliable | PacketFlag::NeedAck);
        packet.sequenceId  = 1;
        memcpy(packet.connectionSignature, m_clientSignature, 16);
        packet.payload = m_connectPayload;
        Transmit(PRUDPMessageV1::Encode(packet, m_sendContext));
        m_handshakeDeadline = Time::GetTime() + ResendInterval;
        m_handshakeTries++;
    }

    void PRUDPEndPoint::SendAck(const PRUDPPacket &received)
    {
        PRUDPPacket packet = NewPacket(received.type, PacketFlag::Ack);
        packet.sequenceId  = received.sequenceId;
        packet.substreamId = received.substreamId;
        packet.fragmentId  = received.fragmentId;
        Transmit(PRUDPMessageV1::Encode(packet, m_sendContext));
    }

    void PRUDPEndPoint::SendPing(qUnsignedInt16 flags, qUnsignedInt16 sequence)
    {
        PRUDPPacket packet = NewPacket(PacketType::Ping, flags);
        packet.sequenceId  = sequence;
        Transmit(PRUDPMessageV1::Encode(packet, m_sendContext));
    }

    qResult PRUDPEndPoint::Send(const Buffer &payload)
    {
        if (m_state != Connected)
            return qResult(QResult::Transport_ConnectionFailure);
        size_t offset          = 0;
        size_t total           = payload.GetContentSize();
        qUnsignedInt8 fragment = 1;
        TimeValue now          = Time::GetTime();
        do
        {
            size_t chunk       = total - offset < FragmentSize ? total - offset : FragmentSize;
            bool last          = offset + chunk == total;
            PRUDPPacket packet = NewPacket(PacketType::Data, PacketFlag::Reliable | PacketFlag::NeedAck | PacketFlag::HasSize);
            packet.sequenceId  = m_nextSequence++;
            packet.fragmentId  = last ? 0 : fragment++;
            packet.payload     = Buffer(payload.GetContentPtr() + offset, chunk);
            // The cipher is one continuous stream, so we encrypt once here and resend the stored ciphertext
            m_sendCipher.Process(packet.payload.GetContentPtr(), chunk);
            Pending pending{PRUDPMessageV1::Encode(packet, m_sendContext), now + ResendInterval, 1};
            Transmit(pending.datagram);
            m_unacknowledged[packet.sequenceId] = std::move(pending);
            offset += chunk;
        } while (offset < total);
        return qResult(true);
    }

    void PRUDPEndPoint::Disconnect()
    {
        if (m_state == Connected)
        {
            PRUDPPacket packet = NewPacket(PacketType::Disconnect, PacketFlag::Reliable | PacketFlag::NeedAck);
            packet.sequenceId  = m_nextSequence++;
            Buffer datagram    = PRUDPMessageV1::Encode(packet, m_sendContext);
            // We never wait for the acknowledgement, so sending a few copies makes it likely the server sees one
            for (qInt32 i = 0; i < 3; i++) Transmit(datagram);
        }
        Close();
    }

    void PRUDPEndPoint::Close()
    {
        m_state = Closed;
        Scheduler::GetInstance()->UnregisterTask(this);
        m_socket.Close();
        m_unacknowledged.clear();
        m_outOfOrder.clear();
        if (m_connectContext)
        {
            CallContext *context = m_connectContext;
            m_connectContext     = nullptr;
            context->SignalFailure(qResult(QResult::Core_Cancelled));
        }
    }

    void PRUDPEndPoint::Fail(qResult result)
    {
        CallContext *context = m_connectContext;
        m_connectContext     = nullptr;
        bool wasConnected    = m_state == Connected;
        Close();
        if (context)
            context->SignalFailure(result);
        if (wasConnected && m_onDisconnect)
            m_onDisconnect(result);
    }

    void PRUDPEndPoint::Poll(TimeValue now)
    {
        qUnsignedInt8 datagram[2048];
        InetAddress from;
        qInt32 received = 0;
        while (m_state != Closed && (received = m_socket.RecvFrom(&from, datagram, sizeof(datagram))) > 0)
        {
            if (!(from == m_server))
                continue;
            size_t offset = 0;
            PRUDPPacket packet;
            while (m_state != Closed && PRUDPMessageV1::Decode(datagram, (size_t) received, offset, packet))
            {
                m_lastReceived = now;
                HandlePacket(packet, now);
                packet = PRUDPPacket();
            }
        }
        if (m_state == Closed)
            return;
        if (received < 0)
        {
            Fail(qResult(QResult::Transport_IOError));
            return;
        }

        if (m_state == SynSent || m_state == ConnectSent)
        {
            if (now >= m_handshakeDeadline)
            {
                if (m_handshakeTries >= MaxTries)
                {
                    Fail(qResult(QResult::Transport_Timeout));
                    return;
                }
                if (m_state == SynSent)
                    SendSyn();
                else
                    SendConnect();
            }
            return;
        }

        for (auto &[sequence, pending] : m_unacknowledged)
        {
            if (now < pending.nextSend)
                continue;
            if (pending.tries >= MaxTries)
            {
                Fail(qResult(QResult::Transport_Timeout));
                return;
            }
            Transmit(pending.datagram);
            pending.tries++;
            pending.nextSend = now + ResendInterval * pending.tries;
        }

        if (now - m_lastReceived > IdleTimeout)
        {
            Fail(qResult(QResult::Transport_Timeout));
            return;
        }
        if (now - m_lastSent > PingInterval)
            SendPing(PacketFlag::NeedAck, ++m_pingSequence);
    }

    void PRUDPEndPoint::HandlePacket(PRUDPPacket &packet, TimeValue now)
    {
        (void) now;
        bool isAck = packet.flags & PacketFlag::Ack;
        switch (packet.type)
        {
            case PacketType::Syn:
                if (isAck && m_state == SynSent)
                {
                    // Every packet we send from here on is signed with the signature the server chose
                    memcpy(m_sendContext.connectionSignature, packet.connectionSignature, 16);
                    m_minorVersion   = packet.minorVersion;
                    m_state          = ConnectSent;
                    m_handshakeTries = 0;
                    SendConnect();
                }
                break;
            case PacketType::Connect:
                if (isAck && m_state == ConnectSent)
                {
                    m_serverSessionId = packet.sessionId;
                    m_connectResponse = packet.payload;
                    if (m_secure)
                        m_sendContext.sessionKey = m_sessionKey;
                    m_state              = Connected;
                    CallContext *context = m_connectContext;
                    m_connectContext     = nullptr;
                    if (context)
                        context->SignalSuccess();
                }
                break;
            case PacketType::Data:
                if (m_state != Connected)
                    break;
                if (isAck || (packet.flags & PacketFlag::MultiAck))
                    HandleAck(packet);
                else if (packet.flags & PacketFlag::Reliable)
                    HandleReliableData(packet);
                break;
            case PacketType::Ping:
                if ((packet.flags & PacketFlag::NeedAck) && !isAck)
                    SendPing(PacketFlag::Ack, packet.sequenceId);
                break;
            case PacketType::Disconnect:
                if (!isAck)
                {
                    if (packet.flags & PacketFlag::NeedAck)
                        SendAck(packet);
                    Fail(qResult(QResult::Transport_ConnectionReset));
                }
                break;
        }
    }

    void PRUDPEndPoint::HandleAck(const PRUDPPacket &packet)
    {
        if (!(packet.flags & PacketFlag::MultiAck))
        {
            m_unacknowledged.erase(packet.sequenceId);
            return;
        }
        const Buffer &p   = packet.payload;
        const qByte *data = p.GetContentPtr();
        size_t size       = p.GetContentSize();
        auto u16          = [&](size_t at)
        {
            return (qUnsignedInt16) (data[at] | (data[at + 1] << 8));
        };

        qUnsignedInt16 base;
        size_t first;
        // The newer layout announces its own entry count, which is how we tell it apart from the older one
        if (size >= 4 && size == 4 + 2 * (size_t) data[1])
        {
            base  = u16(2);
            first = 4;
        }
        else
        {
            base  = packet.sequenceId;
            first = 0;
        }
        for (auto it = m_unacknowledged.begin(); it != m_unacknowledged.end();)
        {
            if (!SequenceBefore(base, it->first))
                it = m_unacknowledged.erase(it);
            else
                ++it;
        }
        for (size_t at = first; at + 2 <= size; at += 2) m_unacknowledged.erase(u16(at));
    }

    void PRUDPEndPoint::HandleReliableData(PRUDPPacket &packet)
    {
        if (packet.flags & PacketFlag::NeedAck)
            SendAck(packet);
        if (SequenceBefore(packet.sequenceId, m_expectedSequence))
            return;
        m_outOfOrder[packet.sequenceId] = std::move(packet);

        // We decrypt strictly in sequence order because the server encrypted the stream in that order
        for (auto it = m_outOfOrder.find(m_expectedSequence); it != m_outOfOrder.end(); it = m_outOfOrder.find(m_expectedSequence))
        {
            PRUDPPacket next = std::move(it->second);
            m_outOfOrder.erase(it);
            m_expectedSequence++;
            m_receiveCipher.Process(next.payload.GetContentPtr(), next.payload.GetContentSize());
            m_reassembly.Append(next.payload.GetContentPtr(), next.payload.GetContentSize());
            if (next.fragmentId == 0)
            {
                Buffer message = std::move(m_reassembly);
                m_reassembly.Clear();
                if (m_onReceive)
                    m_onReceive(message);
                if (m_state != Connected)
                    return;
            }
        }
    }

} // namespace nn::nex
