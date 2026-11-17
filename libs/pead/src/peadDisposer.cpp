#include <cstddef>
#include <pead/peadHeapMgr.h>

namespace pead
{

    IDisposer::IDisposer() : m_disposerHeap(nullptr)
    {
        if (!HeapMgr::isInitialized())
            return;
        Heap *heap = HeapMgr::instance()->findContainHeap(this);
        if (heap)
            heap->appendDisposer_(this);
    }

    IDisposer::IDisposer(Heap *heap, HeapNullOption option) : m_disposerHeap(nullptr)
    {
        if (!heap && HeapMgr::isInitialized())
        {
            if (option == cHeapNullOption_UseSpecifiedOrContainHeap)
                heap = HeapMgr::instance()->findContainHeap(this);
            else if (option == cHeapNullOption_UseSpecifiedOrCurrentHeap)
                heap = HeapMgr::instance()->getCurrentHeap();
        }
        if (heap)
            heap->appendDisposer_(this);
    }

    IDisposer::~IDisposer()
    {
        if (m_disposerHeap)
            m_disposerHeap->removeDisposer_(this);
    }

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif
    s32 IDisposer::getListNodeOffset()
    {
        return (s32) offsetof(IDisposer, m_listNode);
    }
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

} // namespace pead
