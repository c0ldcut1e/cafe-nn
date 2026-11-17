/**
 * @file
 * @brief This is RC4, which NEX uses for ticket sealing and PRUDP payloads.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <nn/nex/types.h>

namespace nn::nex
{

    /**
     * @brief This is RC4, which NEX uses for ticket sealing and PRUDP payloads.
     */
    class RC4
    {
    public:
        RC4() = default;
        RC4(const void *key, size_t size)
        {
            SetKey(key, size);
        }
        void SetKey(const void *key, size_t size);

        /**
         * @brief The keystream carries over between calls since each PRUDP stream is one continuous RC4 session.
         *
         * @param data Encrypted or decrypted in place.
         * @param size How many bytes to process.
         */
        void Process(void *data, size_t size);

    private:
        qUnsignedInt8 m_s[256] = {};
        qUnsignedInt8 m_i      = 0;
        qUnsignedInt8 m_j      = 0;
    };

} // namespace nn::nex
