/**
 * @file
 * @brief These allocate from pead heaps. We don't replace the global operator new, so anything made with new (heap) has to be destroyed with pead::Delete and not with delete.
 */

#pragma once

#include <cstddef>
#include <new>
#include <pead/peadTypes.h>

namespace pead
{
    class Heap;

    /**
     * @brief This allocates from the current heap, falling back to the root heap when the thread has none.
     *
     * @param size The size in bytes.
     * @param heap The heap to use, or null for the current one.
     * @param alignment The alignment in bytes. A negative value allocates from the back of the heap.
     * @return The block, or null when the heap is full.
     */
    void *allocateFromSeadHeap(u32 size);
    void *allocateFromSeadHeap(u32 size, Heap *heap);
    void *allocateFromSeadHeap(u32 size, Heap *heap, s32 alignment);

    /**
     * @brief This returns a block to whichever heap it came from.
     *
     * @param ptr The block, or null.
     */
    void freeToSeadHeap(void *ptr);

    /**
     * @brief This allocates an array of n values and records n in front of it, so getArraySize can read it back.
     *
     * @param n The element count.
     * @param heap The heap to use, or null for the current one.
     * @param alignment The alignment of the first element.
     * @return The first element, or null when the heap is full.
     */
    template<typename T>
    T *NewArray(u32 n, Heap *heap, s32 alignment = 8);
    template<typename T>
    void DeleteArray(T *array);
    u32 getArraySize(void *array);

    template<typename T>
    void Delete(T *obj)
    {
        if (obj)
        {
            obj->~T();
            freeToSeadHeap(obj);
        }
    }

} // namespace pead

// These are noexcept so a new expression checks for null and skips the constructor when the heap is full.
void *operator new(size_t size, pead::Heap *heap, pead::s32 alignment = 8) noexcept;
void *operator new[](size_t size, pead::Heap *heap, pead::s32 alignment = 8) noexcept;
void operator delete(void *ptr, pead::Heap *heap, pead::s32 alignment) noexcept;
void operator delete[](void *ptr, pead::Heap *heap, pead::s32 alignment) noexcept;

namespace pead
{
    namespace detail
    {
        // We keep the count in a header the size of the alignment, so the elements stay aligned.
        u32 getArrayHeaderSize(s32 alignment);
    } // namespace detail

    template<typename T>
    T *NewArray(u32 n, Heap *heap, s32 alignment)
    {
        u32 header = detail::getArrayHeaderSize(alignment);
        u8 *block  = (u8 *) allocateFromSeadHeap(header + n * (u32) sizeof(T), heap, alignment);
        if (!block)
            return nullptr;
        *(u32 *) (block + header - sizeof(u32))     = n;
        *(u32 *) (block + header - sizeof(u32) * 2) = header;
        T *array                                    = (T *) (block + header);
        for (u32 i = 0; i < n; i++) new (array + i) T();
        return array;
    }

    template<typename T>
    void DeleteArray(T *array)
    {
        if (!array)
            return;
        u32 n      = getArraySize(array);
        u32 header = *(u32 *) ((u8 *) array - sizeof(u32) * 2);
        for (u32 i = n; i > 0; i--) array[i - 1].~T();
        freeToSeadHeap((u8 *) array - header);
    }

} // namespace pead
