#include <pead/peadHeapMgr.h>
#include <pead/peadPrint.h>
#include <pead/peadStream.h>

namespace pead
{

    Heap::Heap(const SafeString &name, Heap *parent, void *start, u32 size, HeapDirection direction, bool enableLock)
        : IDisposer(parent, cHeapNullOption_DoNotAppendDisposerIfNoHeapSpecified), m_name(name), m_start(start), m_size(size), m_parent(parent),
          m_direction(direction), m_enableLock(enableLock)
    {
        m_children.initOffset((s32) ((u8 *) &m_childNode - (u8 *) this));
        m_disposers.initOffset(IDisposer::getListNodeOffset());
    }

    Heap::~Heap() = default;

    void *Heap::tryRealloc(void *ptr, u32 newSize, s32 alignment)
    {
        return ptr ? nullptr : tryAlloc(newSize, alignment);
    }

    void Heap::dump() const
    {
        system::Print("[%s] start=%p size=%u free=%u\n", m_name.cstr(), m_start, getSize(), getFreeSize());
    }

    void Heap::dumpYAML(WriteStream &stream, s32 indent) const
    {
        FixedSafeString<160> line;
        line.format("%*s- name: \"%s\"\n%*s  size: %u\n%*s  free: %u\n", indent, "", m_name.cstr(), indent, "", getSize(), indent, "", getFreeSize());
        stream.writeDecorationText(line);
    }

    void Heap::dumpTreeYAML(WriteStream &stream, s32 indent) const
    {
        dumpYAML(stream, indent);
        if (m_children.isEmpty())
            return;
        FixedSafeString<64> line;
        line.format("%*s  children:\n", indent, "");
        stream.writeDecorationText(line);
        for (Heap &child : m_children) child.dumpTreeYAML(stream, indent + 4);
    }

    Heap *Heap::findContainHeap_(const void *ptr)
    {
        if (!isInclude(ptr))
            return nullptr;
        for (Heap &child : m_children)
        {
            Heap *found = child.findContainHeap_(ptr);
            if (found)
                return found;
        }
        return this;
    }

    void Heap::destruct_()
    {
        dispose_(nullptr, nullptr);
    }

    void Heap::dispose_(const void *begin, const void *end)
    {
        bool locked = lockIfEnabled_();
        // Each disposer's destructor unlinks it, so we always take whatever is newest until nothing in range is left.
        for (;;)
        {
            IDisposer *target        = nullptr;
            const ListNode *sentinel = ((const ListImpl &) m_disposers).end();
            for (ListNode *node = sentinel->prev(); node != sentinel; node = node->prev())
            {
                IDisposer *candidate = (IDisposer *) ((u8 *) node - IDisposer::getListNodeOffset());
                if (!begin || ((const u8 *) candidate >= (const u8 *) begin && (const u8 *) candidate < (const u8 *) end))
                {
                    target = candidate;
                    break;
                }
            }
            if (!target)
                break;
            target->~IDisposer();
            // We unlink it ourselves in case its destructor didn't, so the loop always makes progress.
            if (target->m_disposerHeap == this)
                removeDisposer_(target);
        }
        if (locked)
            unlockIfEnabled_();
    }

    void Heap::appendDisposer_(IDisposer *disposer)
    {
        bool locked = lockIfEnabled_();
        m_disposers.pushBack(disposer);
        disposer->m_disposerHeap = this;
        if (locked)
            unlockIfEnabled_();
    }

    void Heap::removeDisposer_(IDisposer *disposer)
    {
        bool locked = lockIfEnabled_();
        m_disposers.erase(disposer);
        disposer->m_disposerHeap = nullptr;
        if (locked)
            unlockIfEnabled_();
    }

    void Heap::pushBackChild_(Heap *child)
    {
        ScopedLock<detail::RecursiveLock> lock(HeapMgr::getHeapTreeLock());
        m_children.pushBack(child);
    }

    void Heap::eraseChild_(Heap *child)
    {
        ScopedLock<detail::RecursiveLock> lock(HeapMgr::getHeapTreeLock());
        if (m_children.isNodeLinked(child))
            m_children.erase(child);
    }

} // namespace pead
