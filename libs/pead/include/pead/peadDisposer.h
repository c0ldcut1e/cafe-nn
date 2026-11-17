/**
 * @file
 * @brief This is the base for objects that a heap destroys along with itself, so nothing outlives the memory it sits in.
 */

#pragma once

#include <pead/peadList.h>

namespace pead
{
    class Heap;

    /**
     * @brief An IDisposer registers with the heap that holds it. When that heap is destroyed, it runs the object's destructor first.
     */
    class IDisposer
    {
    public:
        enum HeapNullOption
        {
            cHeapNullOption_AlwaysUseSpecifiedHeap               = 0,
            cHeapNullOption_UseSpecifiedOrContainHeap            = 1,
            cHeapNullOption_DoNotAppendDisposerIfNoHeapSpecified = 2,
            cHeapNullOption_UseSpecifiedOrCurrentHeap            = 3,
        };

        /**
         * @brief This registers with whichever heap holds this object, or with none when it is not inside a heap.
         */
        IDisposer();

        /**
         * @brief This registers with heap, and option says what to do when heap is null.
         *
         * @param heap The heap to register with.
         * @param option The fallback when heap is null.
         */
        IDisposer(Heap *heap, HeapNullOption option);
        virtual ~IDisposer();
        IDisposer(const IDisposer &)            = delete;
        IDisposer &operator=(const IDisposer &) = delete;

        static s32 getListNodeOffset();

    protected:
        Heap *getDisposerHeap_() const
        {
            return m_disposerHeap;
        }

    private:
        friend class Heap;

        Heap *m_disposerHeap;
        ListNode m_listNode;
    };

} // namespace pead
