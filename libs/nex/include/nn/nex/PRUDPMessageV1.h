/**
 * @file
 * @brief This encodes and decodes PRUDP v1 packets and computes their HMAC signatures.
 */

#pragma once

#include <nn/nex/types.h>

namespace nn::nex
{

    enum class PacketType : qUnsignedInt8
    {
        Syn        = 0,
        Connect    = 1,
        Data       = 2,
        Disconnect = 3,
        Ping       = 4
    };

    namespace PacketFlag
    {
        inline constexpr qUnsignedInt16 Ack      = 0x001;
        inline constexpr qUnsignedInt16 Reliable = 0x002;
        inline constexpr qUnsignedInt16 NeedAck  = 0x004;
        inline constexpr qUnsignedInt16 HasSize  = 0x008;
        inline constexpr qUnsignedInt16 MultiAck = 0x200;
    } // namespace PacketFlag

    /**
     * @brief This is one decoded PRUDP packet. The payload is still encrypted until the endpoint decrypts it.
     */
    struct PRUDPPacket
    {
        qUnsignedInt8 sourcePort      = 0;
        qUnsignedInt8 destinationPort = 0;
        PacketType type               = PacketType::Syn;
        qUnsignedInt16 flags          = 0;
        qUnsignedInt8 sessionId       = 0;
        qUnsignedInt8 substreamId     = 0;
        qUnsignedInt16 sequenceId     = 0;
        qUnsignedInt8 signature[16]   = {};

        qUnsignedInt32 minorVersion                = 0;
        qUnsignedInt32 supportedFunctions          = 0;
        qUnsignedInt8 connectionSignature[16]      = {};
        qUnsignedInt8 maximumSubstreamId           = 0;
        qUnsignedInt16 initialUnreliableSequenceId = 0;
        qUnsignedInt8 fragmentId                   = 0;

        Buffer payload;
    };

    /**
     * @brief This holds what both peers derive the per-packet HMAC from, and it changes once the secure session key is known.
     */
    struct PRUDPSignatureContext
    {
        qUnsignedInt8 accessKeyHash[16] = {};
        qUnsignedInt32 accessKeySum     = 0;
        Buffer sessionKey;

        /**
         * @brief Syn and Connect are signed with the peer's connection signature, so the caller fills this in per direction.
         */
        qUnsignedInt8 connectionSignature[16] = {};

        void SetAccessKey(const char *accessKey);
    };

    class PRUDPMessageV1
    {
    public:
        static Buffer Encode(PRUDPPacket &packet, const PRUDPSignatureContext &ctx);

        /**
         * @brief A single UDP datagram can carry several packets back to back, so this advances offset past the one it reads.
         *
         * @param data The whole datagram.
         * @param size The datagram's size.
         * @param offset Where the packet starts, and advanced past it on return.
         * @param out Receives the packet.
         * @return Whether a valid packet was read.
         */
        static bool Decode(const qUnsignedInt8 *data, size_t size, size_t &offset, PRUDPPacket &out);
        static bool VerifySignature(const PRUDPPacket &packet, const PRUDPSignatureContext &ctx);
        static void ComputeSignature(const PRUDPPacket &packet, const PRUDPSignatureContext &ctx, qUnsignedInt8 out[16]);

    private:
        static void EncodeHeader(const PRUDPPacket &packet, qUnsignedInt8 optionsSize, qUnsignedInt8 out[12]);
        static Buffer EncodeOptions(const PRUDPPacket &packet);
    };

} // namespace nn::nex
