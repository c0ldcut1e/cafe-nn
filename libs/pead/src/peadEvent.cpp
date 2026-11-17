#include <pead/peadEvent.h>

namespace pead
{

    Event::Event() : IDisposer(), m_initialized(false)
    {
        initialize(false);
    }

    Event::Event(bool manualReset) : IDisposer(), m_initialized(false)
    {
        initialize(manualReset);
    }

    Event::Event(Heap *heap) : IDisposer(heap, cHeapNullOption_UseSpecifiedOrContainHeap), m_initialized(false)
    {
        initialize(false);
    }

    Event::Event(Heap *heap, bool manualReset) : IDisposer(heap, cHeapNullOption_UseSpecifiedOrContainHeap), m_initialized(false)
    {
        initialize(manualReset);
    }

    Event::~Event() = default;

#ifdef __WIIU__
    void Event::initialize(bool manualReset)
    {
        OSInitEvent(&m_event, FALSE, manualReset ? OS_EVENT_MODE_MANUAL : OS_EVENT_MODE_AUTO);
        m_initialized = true;
    }

    void Event::wait()
    {
        OSWaitEvent(&m_event);
    }

    bool Event::wait(TickSpan timeout)
    {
        return OSWaitEventWithTimeout(&m_event, (OSTime) timeout.toNanoSeconds());
    }

    void Event::setSignal()
    {
        OSSignalEvent(&m_event);
    }

    void Event::resetSignal()
    {
        OSResetEvent(&m_event);
    }
#else
    void Event::initialize(bool manualReset)
    {
        m_signaled    = false;
        m_manualReset = manualReset;
        m_initialized = true;
    }

    void Event::wait()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cond.wait(lock,
                    [this]
                    {
                        return m_signaled;
                    });
        if (!m_manualReset)
            m_signaled = false;
    }

    bool Event::wait(TickSpan timeout)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (!m_cond.wait_for(lock, std::chrono::nanoseconds(timeout.toNanoSeconds()),
                             [this]
                             {
                                 return m_signaled;
                             }))
            return false;
        if (!m_manualReset)
            m_signaled = false;
        return true;
    }

    void Event::setSignal()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_signaled = true;
        // An auto-reset event wakes only one waiter, since the first one through clears the signal anyway.
        if (m_manualReset)
            m_cond.notify_all();
        else
            m_cond.notify_one();
    }

    void Event::resetSignal()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_signaled = false;
    }
#endif

} // namespace pead
