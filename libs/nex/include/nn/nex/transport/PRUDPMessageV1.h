/**
 * @file
 * @brief A single UDP datagram can carry several packets back to back, so this advances offset past the one it reads.
 */

#pragma once

#include <nn/nex/core/Buffer.h>
#include <nn/nex/transport/PRUDPPacket.h>
#include <nn/nex/transport/PRUDPSignatureContext.h>
#include <nn/nex/types.h>

namespace nn::nex
{

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
