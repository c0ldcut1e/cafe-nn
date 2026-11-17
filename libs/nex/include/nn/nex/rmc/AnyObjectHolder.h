/**
 * @file
 * @brief Nintendo spells the holder for one class family this way, and the second argument is always the name type.
 */

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <nn/nex/core/String.h>
#include <nn/nex/rmc/AnyDataHolder.h>
#include <nn/nex/rmc/Message.h>

namespace nn::nex
{

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

} // namespace nn::nex
