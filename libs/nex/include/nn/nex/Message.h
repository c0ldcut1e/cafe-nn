/**
 * @file
 * @brief This is RMC serialization: Message, the type traits that stream each wire type, and the Data class hierarchy that AnyDataHolder carries by name.
 */

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <nn/nex/ByteStream.h>
#include <nn/nex/StationURL.h>

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

    class Protocol
    {
    public:
        static void AddMethodID(Message *message, qUnsignedInt32 methodId)
        {
            message->WriteU32(methodId);
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

    /**
     * @brief This is the base of every class a holder can carry, and the type name is how the receiver picks the class.
     */
    class Data
    {
    public:
        Data()          = default;
        virtual ~Data() = default;
        virtual const char *GetDataTypeName() const
        {
            return "Data";
        }
        virtual Data *Clone() const
        {
            return new Data(*this);
        }
        virtual void StreamOut(Message &m) const
        {
            (void) m;
        }
        virtual bool StreamIn(Message &m)
        {
            return m.IsValid();
        }
    };

    /**
     * @brief We look classes up by name here because there is no RTTI on the console build.
     */
    class DataFactory
    {
    public:
        using Creator = Data *(*)();
        static void Register(const char *name, Creator creator);
        static Data *Create(const String &name);
    };

    template<typename T>
    struct DataFactoryRegistration
    {
        explicit DataFactoryRegistration(const char *name)
        {
            DataFactory::Register(name,
                                  []() -> Data *
                                  {
                                      return new T();
                                  });
        }
    };

    /**
     * @brief This carries any Data subclass on the wire as its type name followed by the object, and builds the right class through DataFactory on the way in.
     */
    class AnyDataHolder
    {
    public:
        AnyDataHolder() = default;
        explicit AnyDataHolder(const Data &data) : m_data(data.Clone())
        {}
        AnyDataHolder(const AnyDataHolder &o) : m_data(o.m_data ? o.m_data->Clone() : nullptr)
        {}
        AnyDataHolder &operator=(const AnyDataHolder &o)
        {
            if (this != &o)
                m_data.reset(o.m_data ? o.m_data->Clone() : nullptr);
            return *this;
        }

        Data *Get() const
        {
            return m_data.get();
        }
        void Set(const Data &data)
        {
            m_data.reset(data.Clone());
        }
        void Add(Message &m) const;
        bool Extract(Message &m);

    private:
        std::unique_ptr<Data> m_data;
    };

    /**
     * @brief Nintendo spells the holder for one class family this way, and the second argument is always the name type.
     */
    template<typename T, typename N = String>
    class AnyObjectHolder
    {
    public:
        AnyObjectHolder() = default;
        AnyObjectHolder(const T &object) : m_object((T *) object.Clone())
        {}
        AnyObjectHolder(const AnyObjectHolder &o) : m_object(o.m_object ? (T *) o.m_object->Clone() : nullptr)
        {}
        AnyObjectHolder &operator=(const AnyObjectHolder &o)
        {
            if (this != &o)
                m_object.reset(o.m_object ? (T *) o.m_object->Clone() : nullptr);
            return *this;
        }

        T *Get() const
        {
            return m_object.get();
        }
        T *operator->() const
        {
            return m_object.get();
        }
        bool IsValid() const
        {
            return m_object != nullptr;
        }
        void Set(const T &object)
        {
            m_object.reset((T *) object.Clone());
        }

        void Add(Message &m) const
        {
            AnyDataHolder holder;
            if (m_object)
                holder.Set(*m_object);
            holder.Add(m);
        }

        bool Extract(Message &m)
        {
            AnyDataHolder holder;
            if (!holder.Extract(m))
                return false;
            // The factory only knows classes derived from T for this holder, so the cast is to a known subclass
            m_object.reset(holder.Get() ? (T *) holder.Get()->Clone() : nullptr);
            return m_object != nullptr;
        }

    private:
        std::unique_ptr<T> m_object;
    };

    /**
     * @brief This is the paging window that list calls take, an offset and a maximum count.
     */
    class ResultRange
    {
    public:
        ResultRange(qUnsignedInt32 offset = 0, qUnsignedInt32 size = 10) : m_offset(offset), m_size(size)
        {}
        qUnsignedInt32 GetOffset() const
        {
            return m_offset;
        }
        qUnsignedInt32 GetSize() const
        {
            return m_size;
        }
        void SetOffset(qUnsignedInt32 offset)
        {
            m_offset = offset;
        }
        void SetSize(qUnsignedInt32 size)
        {
            m_size = size;
        }
        void Add(Message &m) const;

    private:
        qUnsignedInt32 m_offset;
        qUnsignedInt32 m_size;
    };

    /**
     * @brief This holds one value of a few wire types, as MatchmakeParam stores them.
     */
    class Variant
    {
    public:
        enum Type
        {
            None         = 0,
            Int64        = 1,
            Double       = 2,
            Bool         = 3,
            StringType   = 4,
            DateTimeType = 5,
            UInt64       = 6
        };

        Variant() = default;
        Variant(const wchar_t *s) : m_type(StringType), m_string(s)
        {}
        Variant(const String &s) : m_type(StringType), m_string(s)
        {}
        Variant(qInt64 v) : m_type(Int64), m_int(v)
        {}
        Variant(qUnsignedInt32 v) : m_type(UInt64), m_int((qInt64) v)
        {}
        Variant(qUnsignedInt64 v) : m_type(UInt64), m_int((qInt64) v)
        {}
        Variant(double v) : m_type(Double), m_double(v)
        {}
        Variant(bool v) : m_type(Bool), m_int(v)
        {}
        Variant(const DateTime &v) : m_type(DateTimeType), m_int((qInt64) v.GetValue())
        {}
        Variant(const Variant &)            = default;
        Variant &operator=(const Variant &) = default;
        ~Variant()                          = default;

        Type GetType() const
        {
            return m_type;
        }
        qUnsignedInt32 GetUInt32Value() const
        {
            return (qUnsignedInt32) m_int;
        }
        qInt64 GetInt64Value() const
        {
            return m_int;
        }
        qUnsignedInt64 GetUInt64Value() const
        {
            return (qUnsignedInt64) m_int;
        }
        double GetDoubleValue() const
        {
            return m_double;
        }
        bool GetBoolValue() const
        {
            return m_int != 0;
        }
        const String &GetStringValue() const
        {
            return m_string;
        }
        DateTime GetDateTimeValue() const
        {
            return DateTime((qUnsignedInt64) m_int);
        }

        void Add(Message &m) const;
        bool Extract(Message &m);

    private:
        Type m_type     = None;
        qInt64 m_int    = 0;
        double m_double = 0;
        String m_string;
    };

} // namespace nn::nex
