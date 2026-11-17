/**
 * @file
 * @brief These are the MD5 and RC4 primitives that Kerberos tickets and PRUDP packets are built on.
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
