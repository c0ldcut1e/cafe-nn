#include <pead/peadCriticalSection.h>

namespace pead
{

    namespace detail
    {

#ifdef __WIIU__
        RecursiveLock::RecursiveLock()
        {
            OSInitMutex(&m_mutex);
        }

        void RecursiveLock::lock()
        {
            OSLockMutex(&m_mutex);
        }

        bool RecursiveLock::tryLock()
        {
            return OSTryLockMutex(&m_mutex);
        }

        void RecursiveLock::unlock()
        {
            OSUnlockMutex(&m_mutex);
        }
#else
        RecursiveLock::RecursiveLock() = default;

        void RecursiveLock::lock()
        {
            m_mutex.lock();
        }

        bool RecursiveLock::tryLock()
        {
            return m_mutex.try_lock();
        }

        void RecursiveLock::unlock()
        {
            m_mutex.unlock();
        }
#endif

    } // namespace detail

    CriticalSection::CriticalSection() : IDisposer()
    {}

    CriticalSection::CriticalSection(Heap *heap) : IDisposer(heap, cHeapNullOption_UseSpecifiedOrContainHeap)
    {}

    Mutex::Mutex() : IDisposer()
    {}

    Mutex::Mutex(Heap *heap) : IDisposer(heap, cHeapNullOption_UseSpecifiedOrContainHeap)
    {}

} // namespace pead
