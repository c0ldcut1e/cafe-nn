/**
 * @file
 * @brief This is a ByteStream that also knows the NEX encodings for strings, buffers, lists and structures.
 */

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <nn/nex/core/ByteStream.h>
#include <nn/nex/core/StationURL.h>
#include <nn/nex/core/String.h>
#include <nn/nex/types.h>

namespace nn::nex
{

    /**
     * @brief This is a ByteStream that also knows the NEX encodings for strings, buffers, lists and structures.
     */
    class Message : public ByteStream
    {
    public:
        using ByteStream::ByteStream;

        /**
         * @brief We write a placeholder size and return its offset, since the size is only known after the content.
         *
         * @param version The structure version, which only some servers check.
         * @return The offset to pass to the matching size patch.
         */
        size_t BeginStructure(qUnsignedInt8 version = 0);
        void EndStructure(size_t start);

        /**
         * @brief The returned end lets the caller skip fields a newer server appended, so newer versions still parse.
         *
         * @param end Receives where the structure ends, so the caller can skip fields it doesn't know.
         * @param version Receives the structure version.
         * @return Whether the header was valid.
         */
        bool BeginExtractStructure(size_t *end, qUnsignedInt8 *version = nullptr);
        void EndExtractStructure(size_t end)
        {
            SetCursor(end);
        }

        void AddStationURL(const StationURL &url)
        {
            WriteString(url.GetURL());
        }
        StationURL ExtractStationURL()
        {
            return StationURL(ReadString());
        }

        template<typename C, typename F>
        void AddList(const C &list, F add)
        {
            WriteU32((qUnsignedInt32) list.size());
            for (const auto &e : list) add(*this, e);
        }

        template<typename C, typename F>
        bool ExtractList(C *out, F extract)
        {
            qUnsignedInt32 count = ReadU32();
            // Every element takes at least one byte, so a larger count can only come from a corrupt message
            if (!IsValid() || count > GetRemaining())
                return false;
            out->clear();
            for (qUnsignedInt32 i = 0; i < count && IsValid(); i++)
            {
                typename C::value_type e{};
                if (!extract(*this, e))
                    return false;
                out->push_back(std::move(e));
            }
            return IsValid();
        }
    };

    struct _Type_string
    {
        static void Add(Message *m, const String &s)
        {
            m->WriteString(s);
        }
        static bool Extract(Message *m, String *s)
        {
            *s = m->ReadString();
            return m->IsValid();
        }
    };

    struct _Type_uint32
    {
        using CPPType = qUnsignedInt32;
        static void Add(Message *m, qUnsignedInt32 v)
        {
            m->WriteU32(v);
        }
        static bool Extract(Message *m, qUnsignedInt32 *v)
        {
            *v = m->ReadU32();
            return m->IsValid();
        }
    };

    struct _Type_byte
    {
        using CPPType = qByte;
        static void Add(Message *m, qByte v)
        {
            m->WriteU8(v);
        }
        static bool Extract(Message *m, qByte *v)
        {
            *v = m->ReadU8();
            return m->IsValid();
        }
    };

    template<typename T>
    struct _Type_qlist
    {
        static void Add(Message *m, const qList<typename T::CPPType> &l)
        {
            m->AddList(l,
                       [](Message &s, const typename T::CPPType &e)
                       {
                           T::Add(&s, e);
                       });
        }
        static bool Extract(Message *m, qList<typename T::CPPType> *l)
        {
            return m->ExtractList(l,
                                  [](Message &s, typename T::CPPType &e)
                                  {
                                      return T::Extract(&s, &e);
                                  });
        }
    };

    template<typename T>
    struct _Type_qvector
    {
        static void Add(Message *m, const qVector<typename T::CPPType> &v)
        {
            m->AddList(v,
                       [](Message &s, const typename T::CPPType &e)
                       {
                           T::Add(&s, e);
                       });
        }
        static bool Extract(Message *m, qVector<typename T::CPPType> *v)
        {
            return m->ExtractList(v,
                                  [](Message &s, typename T::CPPType &e)
                                  {
                                      return T::Extract(&s, &e);
                                  });
        }
    };

} // namespace nn::nex
