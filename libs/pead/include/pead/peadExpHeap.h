/**
 * @file
 * @brief This is the expanded heap, pead's general-purpose allocator. Every block has a small header, and blocks can be allocated from either end and freed in any order.
 */

#pragma once

#include <pead/peadHeap.h>

namespace pead
{
    /**
     * @brief This is the header in front of every block, both free and used.
     */
    struct MemBlock
    {
        static constexpr u16 cFreeSignature = 0x4652;
        static constexpr u16 cUseSignature  = 0x5544;

        u16 signature;
        // This is the gap between the block's region and its header, left over from aligning the payload.
        u16 alignmentPadding;
        u32 size;
        ListNode node;

        void *memory() const
        {
            return (u8 *) this + sizeof(MemBlock);
        }
        u8 *regionStart() const
        {
            return (u8 *) this - alignmentPadding;
        }
        u8 *regionEnd() const
        {
            return (u8 *) memory() + size;
        }
    };

    class ExpHeap : public Heap
    {
        PEAD_RTTI_OVERRIDE(ExpHeap, Heap)

    public:
        enum AllocMode
        {
            cAllocMode_FirstFit = 0,
            cAllocMode_BestFit  = 1,
        };

        enum FindMode
        {
            cFindMode_FirstFit = 0,
            cFindMode_BestFit  = 1,
        };

        // This is the smallest free block we split off, since a smaller one could never satisfy an allocation.
        static constexpr u32 cMinFreeBlockSize = 4;

        ExpHeap(const SafeString &name, Heap *parent, void *start, u32 size, HeapDirection direction, bool enableLock);
        ~ExpHeap() override;

        /**
         * @brief This carves a new heap out of parent.
         *
         * @param size The size in bytes, or 0 to take everything parent has left.
         * @param name The name shown in dumps. It is copied.
         * @param parent The heap to allocate from, or null for the current one.
         * @param direction The default allocation end.
         * @param enableLock True when more than one thread uses the heap.
         * @return The heap, or null when parent had no room.
         */
        static ExpHeap *create(u32 size, const SafeString &name, Heap *parent, HeapDirection direction, bool enableLock);
        static ExpHeap *tryCreate(u32 size, const SafeString &name, Heap *parent, HeapDirection direction, bool enableLock);

        /**
         * @brief This builds a root heap inside memory the caller owns. The heap object itself takes the first bytes.
         *
         * @param address The memory, which has to outlive the heap.
         * @param size The size of the memory in bytes.
         * @param name The name shown in dumps. It is copied.
         * @param enableLock True when more than one thread uses the heap.
         * @return The heap, or null when the memory is too small.
         */
        static ExpHeap *create(void *address, u32 size, const SafeString &name, bool enableLock);
        static ExpHeap *tryCreate(void *address, u32 size, const SafeString &name, bool enableLock);

        void destroy() override;
        u32 adjust() override;
        void *tryAlloc(u32 size, s32 alignment) override;
        void free(void *ptr) override;
        void *resizeFront(void *ptr, u32 newSize) override;
        u32 resizeBack(void *ptr, u32 newSize) override;
        void *tryRealloc(void *ptr, u32 newSize, s32 alignment) override;
        void freeAll() override;
        const void *getStartAddress() const override
        {
            return m_start;
        }
        const void *getEndAddress() const override
        {
            return (u8 *) m_start + m_size;
        }
        u32 getSize() const override
        {
            return m_size;
        }
        u32 getFreeSize() const override;
        u32 getMaxAllocatableSize(s32 alignment) const override;
        bool isInclude(const void *ptr) const override;
        bool isEmpty() const override
        {
            return m_useList.isEmpty();
        }
        bool isFreeable() const override
        {
            return true;
        }
        bool isResizable() const override
        {
            return true;
        }
        bool isAdjustable() const override
        {
            return true;
        }
        void dump() const override;
        void dumpYAML(WriteStream &stream, s32 indent) const override;
        void dumpFreeList() const;
        void dumpUseList() const;

        u32 getAllocatedSize(void *ptr);
        AllocMode getAllocMode() const
        {
            return m_allocMode;
        }
        void setAllocMode(AllocMode mode)
        {
            m_allocMode = mode;
        }
        s32 getUseCount() const
        {
            return m_useList.size();
        }
        s32 getFreeCount() const
        {
            return m_freeList.size();
        }

    private:
        static ExpHeap *doCreate(ExpHeap *heap, Heap *parent);
        static s32 compareMemBlockAddr_(const MemBlock *a, const MemBlock *b);

        void createMaxSizeFreeMemBlock_();
        void *allocFromHead_(u32 size, s32 alignment);
        void *allocFromTail_(u32 size, s32 alignment);
        MemBlock *findFreeMemBlockFromHead_(u32 size, s32 alignment, FindMode mode) const;
        MemBlock *findFreeMemBlockFromTail_(u32 size, s32 alignment, FindMode mode) const;
        void *allocFromFreeBlock_(MemBlock *freeBlock, u8 *memory, u32 size);
        void pushToFreeList_(MemBlock *block);
        void pushToUseList_(MemBlock *block);
        MemBlock *makeFreeBlock_(u8 *regionStart, u8 *regionEnd);
        MemBlock *findFirstMemBlockIfFree_();
        MemBlock *findLastMemBlockIfFree_();
        u32 adjustBack_();
        bool isValidBlock_(const void *ptr) const;

        OffsetList<MemBlock> m_freeList;
        OffsetList<MemBlock> m_useList;
        AllocMode m_allocMode;
    };

} // namespace pead
