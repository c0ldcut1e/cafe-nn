#include <nn/nex/core/String.h>
#include <nn/nex/matchmaking/MatchmakeParam.h>
#include <nn/nex/rmc/Message.h>
#include <nn/nex/rmc/Variant.h>
#include <nn/nex/types.h>

namespace nn::nex
{

    void MatchmakeParam::StreamOut(Message &m) const
    {
        size_t s = m.BeginStructure();
        m.WriteU32((qUnsignedInt32) params.size());
        for (const auto &[key, value] : params)
        {
            m.WriteString(key);
            value.Add(m);
        }
        m.EndStructure(s);
    }

    bool MatchmakeParam::StreamIn(Message &m)
    {
        size_t end;
        if (!m.BeginExtractStructure(&end))
            return false;
        params.clear();
        qUnsignedInt32 count = m.ReadU32();
        for (qUnsignedInt32 i = 0; i < count && m.IsValid(); i++)
        {
            String key = m.ReadString();
            Variant value;
            if (!value.Extract(m))
                return false;
            params[key] = value;
        }
        m.EndExtractStructure(end);
        return m.IsValid();
    }

} // namespace nn::nex
