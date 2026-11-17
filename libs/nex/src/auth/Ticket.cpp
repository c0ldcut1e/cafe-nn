#include <nn/nex/auth/Ticket.h>
#include <nn/nex/core/Buffer.h>
#include <nn/nex/core/ByteStream.h>
#include <nn/nex/types.h>

namespace nn::nex
{

    bool Ticket::Parse(const Buffer &decrypted)
    {
        // The session key is always 32 bytes, and the target principal and internal ticket follow it.
        ByteStream s(decrypted.GetContentPtr(), decrypted.GetContentSize());
        qUnsignedInt8 key[32];
        s.Read(key, sizeof(key));
        qUnsignedInt32 target = s.ReadU32();
        Buffer ticket         = s.ReadBuffer();
        if (!s.IsValid())
            return false;
        sessionKey        = Buffer(key, sizeof(key));
        targetPrincipalId = target;
        internalTicket    = ticket;
        return true;
    }

} // namespace nn::nex
