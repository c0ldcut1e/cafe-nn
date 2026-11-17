#include <algorithm>
#include <nn/nex/auth/NintendoLoginData.h>
#include <nn/nex/rmc/Message.h>

namespace nn::nex
{

    void NintendoLoginData::StreamOut(Message &m) const
    {
        size_t s = m.BeginStructure();
        m.WriteString(token);
        m.EndStructure(s);
    }

} // namespace nn::nex
