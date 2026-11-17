/**
 * @file
 * @brief This is MD5, which NEX uses for Kerberos keys and HMACs.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <nn/nex/types.h>

namespace nn::nex
{

    /**
     * @brief This is MD5, which NEX uses for Kerberos keys and HMACs.
     */
    class MD5
    {
    public:
        MD5()
        {
            Reset();
        }
        void Reset();
        void Update(const void *data, size_t size);
        void Final(qUnsignedInt8 out[16]);
        static void Hash(const void *data, size_t size, qUnsignedInt8 out[16]);

    private:
        void Block(const qUnsignedInt8 *p);

        qUnsignedInt32 m_state[4];
        qUnsignedInt64 m_length;
        qUnsignedInt8 m_pending[64];
    };

    void HmacMD5(const void *key, size_t keySize, const void *data, size_t size, qUnsignedInt8 out[16]);

} // namespace nn::nex
