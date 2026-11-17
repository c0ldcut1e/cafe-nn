/**
 * @file
 * @brief This is a byte buffer that travels with a 32 bit length.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <list>
#include <nn/nex/types.h>
#include <string>
#include <vector>

namespace nn::nex
{

    /**
     * @brief This is a byte buffer that travels with a 32 bit length.
     */
    class Buffer
    {
    public:
        Buffer() = default;
        Buffer(const void *data, size_t size) : m_data((const qByte *) data, (const qByte *) data + size)
        {}

        const qByte *GetContentPtr() const
        {
            return m_data.data();
        }
        qByte *GetContentPtr()
        {
            return m_data.data();
        }
        size_t GetContentSize() const
        {
            return m_data.size();
        }
        void Resize(size_t size)
        {
            m_data.resize(size);
        }
        void Append(const void *data, size_t size)
        {
            m_data.insert(m_data.end(), (const qByte *) data, (const qByte *) data + size);
        }
        void Clear()
        {
            m_data.clear();
        }

    private:
        std::vector<qByte> m_data;
    };

    /**
     * @brief qBuffer differs from Buffer only on the wire, where it uses a 16 bit length.
     */
    class qBuffer : public Buffer
    {
    public:
        using Buffer::Buffer;
    };

} // namespace nn::nex
