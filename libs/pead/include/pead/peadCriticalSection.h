/**
 * @file
 * @brief These are pead's locks. CriticalSection and Mutex are both recursive, so the thread that holds one can lock it again.
 */

#pragma once

#include <pead/peadDisposer.h>

#ifdef __WIIU__
#include <coreinit/mutex.h>
#else
#include <mutex>
#endif

namespace pead
{
    namespace detail
    {
        // Heaps lock with this directly because a CriticalSection inside a heap would register as a disposer of its parent.
        class RecursiveLock
        {
        public:
            RecursiveLock();
            RecursiveLock(const RecursiveLock &)            = delete;
            RecursiveLock &operator=(const RecursiveLock &) = delete;

            void lock();
            bool tryLock();
            void unlock();

        private:
#ifdef __WIIU__
            OSMutex m_mutex;
#else
            std::recursive_mutex m_mutex;
#endif
        };
    } // namespace detail

    class CriticalSection : public IDisposer
    {
    public:
        CriticalSection();
        explicit CriticalSection(Heap *heap);

        void lock()
        {
            m_lock.lock();
        }
        bool tryLock()
        {
            return m_lock.tryLock();
        }
        void unlock()
        {
            m_lock.unlock();
        }

    private:
        detail::RecursiveLock m_lock;
    };

    class Mutex : public IDisposer
    {
    public:
        Mutex();
        explicit Mutex(Heap *heap);

        void lock()
        {
            m_lock.lock();
        }
        bool tryLock()
        {
            return m_lock.tryLock();
        }
        void unlock()
        {
            m_lock.unlock();
        }

    private:
        detail::RecursiveLock m_lock;
    };

    template<typename T>
    class ScopedLock
    {
    public:
        explicit ScopedLock(T *lockable) : m_lockable(lockable)
        {
            m_lockable->lock();
        }
        ~ScopedLock()
        {
            m_lockable->unlock();
        }
        ScopedLock(const ScopedLock &)            = delete;
        ScopedLock &operator=(const ScopedLock &) = delete;

    private:
        T *m_lockable;
    };

    /**
     * @brief This locks only when doLock is true, for code that is sometimes called with the lock already held.
     */
    template<typename T>
    class ConditionalScopedLock
    {
    public:
        ConditionalScopedLock(T *lockable, bool doLock) : m_lockable(doLock ? lockable : nullptr)
        {
            if (m_lockable)
                m_lockable->lock();
        }
        ~ConditionalScopedLock()
        {
            if (m_lockable)
                m_lockable->unlock();
        }
        ConditionalScopedLock(const ConditionalScopedLock &)            = delete;
        ConditionalScopedLock &operator=(const ConditionalScopedLock &) = delete;

    private:
        T *m_lockable;
    };

} // namespace pead
