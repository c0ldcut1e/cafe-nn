#include <pead/peadHeapMgr.h>

namespace pead
{

    void *allocateFromSeadHeap(u32 size)
    {
        return allocateFromSeadHeap(size, nullptr, 8);
    }

    void *allocateFromSeadHeap(u32 size, Heap *heap)
    {
        return allocateFromSeadHeap(size, heap, 8);
    }

    void *allocateFromSeadHeap(u32 size, Heap *heap, s32 alignment)
    {
        if (!heap)
        {
            HeapMgr *mgr = HeapMgr::instance();
            if (!mgr)
                return nullptr;
            heap = mgr->getCurrentHeap();
        }
        return heap ? heap->tryAlloc(size, alignment) : nullptr;
    }

    void freeToSeadHeap(void *ptr)
    {
        if (!ptr)
            return;
        HeapMgr *mgr = HeapMgr::instance();
        Heap *heap   = mgr ? mgr->findContainHeap(ptr) : nullptr;
        if (heap)
            heap->free(ptr);
    }

    u32 getArraySize(void *array)
    {
        return *(u32 *) ((u8 *) array - sizeof(u32));
    }

    namespace detail
    {

        u32 getArrayHeaderSize(s32 alignment)
        {
            u32 a = (u32) (alignment < 0 ? -alignment : alignment);
            return a < 8 ? 8 : a;
        }

    } // namespace detail

} // namespace pead

void *operator new(size_t size, pead::Heap *heap, pead::s32 alignment) noexcept
{
    return pead::allocateFromSeadHeap((pead::u32) size, heap, alignment);
}

void *operator new[](size_t size, pead::Heap *heap, pead::s32 alignment) noexcept
{
    return pead::allocateFromSeadHeap((pead::u32) size, heap, alignment);
}

void operator delete(void *ptr, pead::Heap *, pead::s32) noexcept
{
    pead::freeToSeadHeap(ptr);
}

void operator delete[](void *ptr, pead::Heap *, pead::s32) noexcept
{
    pead::freeToSeadHeap(ptr);
}
