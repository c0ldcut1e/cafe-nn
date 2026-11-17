/**
 * @file
 * @brief This is an AEAD mode that encrypts.
 */

#pragma once

#include <cstddef>
#include <nn/crypto/types.h>

namespace nn::crypto
{

    /**
     * @brief This is an AEAD mode that encrypts. Callers go Initialize, UpdateAdata until done, UpdateAdataFinal, UpdatePdata until
     * done, UpdatePdataFinal, then GenerateMac.
     */
    class AuthenticatedEncryptor
    {
    public:
        virtual ~AuthenticatedEncryptor() = default;

        virtual size_t GetIvSize() const   = 0;
        virtual size_t GetUnitSize() const = 0;
        virtual size_t GetMacSize() const  = 0;
        virtual void Finalize()            = 0;

        virtual void UpdateAdata(const void *data, size_t size)                                = 0;
        virtual void UpdateAdataFinal()                                                        = 0;
        virtual size_t UpdatePdata(void *dst, size_t dstSize, const void *src, size_t srcSize) = 0;
        virtual size_t UpdatePdataFinal(void *dst, size_t dstSize)                             = 0;

        /**
         * @brief Writes the tag once all data went in.
         *
         * @param mac Receives the tag.
         * @param macSize Must equal GetMacSize().
         * @return false when called too early or with the wrong size.
         */
        virtual bool GenerateMac(void *mac, size_t macSize) = 0;

        /**
         * @brief Feeds all associated data and plaintext of an already initialized mode and writes the ciphertext and tag.
         *
         * @param dst Receives the ciphertext, which is as long as the plaintext.
         * @param mac Receives the tag.
         * @param dstSize Room in dst.
         * @param src The plaintext. It may be the same buffer as dst.
         * @param srcSize How many bytes src holds.
         * @param adata Associated data that is authenticated but not encrypted.
         * @param adataSize How many bytes adata holds.
         * @param macSize Room in mac, which has to equal GetMacSize().
         * @return false when anything didn't fit the sizes given to Initialize.
         */
        bool EncryptAndGenerate(void *dst, void *mac, size_t dstSize, const void *src, size_t srcSize, const void *adata, size_t adataSize,
                                size_t macSize);
    };

} // namespace nn::crypto
