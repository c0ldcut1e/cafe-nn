/**
 * @file
 * @brief This owns the root heap and knows every heap, so it can find which heap a pointer came from and which heap the calling thread allocates from.
 */

#pragma once

#include <pead/peadDelegate.h>
#include <pead/peadExpHeap.h>

namespace pead
{
    /**
     * @brief This is the memory the root heap is built in.
     */
    class Arena
    {
    public:
        Arena();
        ~Arena();
        Arena(const Arena &)            = delete;
        Arena &operator=(const Arena &) = delete;

        /**
         * @brief This allocates size bytes from the system.
         *
         * @param size The size in bytes.
         * @return False when the system had no room.
         */
        bool initialize(u32 size);

        /**
         * @brief This uses memory the caller owns, which has to outlive the arena.
         */
        void initialize(void *start, u32 size);
        void destroy();

        u8 *getStartAddress() const
        {
            return m_start;
        }
        u32 getSize() const
        {
            return m_size;
        }

    private:
        u8 *m_start;
        u32 m_size;
        bool m_ownsMemory;
    };

    class HeapMgr
    {
    public:
        struct AllocCallbackArg
        {
            Heap *heap;
            void *ptr;
            u32 requestSize;
            s32 alignment;
        };

        struct FreeCallbackArg
        {
            Heap *heap;
            void *ptr;
        };

        using IAllocCallback = IDelegate1<const AllocCallbackArg *>;
        using IFreeCallback  = IDelegate1<const FreeCallbackArg *>;

        HeapMgr();
        ~HeapMgr();

        /**
         * @brief This builds the root heap in arena. Call it once before any other pead allocation.
         *
         * @param arena Memory for the root heap, which has to outlive the heap manager.
         */
        static void initialize(Arena *arena);

        /**
         * @brief This destroys the root heap and with it every heap and disposer.
         */
        static void destroy();
        static bool isInitialized()
        {
            return s_rootHeap != nullptr;
        }
        static HeapMgr *instance()
        {
            return s_rootHeap ? &s_instance : nullptr;
        }
        static Heap *getRootHeap()
        {
            return s_rootHeap;
        }

        /**
         * @brief This finds the deepest heap that holds ptr.
         *
         * @param ptr The address to look up.
         * @return The heap, or null when no heap holds ptr.
         */
        Heap *findContainHeap(const void *ptr) const;
        static bool isContainedInAnyHeap(const void *ptr);

        /**
         * @brief This finds a heap by name, searching depth first from the root.
         *
         * @param name The name to look for.
         * @param index Which match to return, counting from 0.
         * @return The heap, or null.
         */
        static Heap *findHeapByName(const SafeString &name, s32 index = 0);

        /**
         * @brief This returns the heap the calling thread allocates from by default, which is the root heap for threads pead did not create.
         */
        Heap *getCurrentHeap() const;

        /**
         * @brief This changes the calling thread's current heap.
         *
         * @param heap The new current heap.
         * @return The previous current heap.
         */
        Heap *setCurrentHeap_(Heap *heap);

        IAllocCallback *setAllocCallback(IAllocCallback *callback);
        IFreeCallback *setFreeCallback(IFreeCallback *callback);
        IAllocCallback *getAllocCallback() const
        {
            return m_allocCallback;
        }
        IFreeCallback *getFreeCallback() const
        {
            return m_freeCallback;
        }

        static detail::RecursiveLock *getHeapTreeLock()
        {
            return &s_heapTreeLock;
        }

    private:
        static Heap *findHeapByName_(Heap *heap, const SafeString &name, s32 *index);

        IAllocCallback *m_allocCallback;
        IFreeCallback *m_freeCallback;

        static HeapMgr s_instance;
        static Heap *s_rootHeap;
        static Arena *s_arena;
        static detail::RecursiveLock s_heapTreeLock;
    };

    /**
     * @brief This makes heap the calling thread's current heap until it goes out of scope.
     */
    class CurrentHeapSetter
    {
    public:
        explicit CurrentHeapSetter(Heap *heap) : m_previous(HeapMgr::instance() ? HeapMgr::instance()->setCurrentHeap_(heap) : nullptr)
        {}
        ~CurrentHeapSetter()
        {
            if (HeapMgr::instance())
                HeapMgr::instance()->setCurrentHeap_(m_previous);
        }
        CurrentHeapSetter(const CurrentHeapSetter &)            = delete;
        CurrentHeapSetter &operator=(const CurrentHeapSetter &) = delete;

    private:
        Heap *m_previous;
    };

} // namespace pead
