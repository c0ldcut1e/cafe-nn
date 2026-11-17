#include <cstdlib>
#include <pead/peadHeapMgr.h>
#include <pead/peadThread.h>

namespace pead
{

    namespace
    {

        // Threads pead did not create have no Thread object, so their current heap lives here.
        thread_local Heap *t_fallbackHeap = nullptr;

    } // namespace

    HeapMgr HeapMgr::s_instance;
    Heap *HeapMgr::s_rootHeap = nullptr;
    Arena *HeapMgr::s_arena   = nullptr;
    detail::RecursiveLock HeapMgr::s_heapTreeLock;

    Arena::Arena() : m_start(nullptr), m_size(0), m_ownsMemory(false)
    {}

    Arena::~Arena()
    {
        destroy();
    }

    bool Arena::initialize(u32 size)
    {
        destroy();
        m_start = (u8 *) malloc(size);
        if (!m_start)
            return false;
        m_size       = size;
        m_ownsMemory = true;
        return true;
    }

    void Arena::initialize(void *start, u32 size)
    {
        destroy();
        m_start      = (u8 *) start;
        m_size       = size;
        m_ownsMemory = false;
    }

    void Arena::destroy()
    {
        if (m_ownsMemory)
            ::free(m_start);
        m_start      = nullptr;
        m_size       = 0;
        m_ownsMemory = false;
    }

    HeapMgr::HeapMgr() : m_allocCallback(nullptr), m_freeCallback(nullptr)
    {}

    HeapMgr::~HeapMgr() = default;

    void HeapMgr::initialize(Arena *arena)
    {
        if (s_rootHeap || !arena || !arena->getStartAddress())
            return;
        s_arena    = arena;
        s_rootHeap = ExpHeap::create(arena->getStartAddress(), arena->getSize(), "RootHeap", true);
    }

    void HeapMgr::destroy()
    {
        if (!s_rootHeap)
            return;
        Heap *root = s_rootHeap;
        s_rootHeap = nullptr;
        root->destroy();
        s_arena        = nullptr;
        t_fallbackHeap = nullptr;
    }

    Heap *HeapMgr::findContainHeap(const void *ptr) const
    {
        if (!s_rootHeap)
            return nullptr;
        ScopedLock<detail::RecursiveLock> lock(&s_heapTreeLock);
        return s_rootHeap->findContainHeap_(ptr);
    }

    bool HeapMgr::isContainedInAnyHeap(const void *ptr)
    {
        return s_rootHeap && s_rootHeap->isInclude(ptr);
    }

    Heap *HeapMgr::findHeapByName(const SafeString &name, s32 index)
    {
        if (!s_rootHeap)
            return nullptr;
        ScopedLock<detail::RecursiveLock> lock(&s_heapTreeLock);
        return findHeapByName_(s_rootHeap, name, &index);
    }

    Heap *HeapMgr::findHeapByName_(Heap *heap, const SafeString &name, s32 *index)
    {
        if (heap->getName() == name)
        {
            if (*index == 0)
                return heap;
            (*index)--;
        }
        for (Heap &child : heap->m_children)
        {
            if (Heap *found = findHeapByName_(&child, name, index))
                return found;
        }
        return nullptr;
    }

    Heap *HeapMgr::getCurrentHeap() const
    {
        ThreadMgr *threads = ThreadMgr::instance();
        Thread *thread     = threads ? threads->getCurrentThread() : nullptr;
        if (thread && thread->getCurrentHeap())
            return thread->getCurrentHeap();
        return t_fallbackHeap ? t_fallbackHeap : s_rootHeap;
    }

    Heap *HeapMgr::setCurrentHeap_(Heap *heap)
    {
        ThreadMgr *threads = ThreadMgr::instance();
        Thread *thread     = threads ? threads->getCurrentThread() : nullptr;
        if (thread)
            return thread->setCurrentHeap_(heap);
        Heap *previous = getCurrentHeap();
        t_fallbackHeap = heap;
        return previous;
    }

    HeapMgr::IAllocCallback *HeapMgr::setAllocCallback(IAllocCallback *callback)
    {
        IAllocCallback *previous = m_allocCallback;
        m_allocCallback          = callback;
        return previous;
    }

    HeapMgr::IFreeCallback *HeapMgr::setFreeCallback(IFreeCallback *callback)
    {
        IFreeCallback *previous = m_freeCallback;
        m_freeCallback          = callback;
        return previous;
    }

} // namespace pead
