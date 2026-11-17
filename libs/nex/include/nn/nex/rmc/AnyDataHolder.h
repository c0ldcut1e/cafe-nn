/**
 * @file
 * @brief This carries any Data subclass on the wire as its type name followed by the object, and builds the right class through DataFactory
 * on the way in.
 */

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <nn/nex/rmc/Data.h>
#include <nn/nex/rmc/Message.h>

namespace nn::nex
{

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

} // namespace nn::nex
