#include <algorithm>
#include <nn/nex/auth/AuthenticationInfo.h>
#include <nn/nex/rmc/Message.h>

namespace nn::nex
{

    void AuthenticationInfo::StreamOut(Message &m) const
    {
        // We write an empty header for the parent Data level because the server expects one per class level
        m.EndStructure(m.BeginStructure());
        size_t s = m.BeginStructure();
        m.WriteString(token);
        m.WriteU32(ngsVersion);
        m.WriteU8(tokenType);
        m.WriteU32(serverVersion);
        m.EndStructure(s);
    }

} // namespace nn::nex
