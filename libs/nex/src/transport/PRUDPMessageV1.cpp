#include <nn/nex/ByteStream.h>
#include <nn/nex/Crypto.h>
#include <nn/nex/PRUDPMessageV1.h>

namespace nn::nex
{

    namespace
    {

        constexpr qUnsignedInt8 kMagic[2] = {0xEA, 0xD0};
        constexpr size_t kHeaderSize      = 12;

        enum Option : qUnsignedInt8
        {
            SupportedFunctions  = 0,
            ConnectionSignature = 1,
            FragmentId          = 2,
            InitialSequenceId   = 3,
            MaxSubstreamId      = 4
        };

        bool IsHandshake(PacketType type)
        {
            return type == PacketType::Syn || type == PacketType::Connect;
        }

    } // namespace

    void PRUDPSignatureContext::SetAccessKey(const char *accessKey)
    {
        size_t len = strlen(accessKey);
        MD5::Hash(accessKey, len, accessKeyHash);
        accessKeySum = 0;
        for (size_t i = 0; i < len; i++) accessKeySum += (qUnsignedInt8) accessKey[i];
    }

    void PRUDPMessageV1::EncodeHeader(const PRUDPPacket &packet, qUnsignedInt8 optionsSize, qUnsignedInt8 out[12])
    {
        qUnsignedInt16 payloadSize = (qUnsignedInt16) packet.payload.GetContentSize();
        qUnsignedInt16 typeFlags   = (qUnsignedInt16) ((qUnsignedInt16) packet.type | (packet.flags << 4));
        out[0]                     = 1;
        out[1]                     = optionsSize;
        out[2]                     = (qUnsignedInt8) payloadSize;
        out[3]                     = (qUnsignedInt8) (payloadSize >> 8);
        out[4]                     = packet.sourcePort;
        out[5]                     = packet.destinationPort;
        out[6]                     = (qUnsignedInt8) typeFlags;
        out[7]                     = (qUnsignedInt8) (typeFlags >> 8);
        out[8]                     = packet.sessionId;
        out[9]                     = packet.substreamId;
        out[10]                    = (qUnsignedInt8) packet.sequenceId;
        out[11]                    = (qUnsignedInt8) (packet.sequenceId >> 8);
    }

    // Servers parse options by id but older ones expect this exact order, so we never reorder them
    Buffer PRUDPMessageV1::EncodeOptions(const PRUDPPacket &packet)
    {
        ByteStream s;
        if (IsHandshake(packet.type))
        {
            s.WriteU8(SupportedFunctions);
            s.WriteU8(4);
            s.WriteU32((packet.minorVersion & 0xFF) | (packet.supportedFunctions << 8));
            s.WriteU8(ConnectionSignature);
            s.WriteU8(16);
            s.Write(packet.connectionSignature, 16);
            if (packet.type == PacketType::Connect)
            {
                s.WriteU8(InitialSequenceId);
                s.WriteU8(2);
                s.WriteU16(packet.initialUnreliableSequenceId);
            }
            s.WriteU8(MaxSubstreamId);
            s.WriteU8(1);
            s.WriteU8(packet.maximumSubstreamId);
        }
        else if (packet.type == PacketType::Data)
        {
            s.WriteU8(FragmentId);
            s.WriteU8(1);
            s.WriteU8(packet.fragmentId);
        }
        return Buffer(s.GetData(), s.GetSize());
    }

    void PRUDPMessageV1::ComputeSignature(const PRUDPPacket &packet, const PRUDPSignatureContext &ctx, qUnsignedInt8 out[16])
    {
        Buffer options = EncodeOptions(packet);
        qUnsignedInt8 header[kHeaderSize];
        EncodeHeader(packet, (qUnsignedInt8) options.GetContentSize(), header);

        // The first four header bytes are left out because they only describe sizes the HMAC covers anyway
        ByteStream s;
        s.Write(header + 4, kHeaderSize - 4);
        s.Write(ctx.sessionKey.GetContentPtr(), ctx.sessionKey.GetContentSize());
        s.WriteU32(ctx.accessKeySum);
        s.Write(ctx.connectionSignature, 16);
        s.Write(options.GetContentPtr(), options.GetContentSize());
        s.Write(packet.payload.GetContentPtr(), packet.payload.GetContentSize());
        HmacMD5(ctx.accessKeyHash, 16, s.GetData(), s.GetSize(), out);
    }

    Buffer PRUDPMessageV1::Encode(PRUDPPacket &packet, const PRUDPSignatureContext &ctx)
    {
        Buffer options = EncodeOptions(packet);
        qUnsignedInt8 header[kHeaderSize];
        EncodeHeader(packet, (qUnsignedInt8) options.GetContentSize(), header);
        ComputeSignature(packet, ctx, packet.signature);

        Buffer out;
        out.Append(kMagic, 2);
        out.Append(header, kHeaderSize);
        out.Append(packet.signature, 16);
        out.Append(options.GetContentPtr(), options.GetContentSize());
        out.Append(packet.payload.GetContentPtr(), packet.payload.GetContentSize());
        return out;
    }

    bool PRUDPMessageV1::Decode(const qUnsignedInt8 *data, size_t size, size_t &offset, PRUDPPacket &out)
    {
        if (size - offset < 2 + kHeaderSize + 16 || memcmp(data + offset, kMagic, 2) != 0)
            return false;
        ByteStream s(data + offset + 2, size - offset - 2);
        if (s.ReadU8() != 1)
            return false;
        qUnsignedInt8 optionsSize  = s.ReadU8();
        qUnsignedInt16 payloadSize = s.ReadU16();
        out.sourcePort             = s.ReadU8();
        out.destinationPort        = s.ReadU8();
        qUnsignedInt16 typeFlags   = s.ReadU16();
        out.type                   = (PacketType) (typeFlags & 0xF);
        out.flags                  = typeFlags >> 4;
        out.sessionId              = s.ReadU8();
        out.substreamId            = s.ReadU8();
        out.sequenceId             = s.ReadU16();
        s.Read(out.signature, 16);
        if (s.GetRemaining() < (size_t) optionsSize + payloadSize)
            return false;

        ByteStream opts(s.GetCursorPtr(), optionsSize);
        while (opts.GetRemaining() >= 2)
        {
            qUnsignedInt8 id  = opts.ReadU8();
            qUnsignedInt8 len = opts.ReadU8();
            if (id == SupportedFunctions && len == 4)
            {
                qUnsignedInt32 v       = opts.ReadU32();
                out.minorVersion       = v & 0xFF;
                out.supportedFunctions = v >> 8;
            }
            else if (id == ConnectionSignature && len == 16)
            {
                opts.Read(out.connectionSignature, 16);
            }
            else if (id == FragmentId && len == 1)
            {
                out.fragmentId = opts.ReadU8();
            }
            else if (id == InitialSequenceId && len == 2)
            {
                out.initialUnreliableSequenceId = opts.ReadU16();
            }
            else if (id == MaxSubstreamId && len == 1)
            {
                out.maximumSubstreamId = opts.ReadU8();
            }
            else
            {
                qUnsignedInt8 skip[255];
                opts.Read(skip, len);
            }
        }
        if (!opts.IsValid())
            return false;

        out.payload = Buffer(s.GetCursorPtr() + optionsSize, payloadSize);
        offset += 2 + kHeaderSize + 16 + optionsSize + payloadSize;
        return true;
    }

    bool PRUDPMessageV1::VerifySignature(const PRUDPPacket &packet, const PRUDPSignatureContext &ctx)
    {
        qUnsignedInt8 expected[16];
        ComputeSignature(packet, ctx, expected);
        return memcmp(expected, packet.signature, 16) == 0;
    }

} // namespace nn::nex
