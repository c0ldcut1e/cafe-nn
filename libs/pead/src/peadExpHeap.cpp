#include <cstring>
#include <pead/peadHeapMgr.h>
#include <pead/peadPrint.h>
#include <pead/peadStream.h>

namespace pead
{

    namespace
    {

        constexpr u32 cHeaderSize = (u32) sizeof(MemBlock);
        // Block sizes stay a multiple of this so every header we place lands correctly aligned for its pointers.
        constexpr u32 cMinAlignment = alignof(MemBlock) > 4 ? (u32) alignof(MemBlock) : 4;

        u8 *AlignUp(const void *ptr, u32 alignment)
        {
            uintptr_t p = (uintptr_t) ptr;
            return (u8 *) ((p + alignment - 1) & ~(uintptr_t) (alignment - 1));
        }

        u8 *AlignDown(const void *ptr, u32 alignment)
        {
            return (u8 *) ((uintptr_t) ptr & ~(uintptr_t) (alignment - 1));
        }

        u32 RoundSize(u32 size)
        {
            if (size == 0)
                size = 1;
            return (size + cMinAlignment - 1) & ~(cMinAlignment - 1);
        }

        u32 NormalizeAlignment(s32 alignment)
        {
            u32 a = (u32) (alignment < 0 ? -alignment : alignment);
            return a < cMinAlignment ? cMinAlignment : a;
        }

        u32 HeapObjectSize()
        {
            return (u32) (AlignUp((const void *) (uintptr_t) sizeof(ExpHeap), 8) - (u8 *) nullptr);
        }

    } // namespace

    ExpHeap::ExpHeap(const SafeString &name, Heap *parent, void *start, u32 size, HeapDirection direction, bool enableLock)
        : Heap(name, parent, start, size, direction, enableLock), m_freeList((s32) offsetof(MemBlock, node)),
          m_useList((s32) offsetof(MemBlock, node)), m_allocMode(cAllocMode_FirstFit)
    {}

    ExpHeap::~ExpHeap()
    {
        destruct_();
        if (m_parent)
            m_parent->eraseChild_(this);
    }

    ExpHeap *ExpHeap::create(u32 size, const SafeString &name, Heap *parent, HeapDirection direction, bool enableLock)
    {
        return tryCreate(size, name, parent, direction, enableLock);
    }

    ExpHeap *ExpHeap::tryCreate(u32 size, const SafeString &name, Heap *parent, HeapDirection direction, bool enableLock)
    {
        if (!parent)
        {
            HeapMgr *mgr = HeapMgr::instance();
            parent       = mgr ? mgr->getCurrentHeap() : nullptr;
            if (!parent)
                return nullptr;
        }
        if (size == 0)
            size = parent->getMaxAllocatableSize(8);
        if (size <= HeapObjectSize() + cHeaderSize + cMinFreeBlockSize)
            return nullptr;

        u8 *block = (u8 *) parent->tryAlloc(size, direction == cHeapDirection_Forward ? 8 : -8);
        if (!block)
            return nullptr;
        ExpHeap *heap = new (block) ExpHeap(name, parent, block + HeapObjectSize(), size - HeapObjectSize(), direction, enableLock);
        return doCreate(heap, parent);
    }

    ExpHeap *ExpHeap::create(void *address, u32 size, const SafeString &name, bool enableLock)
    {
        return tryCreate(address, size, name, enableLock);
    }

    ExpHeap *ExpHeap::tryCreate(void *address, u32 size, const SafeString &name, bool enableLock)
    {
        u8 *start = AlignUp(address, 8);
        u32 lost  = (u32) (start - (u8 *) address);
        if (!address || size <= lost + HeapObjectSize() + cHeaderSize + cMinFreeBlockSize)
            return nullptr;
        size -= lost;
        ExpHeap *heap = new (start) ExpHeap(name, nullptr, start + HeapObjectSize(), size - HeapObjectSize(), cHeapDirection_Forward, enableLock);
        return doCreate(heap, nullptr);
    }

    ExpHeap *ExpHeap::doCreate(ExpHeap *heap, Heap *parent)
    {
        heap->createMaxSizeFreeMemBlock_();
        if (parent)
            parent->pushBackChild_(heap);
        return heap;
    }

    s32 ExpHeap::compareMemBlockAddr_(const MemBlock *a, const MemBlock *b)
    {
        return a < b ? -1 : a > b ? 1 : 0;
    }

    void ExpHeap::destroy()
    {
        // A child heap's object sits at the start of the block it took from its parent, so that block is what we hand back.
        Heap *parent = m_parent;
        this->~ExpHeap();
        if (parent)
            parent->free(this);
    }

    u32 ExpHeap::adjust()
    {
        bool locked = lockIfEnabled_();
        u32 size    = adjustBack_();
        if (locked)
            unlockIfEnabled_();
        return size;
    }

    u32 ExpHeap::adjustBack_()
    {
        MemBlock *last = findLastMemBlockIfFree_();
        if (!last)
            return m_size;
        m_freeList.erase(last);
        u8 *newEnd = last->regionStart();
        m_size     = (u32) (newEnd - (u8 *) m_start);
        if (m_parent)
            m_parent->resizeBack(this, (u32) (newEnd - (u8 *) this));
        return m_size;
    }

    void *ExpHeap::tryAlloc(u32 size, s32 alignment)
    {
        bool locked   = lockIfEnabled_();
        bool fromHead = (alignment >= 0) == (m_direction == cHeapDirection_Forward);
        u32 rounded   = RoundSize(size);
        u32 align     = NormalizeAlignment(alignment);
        void *ptr     = fromHead ? allocFromHead_(rounded, (s32) align) : allocFromTail_(rounded, (s32) align);
        if (locked)
            unlockIfEnabled_();

        HeapMgr *mgr = HeapMgr::instance();
        if (ptr && mgr && mgr->getAllocCallback())
        {
            HeapMgr::AllocCallbackArg arg = {this, ptr, size, alignment};
            mgr->getAllocCallback()->invoke(&arg);
        }
        return ptr;
    }

    void *ExpHeap::allocFromHead_(u32 size, s32 alignment)
    {
        MemBlock *block = findFreeMemBlockFromHead_(size, alignment, (FindMode) m_allocMode);
        if (!block)
            return nullptr;
        return allocFromFreeBlock_(block, AlignUp(block->regionStart() + cHeaderSize, (u32) alignment), size);
    }

    void *ExpHeap::allocFromTail_(u32 size, s32 alignment)
    {
        MemBlock *block = findFreeMemBlockFromTail_(size, alignment, (FindMode) m_allocMode);
        if (!block)
            return nullptr;
        return allocFromFreeBlock_(block, AlignDown(block->regionEnd() - size, (u32) alignment), size);
    }

    MemBlock *ExpHeap::findFreeMemBlockFromHead_(u32 size, s32 alignment, FindMode mode) const
    {
        MemBlock *best = nullptr;
        for (MemBlock &block : m_freeList)
        {
            u8 *memory = AlignUp(block.regionStart() + cHeaderSize, (u32) alignment);
            if (memory + size > block.regionEnd())
                continue;
            if (mode == cFindMode_FirstFit)
                return &block;
            if (!best || block.size < best->size)
                best = &block;
        }
        return best;
    }

    MemBlock *ExpHeap::findFreeMemBlockFromTail_(u32 size, s32 alignment, FindMode mode) const
    {
        MemBlock *best           = nullptr;
        const ListNode *sentinel = ((const ListImpl &) m_freeList).end();
        for (ListNode *node = sentinel->prev(); node != sentinel; node = node->prev())
        {
            MemBlock *block = (MemBlock *) ((u8 *) node - offsetof(MemBlock, node));
            if (block->regionEnd() - block->regionStart() < (intptr_t) (size + cHeaderSize))
                continue;
            u8 *memory = AlignDown(block->regionEnd() - size, (u32) alignment);
            if (memory - cHeaderSize < block->regionStart())
                continue;
            if (mode == cFindMode_FirstFit)
                return block;
            if (!best || block->size < best->size)
                best = block;
        }
        return best;
    }

    void *ExpHeap::allocFromFreeBlock_(MemBlock *freeBlock, u8 *memory, u32 size)
    {
        u8 *regionStart = freeBlock->regionStart();
        u8 *regionEnd   = freeBlock->regionEnd();
        m_freeList.erase(freeBlock);

        u8 *header    = memory - cHeaderSize;
        u8 *usedStart = regionStart;
        if ((u32) (header - regionStart) >= cHeaderSize + cMinFreeBlockSize)
        {
            pushToFreeList_(makeFreeBlock_(regionStart, header));
            usedStart = header;
        }

        u8 *end = memory + size;
        if ((u32) (regionEnd - end) >= cHeaderSize + cMinFreeBlockSize)
            pushToFreeList_(makeFreeBlock_(end, regionEnd));
        else
            end = regionEnd;

        MemBlock *block         = new (header) MemBlock();
        block->signature        = MemBlock::cUseSignature;
        block->alignmentPadding = (u16) (header - usedStart);
        block->size             = (u32) (end - memory);
        pushToUseList_(block);
        return memory;
    }

    MemBlock *ExpHeap::makeFreeBlock_(u8 *regionStart, u8 *regionEnd)
    {
        MemBlock *block         = new (regionStart) MemBlock();
        block->signature        = MemBlock::cFreeSignature;
        block->alignmentPadding = 0;
        block->size             = (u32) (regionEnd - regionStart - cHeaderSize);
        return block;
    }

    void ExpHeap::pushToFreeList_(MemBlock *block)
    {
        // We keep free blocks in address order so a freed block only has to look at its two neighbors to merge.
        MemBlock *next = nullptr;
        for (MemBlock &candidate : m_freeList)
        {
            if (&candidate > block)
            {
                next = &candidate;
                break;
            }
        }
        MemBlock *prev = next ? (next->node.prev() == ((const ListImpl &) m_freeList).end()
                                         ? nullptr
                                         : (MemBlock *) ((u8 *) next->node.prev() - offsetof(MemBlock, node)))
                              : m_freeList.back();

        if (prev && prev->regionEnd() == block->regionStart())
        {
            prev->size += (u32) (block->regionEnd() - prev->regionEnd());
            block = prev;
        }
        else if (next)
        {
            ((ListImpl &) m_freeList).insertBefore(&next->node, &block->node);
        }
        else
        {
            m_freeList.pushBack(block);
        }

        if (next && block->regionEnd() == next->regionStart())
        {
            block->size += (u32) (next->regionEnd() - block->regionEnd());
            m_freeList.erase(next);
        }
    }

    void ExpHeap::pushToUseList_(MemBlock *block)
    {
        m_useList.pushBack(block);
    }

    bool ExpHeap::isValidBlock_(const void *ptr) const
    {
        if (!isInclude(ptr) || (const u8 *) ptr < (const u8 *) m_start + cHeaderSize)
            return false;
        const MemBlock *block = (const MemBlock *) ((const u8 *) ptr - cHeaderSize);
        return block->signature == MemBlock::cUseSignature && block->node.isLinked();
    }

    void ExpHeap::free(void *ptr)
    {
        if (!ptr)
            return;
        bool locked = lockIfEnabled_();
        bool valid  = isValidBlock_(ptr);
        if (valid)
        {
            MemBlock *block = (MemBlock *) ((u8 *) ptr - cHeaderSize);
            m_useList.erase(block);
            u8 *regionStart = block->regionStart();
            u8 *regionEnd   = block->regionEnd();
            pushToFreeList_(makeFreeBlock_(regionStart, regionEnd));
        }
        if (locked)
            unlockIfEnabled_();

        HeapMgr *mgr = HeapMgr::instance();
        if (valid && mgr && mgr->getFreeCallback())
        {
            HeapMgr::FreeCallbackArg arg = {this, ptr};
            mgr->getFreeCallback()->invoke(&arg);
        }
    }

    u32 ExpHeap::resizeBack(void *ptr, u32 newSize)
    {
        bool locked = lockIfEnabled_();
        u32 result  = 0;
        if (isValidBlock_(ptr))
        {
            MemBlock *block = (MemBlock *) ((u8 *) ptr - cHeaderSize);
            u8 *memory      = (u8 *) ptr;
            u8 *regionEnd   = block->regionEnd();
            u32 size        = RoundSize(newSize);
            bool fits       = true;
            if (size > block->size)
            {
                MemBlock *next = nullptr;
                for (MemBlock &candidate : m_freeList)
                {
                    if (candidate.regionStart() == regionEnd)
                    {
                        next = &candidate;
                        break;
                    }
                }
                if (next && (u32) (next->regionEnd() - memory) >= size)
                {
                    m_freeList.erase(next);
                    regionEnd = next->regionEnd();
                }
                else
                {
                    fits = false;
                }
            }
            if (fits)
            {
                u8 *end = memory + size;
                if ((u32) (regionEnd - end) >= cHeaderSize + cMinFreeBlockSize)
                    pushToFreeList_(makeFreeBlock_(end, regionEnd));
                else
                    end = regionEnd;
                block->size = (u32) (end - memory);
                result      = block->size;
            }
        }
        if (locked)
            unlockIfEnabled_();
        return result;
    }

    void *ExpHeap::resizeFront(void *ptr, u32 newSize)
    {
        bool locked  = lockIfEnabled_();
        void *result = nullptr;
        if (isValidBlock_(ptr))
        {
            MemBlock *block = (MemBlock *) ((u8 *) ptr - cHeaderSize);
            u8 *regionStart = block->regionStart();
            u8 *end         = block->regionEnd();
            u32 size        = RoundSize(newSize);
            u8 *memory      = (u32) (end - regionStart) >= size + cHeaderSize ? AlignDown(end - size, cMinAlignment) : nullptr;
            bool fits       = memory != nullptr;
            if (!fits || memory - cHeaderSize < regionStart)
            {
                MemBlock *prev = nullptr;
                for (MemBlock &candidate : m_freeList)
                {
                    if (candidate.regionEnd() == regionStart)
                    {
                        prev = &candidate;
                        break;
                    }
                }
                fits = prev && (u32) (end - prev->regionStart()) >= size + cHeaderSize;
                if (fits)
                {
                    m_freeList.erase(prev);
                    regionStart = prev->regionStart();
                    memory      = AlignDown(end - size, cMinAlignment);
                }
            }
            if (fits)
            {
                m_useList.erase(block);
                u8 *header    = memory - cHeaderSize;
                u8 *usedStart = regionStart;
                if ((u32) (header - regionStart) >= cHeaderSize + cMinFreeBlockSize)
                {
                    pushToFreeList_(makeFreeBlock_(regionStart, header));
                    usedStart = header;
                }
                MemBlock *moved         = new (header) MemBlock();
                moved->signature        = MemBlock::cUseSignature;
                moved->alignmentPadding = (u16) (header - usedStart);
                moved->size             = (u32) (end - memory);
                pushToUseList_(moved);
                result = memory;
            }
        }
        if (locked)
            unlockIfEnabled_();
        return result;
    }

    void *ExpHeap::tryRealloc(void *ptr, u32 newSize, s32 alignment)
    {
        if (!ptr)
            return tryAlloc(newSize, alignment);
        if (resizeBack(ptr, newSize) != 0)
            return ptr;
        void *moved = tryAlloc(newSize, alignment);
        if (!moved)
            return nullptr;
        u32 oldSize = getAllocatedSize(ptr);
        memcpy(moved, ptr, oldSize < newSize ? oldSize : newSize);
        free(ptr);
        return moved;
    }

    void ExpHeap::freeAll()
    {
        bool locked = lockIfEnabled_();
        dispose_(nullptr, nullptr);
        m_useList.clear();
        m_freeList.clear();
        createMaxSizeFreeMemBlock_();
        if (locked)
            unlockIfEnabled_();
    }

    void ExpHeap::createMaxSizeFreeMemBlock_()
    {
        u8 *regionStart = AlignUp(m_start, cMinAlignment);
        u8 *regionEnd   = AlignDown((u8 *) m_start + m_size, cMinAlignment);
        if (regionEnd > regionStart && (u32) (regionEnd - regionStart) >= cHeaderSize + cMinFreeBlockSize)
            pushToFreeList_(makeFreeBlock_(regionStart, regionEnd));
    }

    MemBlock *ExpHeap::findFirstMemBlockIfFree_()
    {
        MemBlock *first = m_freeList.front();
        return first && first->regionStart() == AlignUp(m_start, cMinAlignment) ? first : nullptr;
    }

    MemBlock *ExpHeap::findLastMemBlockIfFree_()
    {
        MemBlock *last = m_freeList.back();
        return last && last->regionEnd() == AlignDown((u8 *) m_start + m_size, cMinAlignment) ? last : nullptr;
    }

    u32 ExpHeap::getAllocatedSize(void *ptr)
    {
        return isValidBlock_(ptr) ? ((MemBlock *) ((u8 *) ptr - cHeaderSize))->size : 0;
    }

    u32 ExpHeap::getFreeSize() const
    {
        u32 total = 0;
        for (MemBlock &block : m_freeList) total += block.size;
        return total;
    }

    u32 ExpHeap::getMaxAllocatableSize(s32 alignment) const
    {
        u32 align = NormalizeAlignment(alignment);
        u32 best  = 0;
        for (MemBlock &block : m_freeList)
        {
            u8 *memory = AlignUp(block.regionStart() + cHeaderSize, align);
            if (memory >= block.regionEnd())
                continue;
            u32 size = (u32) (block.regionEnd() - memory) & ~(cMinAlignment - 1);
            if (size > best)
                best = size;
        }
        return best;
    }

    bool ExpHeap::isInclude(const void *ptr) const
    {
        return (const u8 *) ptr >= (const u8 *) m_start && (const u8 *) ptr < (const u8 *) m_start + m_size;
    }

    void ExpHeap::dump() const
    {
        Heap::dump();
        dumpFreeList();
        dumpUseList();
    }

    void ExpHeap::dumpYAML(WriteStream &stream, s32 indent) const
    {
        Heap::dumpYAML(stream, indent);
        FixedSafeString<96> line;
        line.format("%*s  used_blocks: %d\n%*s  free_blocks: %d\n", indent, "", getUseCount(), indent, "", getFreeCount());
        stream.writeDecorationText(line);
    }

    void ExpHeap::dumpFreeList() const
    {
        for (MemBlock &block : m_freeList) system::Print("  free %p size=%u\n", block.memory(), block.size);
    }

    void ExpHeap::dumpUseList() const
    {
        for (MemBlock &block : m_useList) system::Print("  used %p size=%u\n", block.memory(), block.size);
    }

} // namespace pead
