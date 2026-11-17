/**
 * @file
 * @brief This is an event threads wait on until another thread signals it.
 */

#pragma once

#include <pead/peadDisposer.h>

#ifdef __WIIU__
#include <coreinit/event.h>
#else
#include <condition_variable>
#include <mutex>
#endif

namespace pead
{
    class Event : public IDisposer
    {
    public:
        /**
         * @brief The no-argument constructors leave the event uninitialized, so call initialize before using it.
         */
        Event();
        explicit Event(bool manualReset);
        explicit Event(Heap *heap);
        Event(Heap *heap, bool manualReset);
        ~Event() override;

        /**
         * @brief This sets up the event unsignaled.
         *
         * @param manualReset True to stay signaled until resetSignal, false to wake one waiter and reset.
         */
        void initialize(bool manualReset);
        void wait();

        /**
         * @brief This waits until the event is signaled or the time runs out.
         *
         * @param timeout How long to wait.
         * @return True when the event was signaled, false on timeout.
         */
        bool wait(TickSpan timeout);
        void setSignal();
        void resetSignal();

    private:
        bool m_initialized;
#ifdef __WIIU__
        OSEvent m_event;
#else
        std::mutex m_mutex;
        std::condition_variable m_cond;
        bool m_signaled;
        bool m_manualReset;
#endif
    };

} // namespace pead
