/**
 * @file
 * @brief This is AES with 128, 192 and 256 bit keys.
 */

#pragma once

#include <cstddef>
#include <nn/crypto/BlockCipher.h>
#include <nn/crypto/types.h>

namespace nn::crypto
{

    namespace detail
    {
        template<size_t KeySize>
        class AesImpl
        {
        public:
            static constexpr size_t BlockSize = 16;
            static constexpr size_t Rounds    = KeySize / 4 + 6;

            void SetKey(const qUnsignedInt8 *key);
            void Encrypt(qUnsignedInt8 *dst, const qUnsignedInt8 *src) const;
            void Decrypt(qUnsignedInt8 *dst, const qUnsignedInt8 *src) const;

        private:
            static void MixColumns(qUnsignedInt8 *state);
            static void InvMixColumns(qUnsignedInt8 *state);

            qUnsignedInt8 m_roundKeys[(Rounds + 1) * BlockSize] = {};
        };
    } // namespace detail

    /**
     * @brief This is AES with a KeySize byte key: 16 for AES-128, 24 for AES-192, 32 for AES-256.
     */
    template<size_t KeySize>
    class Aes : public BlockCipher
    {
    public:
        static constexpr size_t BlockSize = 16;

        Aes() = default;
        ~Aes() override;

        size_t GetBlockSize() const override
        {
            return BlockSize;
        }
        size_t GetKeySize() const override
        {
            return KeySize;
        }
        bool SetKey(const void *key, size_t size) override;
        void Encrypt(void *dst, const void *src) const override;
        void Decrypt(void *dst, const void *src) const override;

    private:
        detail::AesImpl<KeySize> m_impl;
    };

    using Aes128 = Aes<16>;
    using Aes192 = Aes<24>;
    using Aes256 = Aes<32>;

} // namespace nn::crypto
