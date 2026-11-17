/**
 * @file
 * @brief This is the paging window that list calls take, an offset and a maximum count.
 */

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <nn/nex/rmc/Message.h>
#include <nn/nex/types.h>

namespace nn::nex
{

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

} // namespace nn::nex
