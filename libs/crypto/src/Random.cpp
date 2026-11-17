#include <cstdlib>
#include <cstring>
#include <mutex>
#include <nn/crypto/Random.h>

#if defined(__WIIU__)
#include <coreinit/time.h>
#elif defined(_WIN32)
#include <windows.h>
// bcrypt.h needs the types windows.h defines, so it comes second.
#include <bcrypt.h>
#else
#include <cerrno>
#include <cstdio>
#include <sys/random.h>
#endif

namespace nn::crypto
{

    namespace
    {

#if defined(__WIIU__)
        // The Wii U gives games no OS random source, so we hash a seed into a state and draw blocks as SHA-1(state || counter). Each
        // draw replaces the state with a different hash of it, so a leaked state doesn't reveal bytes handed out earlier.
        class RandomImpl
        {
        public:
            void InitializeState()
            {
                qUnsignedInt8 seed[64];
                detail::GetSeed(seed, sizeof seed);
                CalculateSha1(m_state, seed, sizeof seed);
                memset(seed, 0, sizeof seed);
                m_initialized = true;
            }

            void UpdateState()
            {
                Sha1Context context;
                qUnsignedInt8 tag = 0xff;
                context.Update(m_state, sizeof m_state);
                context.Update(&tag, 1);
                context.Update(&m_counter, sizeof m_counter);
                context.GetHash(m_state);
            }

            void GetRandom(void *out, size_t size)
            {
                if (!m_initialized)
                    InitializeState();

                // We mix the tick into every request so two consoles booted with the same clocks still drift apart
                OSTick tick = OSGetTick();
                Sha1Context mix;
                mix.Update(m_state, sizeof m_state);
                mix.Update(&tick, sizeof tick);
                mix.GetHash(m_state);

                qUnsignedInt8 *p = static_cast<qUnsignedInt8 *>(out);
                while (size > 0)
                {
                    qUnsignedInt8 block[Sha1Context::HashSize];
                    Sha1Context context;
                    context.Update(m_state, sizeof m_state);
                    context.Update(&m_counter, sizeof m_counter);
                    context.GetHash(block);
                    m_counter++;

                    size_t n = size < sizeof block ? size : sizeof block;
                    memcpy(p, block, n);
                    p += n;
                    size -= n;
                }
                UpdateState();
            }

        private:
            qUnsignedInt8 m_state[Sha1Context::HashSize] = {};
            qUnsignedInt64 m_counter                     = 0;
            bool m_initialized                           = false;
        };

        RandomImpl g_random;
#endif

        std::mutex g_mutex;

    } // namespace

    namespace detail
    {

        void GetSeed(qUnsignedInt8 *out, size_t size)
        {
#if defined(__WIIU__)
            // Each round hashes the clocks again, and the tick has moved on by the time it runs because the hashing takes time itself
            qUnsignedInt8 *p = out;
            while (size > 0)
            {
                struct
                {
                    OSTime time;
                    OSTime systemTime;
                    OSTick tick;
                    const void *where;
                } sample = {OSGetTime(), OSGetSystemTime(), OSGetTick(), p};
                qUnsignedInt8 hash[Sha1Context::HashSize];
                CalculateSha1(hash, &sample, sizeof sample);
                size_t n = size < sizeof hash ? size : sizeof hash;
                memcpy(p, hash, n);
                p += n;
                size -= n;
            }
#else
            GenerateRandomBytesImpl(out, size);
#endif
        }

        // Handing out predictable bytes would be worse than stopping, so we abort when the OS can't give us any
        void GenerateRandomBytesImpl(void *out, size_t size)
        {
#if defined(__WIIU__)
            g_random.GetRandom(out, size);
#elif defined(_WIN32)
            qUnsignedInt8 *p = static_cast<qUnsignedInt8 *>(out);
            while (size > 0)
            {
                ULONG n = size > 0x10000000 ? 0x10000000 : (ULONG) size;
                if (BCryptGenRandom(nullptr, p, n, BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0)
                    abort();
                p += n;
                size -= n;
            }
#else
            qUnsignedInt8 *p = static_cast<qUnsignedInt8 *>(out);
            while (size > 0)
            {
                ssize_t n = getrandom(p, size, 0);
                if (n < 0)
                {
                    if (errno == EINTR)
                        continue;
                    // Kernels older than 3.17 have no getrandom, so we read the device instead
                    FILE *f = fopen("/dev/urandom", "rb");
                    if (!f || fread(p, 1, size, f) != size)
                        abort();
                    fclose(f);
                    return;
                }
                p += n;
                size -= (size_t) n;
            }
#endif
        }

    } // namespace detail

    void GenerateRandomBytes(void *out, size_t size)
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        detail::GenerateRandomBytesImpl(out, size);
    }

} // namespace nn::crypto
