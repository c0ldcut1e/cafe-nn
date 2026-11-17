/**
 * @file
 * @brief This is what every unauthenticated cipher mode shares.
 */

#pragma once

#include <cstddef>
#include <nn/crypto/BlockCipher.h>
#include <nn/crypto/types.h>

namespace nn::crypto
{

    /**
     * @brief This is what every unauthenticated cipher mode shares.
     */
    class CipherMode
    {
    public:
        virtual ~CipherMode() = default;

        virtual size_t GetIvSize() const   = 0;
        virtual size_t GetUnitSize() const = 0;

        /**
         * @brief Binds a cipher and IV. The cipher has to outlive this mode or the next Initialize.
         *
         * @param cipher A cipher whose block size matches the mode.
         * @param iv The starting counter for CTR, or the nonce for CCM.
         * @param ivSize Must equal GetIvSize().
         * @return false when the sizes don't fit.
         */
        virtual bool Initialize(const BlockCipher *cipher, const void *iv, size_t ivSize) = 0;

        /**
         * @brief Wipes the key stream and drops the cipher.
         */
        virtual void Finalize() = 0;
    };

} // namespace nn::crypto
