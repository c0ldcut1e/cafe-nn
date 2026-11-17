/**
 * @file
 * @brief This is nn::crypto, which games use for SHA-1, MD5, AES with CTR and CCM modes, and random bytes. On Wii U the game links its
 * own copy of this library, so we build it everywhere and don't pass anything through to the system.
 */

#pragma once

#include <cstddef>
#include <cstdint>

namespace nn::crypto
{

    using qInt32         = int32_t;
    using qUnsignedInt32 = uint32_t;
    using qUnsignedInt64 = uint64_t;
    using qUnsignedInt8  = uint8_t;

    /**
     * @brief This is the interface every hash context shares, so code that only needs "some hash" can take any of them.
     */
    class HashContextBase
    {
    public:
        virtual ~HashContextBase() = default;

        /**
         * @brief Starts a new hash, dropping anything that was fed in before.
         */
        virtual void Initialize() = 0;

        /**
         * @brief Feeds more input into the hash. Calls after Finalize are ignored until the next Initialize.
         *
         * @param data The bytes to hash.
         * @param size How many bytes data holds.
         */
        virtual void Update(const void *data, size_t size) = 0;

        /**
         * @brief Pads the input and computes the digest. Calling it twice is harmless.
         */
        virtual void Finalize() = 0;

        /**
         * @brief Gets the digest size in bytes.
         *
         * @return 20 for SHA-1, 16 for MD5.
         */
        virtual size_t GetHashSize() const = 0;

        /**
         * @brief Finalizes if that hasn't happened yet and copies the digest out.
         *
         * @param out Receives GetHashSize() bytes.
         */
        virtual void GetHash(void *out) = 0;

    protected:
        virtual void ProcessBlock() = 0;
    };

    /**
     * @brief This holds the 64 byte block buffering and big endian length padding that SHA-1 uses.
     */
    class ShaBlock512BitContext : public HashContextBase
    {
    public:
        void Update(const void *data, size_t size) override;

    protected:
        static constexpr size_t BlockSize = 64;

        void AddPadding();

        /**
         * @brief Resets the buffering to resume a hash whose earlier blocks were already processed.
         *
         * @param length How many bytes the saved state covers. We only keep whole blocks, so the remainder must be fed in again.
         */
        void InitializeWithContext(qUnsignedInt64 length);

        qUnsignedInt8 m_block[BlockSize] = {};
        size_t m_blockUsed               = 0;
        qUnsignedInt64 m_length          = 0;
        bool m_finalized                 = false;
    };

    /**
     * @brief This is SHA-1 as FIPS 180-4 defines it.
     */
    class Sha1Context : public ShaBlock512BitContext
    {
    public:
        static constexpr size_t HashSize = 20;

        Sha1Context()
        {
            Initialize();
        }

        void Initialize() override;
        void Finalize() override;
        size_t GetHashSize() const override
        {
            return HashSize;
        }
        void GetHash(void *out) override;

        /**
         * @brief Resumes a hash from a saved intermediate state, such as one a save file stores to avoid rehashing a large prefix.
         *
         * @param state The five chaining words, in host order.
         * @param length How many bytes the state covers. Anything past the last whole block is dropped.
         */
        void InitializeWithContext(const void *state, qUnsignedInt64 length);

    protected:
        void ProcessBlock() override;

    private:
        qUnsignedInt32 m_state[5]      = {};
        qUnsignedInt8 m_hash[HashSize] = {};
    };

    /**
     * @brief This is MD5 as RFC 1321 defines it.
     */
    class Md5Context : public HashContextBase
    {
    public:
        static constexpr size_t HashSize  = 16;
        static constexpr size_t BlockSize = 64;

        Md5Context()
        {
            Initialize();
        }

        void Initialize() override;
        void Update(const void *data, size_t size) override;
        void Finalize() override;
        size_t GetHashSize() const override
        {
            return HashSize;
        }
        void GetHash(void *out) override;

        /**
         * @brief Gets how many bytes were fed in so far.
         *
         * @return The input length in bytes.
         */
        qUnsignedInt64 GetHashLength() const
        {
            return m_length;
        }

    protected:
        void ProcessBlock() override;

    private:
        qUnsignedInt32 m_state[4]        = {};
        qUnsignedInt8 m_block[BlockSize] = {};
        size_t m_blockUsed               = 0;
        qUnsignedInt64 m_length          = 0;
        qUnsignedInt8 m_hash[HashSize]   = {};
        bool m_finalized                 = false;
    };

    /**
     * @brief Hashes a buffer with SHA-1 in one call.
     *
     * @param out Receives 20 bytes.
     * @param data The bytes to hash.
     * @param size How many bytes data holds.
     */
    void CalculateSha1(void *out, const void *data, size_t size);

    /**
     * @brief Hashes a buffer with MD5 in one call.
     *
     * @param out Receives 16 bytes.
     * @param data The bytes to hash.
     * @param size How many bytes data holds.
     */
    void CalculateMd5(void *out, const void *data, size_t size);

    /**
     * @brief Fills a buffer with random bytes. On Linux and Windows these come from the OS. On Wii U we stretch a seed from the system
     * clocks through a SHA-1 based generator.
     *
     * @param out The buffer to fill.
     * @param size How many bytes to write.
     */
    void GenerateRandomBytes(void *out, size_t size);

    namespace detail
    {
        void GenerateRandomBytesImpl(void *out, size_t size);

        /**
         * @brief Collects seed material for the generator.
         *
         * @param out Receives the seed.
         * @param size How many seed bytes to collect.
         */
        void GetSeed(qUnsignedInt8 *out, size_t size);
    } // namespace detail

    /**
     * @brief This is a block cipher that the cipher modes drive one block at a time.
     */
    class BlockCipher
    {
    public:
        virtual ~BlockCipher() = default;

        virtual size_t GetBlockSize() const = 0;
        virtual size_t GetKeySize() const   = 0;

        /**
         * @brief Expands a key for both directions.
         *
         * @param key The key bytes.
         * @param size Must equal GetKeySize().
         * @return false when the size is wrong, which leaves the old key in place.
         */
        virtual bool SetKey(const void *key, size_t size) = 0;

        /**
         * @brief Encrypts one block. dst and src may be the same buffer.
         *
         * @param dst Receives GetBlockSize() bytes.
         * @param src GetBlockSize() bytes of plaintext.
         */
        virtual void Encrypt(void *dst, const void *src) const = 0;

        /**
         * @brief Decrypts one block. dst and src may be the same buffer.
         *
         * @param dst Receives GetBlockSize() bytes.
         * @param src GetBlockSize() bytes of ciphertext.
         */
        virtual void Decrypt(void *dst, const void *src) const = 0;
    };

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
         * @param iv The initial counter or vector.
         * @param ivSize Must equal GetIvSize().
         * @return false when the sizes don't fit.
         */
        virtual bool Initialize(const BlockCipher *cipher, const void *iv, size_t ivSize) = 0;

        /**
         * @brief Wipes the key stream and drops the cipher.
         */
        virtual void Finalize() = 0;
    };

    /**
     * @brief This is a cipher mode that encrypts, with a one shot helper on top of the streaming calls.
     */
    class Encryptor : public CipherMode
    {
    public:
        /**
         * @brief Processes more input. A stream mode writes as many bytes as it reads.
         *
         * @param dst Receives the output.
         * @param dstSize Room in dst, which has to be at least srcSize.
         * @param src The input. It may be the same buffer as dst.
         * @param srcSize How many bytes src holds.
         * @return How many bytes were written, or 0 when dst is too small.
         */
        virtual size_t Update(void *dst, size_t dstSize, const void *src, size_t srcSize) = 0;

        /**
         * @brief Flushes whatever the mode held back.
         *
         * @param dst Receives the output.
         * @param dstSize Room in dst.
         * @return How many bytes were written.
         */
        virtual size_t UpdateFinal(void *dst, size_t dstSize) = 0;

        /**
         * @brief Runs Initialize, Update, UpdateFinal and Finalize in one go.
         *
         * @return How many bytes were written, or 0 when anything didn't fit.
         */
        size_t Encrypt(void *dst, size_t dstSize, const void *src, size_t srcSize, const void *iv, size_t ivSize, const BlockCipher *cipher);
    };

    /**
     * @brief This is a cipher mode that decrypts. The streaming calls mean the same as Encryptor's.
     */
    class Decryptor : public CipherMode
    {
    public:
        virtual size_t Update(void *dst, size_t dstSize, const void *src, size_t srcSize) = 0;
        virtual size_t UpdateFinal(void *dst, size_t dstSize)                             = 0;

        /**
         * @brief Runs Initialize, Update, UpdateFinal and Finalize in one go.
         *
         * @return How many bytes were written, or 0 when anything didn't fit.
         */
        size_t Decrypt(void *dst, size_t dstSize, const void *src, size_t srcSize, const void *iv, size_t ivSize, const BlockCipher *cipher);
    };

    namespace detail
    {
        /**
         * @brief This is counter mode as SP 800-38A defines it, with the whole block as a big endian counter.
         */
        template<size_t BlockSize>
        class CtrMode
        {
        public:
            bool Initialize(const BlockCipher *cipher, const void *iv, size_t ivSize);
            void Finalize();
            void ProcessBlocks(qUnsignedInt8 *dst, const qUnsignedInt8 *src, size_t size);

            static void IncrementCounter(qUnsignedInt8 *counter);

        private:
            const BlockCipher *m_cipher          = nullptr;
            qUnsignedInt8 m_counter[BlockSize]   = {};
            qUnsignedInt8 m_keyStream[BlockSize] = {};
            size_t m_keyStreamUsed               = BlockSize;
        };
    } // namespace detail

    /**
     * @brief This is CTR mode encryption. It is a stream mode, so Update writes every byte straight away and UpdateFinal writes nothing.
     */
    template<size_t BlockSize>
    class CtrEncryptor : public Encryptor
    {
    public:
        ~CtrEncryptor() override
        {
            Finalize();
        }

        size_t GetIvSize() const override
        {
            return BlockSize;
        }
        size_t GetUnitSize() const override
        {
            return BlockSize;
        }
        bool Initialize(const BlockCipher *cipher, const void *iv, size_t ivSize) override
        {
            return m_mode.Initialize(cipher, iv, ivSize);
        }
        void Finalize() override
        {
            m_mode.Finalize();
        }
        size_t Update(void *dst, size_t dstSize, const void *src, size_t srcSize) override;
        size_t UpdateFinal(void *, size_t) override
        {
            return 0;
        }

    private:
        detail::CtrMode<BlockSize> m_mode;
    };

    /**
     * @brief This is CTR mode decryption, which is the same operation as encryption.
     */
    template<size_t BlockSize>
    class CtrDecryptor : public Decryptor
    {
    public:
        ~CtrDecryptor() override
        {
            Finalize();
        }

        size_t GetIvSize() const override
        {
            return BlockSize;
        }
        size_t GetUnitSize() const override
        {
            return BlockSize;
        }
        bool Initialize(const BlockCipher *cipher, const void *iv, size_t ivSize) override
        {
            return m_mode.Initialize(cipher, iv, ivSize);
        }
        void Finalize() override
        {
            m_mode.Finalize();
        }
        size_t Update(void *dst, size_t dstSize, const void *src, size_t srcSize) override;
        size_t UpdateFinal(void *, size_t) override
        {
            return 0;
        }

    private:
        detail::CtrMode<BlockSize> m_mode;
    };

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
         * @param src The plaintext.
         * @param srcSize How many bytes src holds.
         * @param adata Associated data that is authenticated but not encrypted.
         * @param adataSize How many bytes adata holds.
         * @param macSize Room in mac, which has to equal GetMacSize().
         * @return false when anything didn't fit the sizes given to Initialize.
         */
        bool EncryptAndGenerate(void *dst, void *mac, size_t dstSize, const void *src, size_t srcSize, const void *adata, size_t adataSize,
                                size_t macSize);
    };

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
         * @param src The ciphertext.
         * @param srcSize How many bytes src holds.
         * @param adata Associated data that was authenticated with it.
         * @param adataSize How many bytes adata holds.
         * @param mac The received tag.
         * @param macSize Must equal GetMacSize().
         * @return true only when the tag matched.
         */
        bool DecryptAndVerify(void *dst, size_t dstSize, const void *src, size_t srcSize, const void *adata, size_t adataSize, const void *mac,
                              size_t macSize);
    };

    namespace detail
    {
        /**
         * @brief This is the CBC-MAC and counter logic of CCM as SP 800-38C defines it, which both directions share.
         */
        class CcmMode
        {
        public:
            static constexpr size_t BlockSize = 16;

            bool Initialize(const BlockCipher *cipher, const void *nonce, size_t nonceSize, qUnsignedInt64 adataSize, qUnsignedInt64 dataSize,
                            size_t macSize);
            void Finalize();

            void UpdateAdata(const void *data, size_t size);
            void UpdateAdataFinal();

            // encrypt says whether src is plaintext, since the MAC always covers the plaintext
            size_t UpdateData(void *dst, size_t dstSize, const void *src, size_t srcSize, bool encrypt);
            void UpdateDataFinal();
            bool GenerateMac(void *mac, size_t macSize);

            size_t GetIvSize() const
            {
                return m_nonceSize;
            }
            size_t GetMacSize() const
            {
                return m_macSize;
            }

        private:
            enum class State
            {
                None,
                Adata,
                Data,
                Done,
            };

            void Absorb(const qUnsignedInt8 *data, size_t size);
            void Pad();

            const BlockCipher *m_cipher          = nullptr;
            State m_state                        = State::None;
            qUnsignedInt8 m_mac[BlockSize]       = {};
            size_t m_macUsed                     = 0;
            qUnsignedInt8 m_counter[BlockSize]   = {};
            qUnsignedInt8 m_keyStream[BlockSize] = {};
            size_t m_keyStreamUsed               = BlockSize;
            qUnsignedInt8 m_tagMask[BlockSize]   = {};
            qUnsignedInt64 m_adataLeft           = 0;
            qUnsignedInt64 m_dataLeft            = 0;
            size_t m_nonceSize                   = 0;
            size_t m_macSize                     = 0;
        };
    } // namespace detail

    /**
     * @brief This is CCM encryption over a 16 byte block cipher.
     */
    class CcmEncryptor : public AuthenticatedEncryptor
    {
    public:
        ~CcmEncryptor() override
        {
            Finalize();
        }

        /**
         * @brief Binds a cipher and nonce. CCM needs every length up front because they go into the first MAC block.
         *
         * @param cipher A cipher with 16 byte blocks that outlives this mode.
         * @param nonce The nonce, which must never repeat under one key.
         * @param nonceSize 7 to 13 bytes. Shorter nonces allow longer messages.
         * @param adataSize The exact associated data length that will follow.
         * @param pdataSize The exact plaintext length that will follow.
         * @param macSize An even tag length from 4 to 16.
         * @return false when any size is out of range.
         */
        bool Initialize(const BlockCipher *cipher, const void *nonce, size_t nonceSize, qUnsignedInt64 adataSize, qUnsignedInt64 pdataSize,
                        size_t macSize)
        {
            return m_mode.Initialize(cipher, nonce, nonceSize, adataSize, pdataSize, macSize);
        }

        size_t GetIvSize() const override
        {
            return m_mode.GetIvSize();
        }
        size_t GetUnitSize() const override
        {
            return detail::CcmMode::BlockSize;
        }
        size_t GetMacSize() const override
        {
            return m_mode.GetMacSize();
        }
        void Finalize() override
        {
            m_mode.Finalize();
        }
        void UpdateAdata(const void *data, size_t size) override
        {
            m_mode.UpdateAdata(data, size);
        }
        void UpdateAdataFinal() override
        {
            m_mode.UpdateAdataFinal();
        }
        size_t UpdatePdata(void *dst, size_t dstSize, const void *src, size_t srcSize) override
        {
            return m_mode.UpdateData(dst, dstSize, src, srcSize, true);
        }
        size_t UpdatePdataFinal(void *, size_t) override
        {
            m_mode.UpdateDataFinal();
            return 0;
        }
        bool GenerateMac(void *mac, size_t macSize) override
        {
            return m_mode.GenerateMac(mac, macSize);
        }

    private:
        detail::CcmMode m_mode;
    };

    /**
     * @brief This is CCM decryption over a 16 byte block cipher. Initialize takes the same arguments as CcmEncryptor's, with the
     * ciphertext length in place of the plaintext length.
     */
    class CcmDecryptor : public AuthenticatedDecryptor
    {
    public:
        ~CcmDecryptor() override
        {
            Finalize();
        }

        bool Initialize(const BlockCipher *cipher, const void *nonce, size_t nonceSize, qUnsignedInt64 adataSize, qUnsignedInt64 cdataSize,
                        size_t macSize)
        {
            return m_mode.Initialize(cipher, nonce, nonceSize, adataSize, cdataSize, macSize);
        }

        size_t GetIvSize() const override
        {
            return m_mode.GetIvSize();
        }
        size_t GetUnitSize() const override
        {
            return detail::CcmMode::BlockSize;
        }
        size_t GetMacSize() const override
        {
            return m_mode.GetMacSize();
        }
        void Finalize() override
        {
            m_mode.Finalize();
        }
        void UpdateAdata(const void *data, size_t size) override
        {
            m_mode.UpdateAdata(data, size);
        }
        void UpdateAdataFinal() override
        {
            m_mode.UpdateAdataFinal();
        }
        size_t UpdateCdata(void *dst, size_t dstSize, const void *src, size_t srcSize) override
        {
            return m_mode.UpdateData(dst, dstSize, src, srcSize, false);
        }
        size_t UpdateCdataFinal(void *, size_t) override
        {
            m_mode.UpdateDataFinal();
            return 0;
        }
        bool GenerateMac(void *mac, size_t macSize) override
        {
            return m_mode.GenerateMac(mac, macSize);
        }

    private:
        detail::CcmMode m_mode;
    };

} // namespace nn::crypto
