/**
 * @file
 * @brief This is the abstract heap. Heaps form a tree, so each child takes its memory from its parent, and destroying a heap destroys its children and disposers too.
 */

#pragma once

#include <pead/peadCriticalSection.h>
#include <pead/peadRuntimeTypeInfo.h>
#include <pead/peadSafeString.h>

namespace pead
{
    class WriteStream;

    class Heap : public IDisposer
    {
        PEAD_RTTI_BASE(Heap)

    public:
        enum HeapDirection
        {
            cHeapDirection_Forward = 1,
            cHeapDirection_Reverse = -1,
        };

        /**
         * @brief This links the heap under parent. Subclasses set up the memory itself.
         *
         * @param name The name shown in dumps. It is copied.
         * @param parent The heap the memory came from, or null for a root heap.
         * @param start The first byte the heap manages.
         * @param size The size of the managed memory.
         * @param direction Which end a positive alignment allocates from.
         * @param enableLock True when more than one thread uses the heap.
         */
        Heap(const SafeString &name, Heap *parent, void *start, u32 size, HeapDirection direction, bool enableLock);
        ~Heap() override;

        /**
         * @brief This destroys the heap's disposers and children and gives its memory back to the parent. The subclass destructor calls it.
         */
        virtual void destroy() = 0;

        /**
         * @brief This shrinks the heap to what is allocated and gives the rest back to the parent.
         *
         * @return The new heap size.
         */
        virtual u32 adjust() = 0;

        /**
         * @brief This allocates a block.
         *
         * @param size The size in bytes.
         * @param alignment A power of two. A negative alignment allocates from the opposite end of the heap.
         * @return The block, or null when no free space fits.
         */
        virtual void *tryAlloc(u32 size, s32 alignment) = 0;
        virtual void free(void *ptr)                    = 0;

        /**
         * @brief This resizes a block in place, keeping its end where it is.
         *
         * @param ptr The block.
         * @param newSize The new size in bytes.
         * @return The block's new address, or null when it couldn't grow.
         */
        virtual void *resizeFront(void *ptr, u32 newSize) = 0;

        /**
         * @brief This resizes a block in place, keeping its start where it is.
         *
         * @param ptr The block.
         * @param newSize The new size in bytes.
         * @return The block's new size, or 0 when it couldn't grow.
         */
        virtual u32 resizeBack(void *ptr, u32 newSize) = 0;

        /**
         * @brief This resizes a block, moving it when it can't grow in place.
         *
         * @param ptr The block, or null to allocate.
         * @param newSize The new size in bytes.
         * @param alignment The alignment for a moved block.
         * @return The block, or null when there was no room. The old block stays valid on failure.
         */
        virtual void *tryRealloc(void *ptr, u32 newSize, s32 alignment);
        virtual void freeAll()                                 = 0;
        virtual const void *getStartAddress() const            = 0;
        virtual const void *getEndAddress() const              = 0;
        virtual u32 getSize() const                            = 0;
        virtual u32 getFreeSize() const                        = 0;
        virtual u32 getMaxAllocatableSize(s32 alignment) const = 0;
        virtual bool isInclude(const void *ptr) const          = 0;
        virtual bool isEmpty() const                           = 0;
        virtual bool isFreeable() const                        = 0;
        virtual bool isResizable() const                       = 0;
        virtual bool isAdjustable() const                      = 0;

        /**
         * @brief This prints the heap's state through system::PrintString.
         */
        virtual void dump() const;

        /**
         * @brief This writes the heap's state as YAML.
         *
         * @param stream The stream to write to.
         * @param indent How many spaces deep the heap's entry starts.
         */
        virtual void dumpYAML(WriteStream &stream, s32 indent) const;

        /**
         * @brief This writes the heap and every heap below it as YAML.
         */
        void dumpTreeYAML(WriteStream &stream, s32 indent) const;

        void *alloc(u32 size, s32 alignment = 8)
        {
            return tryAlloc(size, alignment);
        }

        const SafeString &getName() const
        {
            return m_name;
        }
        Heap *getParent() const
        {
            return m_parent;
        }
        HeapDirection getDirection() const
        {
            return m_direction;
        }
        bool isLockEnabled() const
        {
            return m_enableLock;
        }
        s32 getChildCount() const
        {
            return m_children.size();
        }
        Heap *getChild(s32 index) const
        {
            return m_children.nth(index);
        }

        void lock()
        {
            m_lock.lock();
        }
        void unlock()
        {
            m_lock.unlock();
        }

        /**
         * @brief This finds the deepest heap under this one that holds ptr.
         *
         * @param ptr The address to look up.
         * @return The heap, or null when ptr is outside this heap.
         */
        Heap *findContainHeap_(const void *ptr);

    protected:
        friend class IDisposer;
        friend class HeapMgr;
        friend class ExpHeap;

        /**
         * @brief This runs the destructor of every disposer and child, newest first, so subclasses call it before releasing memory.
         */
        void destruct_();

        /**
         * @brief This destroys the disposers that lie in [begin, end). A null range means all of them.
         */
        void dispose_(const void *begin, const void *end);
        void appendDisposer_(IDisposer *disposer);
        void removeDisposer_(IDisposer *disposer);
        void pushBackChild_(Heap *child);
        void eraseChild_(Heap *child);

        bool lockIfEnabled_()
        {
            if (m_enableLock)
                m_lock.lock();
            return m_enableLock;
        }
        void unlockIfEnabled_()
        {
            if (m_enableLock)
                m_lock.unlock();
        }

        FixedSafeString<32> m_name;
        void *m_start;
        u32 m_size;
        Heap *m_parent;
        OffsetList<Heap> m_children;
        ListNode m_childNode;
        OffsetList<IDisposer> m_disposers;
        HeapDirection m_direction;
        detail::RecursiveLock m_lock;
        bool m_enableLock;
    };

} // namespace pead
