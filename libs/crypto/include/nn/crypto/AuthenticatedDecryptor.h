/**
 * @file
 * @brief This is an AEAD mode that decrypts.
 */

#pragma once

#include <cstddef>
#include <nn/crypto/types.h>

namespace nn::crypto
{

    /**
     * @brief This is an AEAD mode that decrypts. Callers go Initialize, UpdateAdata, UpdateAdataFinal, UpdateCdata, UpdateCdataFinal,
     * then compare GenerateMac against the received tag. Nothing decrypted should be trusted until that compare succeeds.
     */
    class AuthenticatedDecryptor
    {
    public:
        virtual ~AuthenticatedDecryptor() = default;

        virtual size_t GetIvSize() const   = 0;
        virtual size_t GetUnitSize() const = 0;
        virtual size_t GetMacSize() const  = 0;
        virtual void Finalize()            = 0;

        virtual void UpdateAdata(const void *data, size_t size)                                = 0;
        virtual void UpdateAdataFinal()                                                        = 0;
        virtual size_t UpdateCdata(void *dst, size_t dstSize, const void *src, size_t srcSize) = 0;
        virtual size_t UpdateCdataFinal(void *dst, size_t dstSize)                             = 0;
        virtual bool GenerateMac(void *mac, size_t macSize)                                    = 0;

        /**
         * @brief Decrypts all ciphertext of an already initialized mode and checks the tag in constant time.
         *
         * @param dst Receives the plaintext. It is zeroed when the tag doesn't match.
         * @param dstSize Room in dst.
         * @param src The ciphertext, without the tag.
         * @param srcSize How many bytes src holds.
         * @param adata Associated data that was authenticated with it.
         * @param adataSize How many bytes adata holds.
         * @param mac The tag that came with the ciphertext.
         * @param macSize Must equal GetMacSize().
         * @return true only when the tag matched.
         */
        bool DecryptAndVerify(void *dst, size_t dstSize, const void *src, size_t srcSize, const void *adata, size_t adataSize, const void *mac,
                              size_t macSize);
    };

} // namespace nn::crypto
