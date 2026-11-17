/**
 * @file
 * @brief This is one decoded PRUDP packet. The payload is still encrypted until the endpoint decrypts it.
 */

#pragma once

#include <nn/nex/core/Buffer.h>
#include <nn/nex/rmc/Data.h>
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

} // namespace nn::nex
