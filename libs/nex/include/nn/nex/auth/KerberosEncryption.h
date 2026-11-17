/**
 * @file
 * @brief NEX seals tickets with RC4 under the key and appends an HMAC-MD5 of the ciphertext under the same key.
 */

#pragma once

#include <nn/nex/core/Buffer.h>
#include <nn/nex/core/String.h>
#include <nn/nex/types.h>

namespace nn::nex
{

    /**
     * @brief NEX seals tickets with RC4 under the key and appends an HMAC-MD5 of the ciphertext under the same key.
     */
    class KerberosEncryption
    {
    public:
        KerberosEncryption(const void *key, size_t size) : m_key(key, size)
        {}
        explicit KerberosEncryption(const Buffer &key) : m_key(key)
        {}

        Buffer Encrypt(const Buffer &plain) const;
        bool Decrypt(const Buffer &sealed, Buffer *plain) const;

        /**
         * @brief The password key is MD5 applied repeatedly, and the count depends on the PID so keys differ per account.
         *
         * @param principalId The account's PID, which sets how many times we hash.
         * @param password The account's NEX password.
         * @return The 16 byte key.
         */
        static Buffer DeriveKey(qUnsignedInt32 principalId, const String &password);

    private:
        Buffer m_key;
    };

} // namespace nn::nex
