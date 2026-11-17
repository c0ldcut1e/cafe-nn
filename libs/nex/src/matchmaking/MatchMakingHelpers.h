/**
 * @file
 * @brief These are the stream helpers the MatchMaking sources share. They are private to the library.
 */

#pragma once

#include <nn/nex/core/String.h>
#include <nn/nex/matchmaking/Gathering.h>
#include <nn/nex/rmc/AnyObjectHolder.h>
#include <nn/nex/rmc/Message.h>
#include <nn/nex/types.h>

namespace nn::nex
{

    namespace
    {

        void AddU32s(Message &m, const qVector<qUnsignedInt32> &v)
        {
            m.AddList(v,
                      [](Message &s, qUnsignedInt32 e)
                      {
                          s.WriteU32(e);
                      });
        }

        bool ExtractU32s(Message &m, qVector<qUnsignedInt32> *v)
        {
            return m.ExtractList(v,
                                 [](Message &s, qUnsignedInt32 &e)
                                 {
                                     e = s.ReadU32();
                                     return s.IsValid();
                                 });
        }

        void AddU32List(Message &m, const qList<qUnsignedInt32> &v)
        {
            m.AddList(v,
                      [](Message &s, qUnsignedInt32 e)
                      {
                          s.WriteU32(e);
                      });
        }

        bool ExtractGatherings(Message &m, qList<AnyObjectHolder<Gathering, String>> *out)
        {
            return m.ExtractList(out,
                                 [](Message &s, AnyObjectHolder<Gathering, String> &h)
                                 {
                                     return h.Extract(s);
                                 });
        }

    } // namespace

} // namespace nn::nex
