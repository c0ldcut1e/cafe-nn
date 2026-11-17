#include <pead/peadHeapMgr.h>
#include <pead/peadThread.h>

#ifdef __WIIU__
#include <coreinit/memdefaultheap.h>
#endif

namespace pead
{

    namespace
    {

        u32 s_nextThreadId = 1;

#ifndef __WIIU__
        thread_local Thread *t_currentThread = nullptr;
#endif

#ifdef __WIIU__
        u32 ToOsAffinity(const CoreIdMask &mask)
        {
            u32 affinity = 0;
            for (s32 core = 0; core < (s32) CoreInfo::getNumCores(); core++)
            {
                if (mask.isOn(CoreId(core)))
                    affinity |= CoreInfo::getPlatformMask(CoreId(core));
            }
            return affinity;
        }
#endif

    } // namespace

    ThreadMgr *ThreadMgr::s_instance = nullptr;

#if defined(__WIIU__)
    int Thread::osThreadEntry_(int, const char **argv)
    {
        threadEntry_((Thread *) argv);
        return 0;
    }
#endif

    Thread::Thread(const SafeString &name, Heap *heap, s32 priority, MessageQueueBlockType blockType, MessageQueue::Element quitMsg, s32 stackSize,
                   s32 messageQueueSize)
        : IDisposer(heap, cHeapNullOption_UseSpecifiedOrCurrentHeap), m_name(name), m_blockType(blockType), m_quitMsg(quitMsg), m_currentHeap(heap),
          m_priority(priority), m_stackSize(stackSize), m_affinity(CoreInfo::getMainCoreId()), m_id(s_nextThreadId++), m_started(false), m_done(false)
#ifdef __WIIU__
          ,
          m_osThread(nullptr), m_stack(nullptr)
#endif
    {
        if (!m_currentHeap && HeapMgr::instance())
            m_currentHeap = HeapMgr::instance()->getCurrentHeap();
        if (messageQueueSize > 0)
            m_messageQueue.allocate(messageQueueSize, heap);
        if (ThreadMgr *mgr = ThreadMgr::instance())
            mgr->add_(this);
    }

    Thread::~Thread()
    {
        if (m_started)
            waitDone();
        if (ThreadMgr *mgr = ThreadMgr::instance())
            mgr->remove_(this);
        m_messageQueue.free();
#ifdef __WIIU__
        freeToSeadHeap(m_osThread);
        freeToSeadHeap(m_stack);
#endif
    }

    void Thread::start()
    {
        if (m_started)
            return;
        m_started = true;
        m_done    = false;
#ifdef __WIIU__
        if (!m_osThread)
        {
            m_osThread = (OSThread *) allocateFromSeadHeap(sizeof(OSThread), m_currentHeap, 8);
            m_stack    = (u8 *) allocateFromSeadHeap((u32) m_stackSize, m_currentHeap, 8);
        }
        if (!m_osThread || !m_stack)
        {
            m_done = true;
            return;
        }
        OSCreateThread(m_osThread, Thread::osThreadEntry_, 0, (char *) this, m_stack + m_stackSize, (u32) m_stackSize, m_priority,
                       (OSThreadAttributes) ToOsAffinity(m_affinity));
        OSSetThreadName(m_osThread, m_name.cstr());
        OSResumeThread(m_osThread);
#else
        m_osThread = std::thread(threadEntry_, this);
#endif
    }

    void Thread::quit(bool isJam)
    {
        if (isJam)
            m_messageQueue.jam(m_quitMsg, MessageQueue::cBlock);
        else
            m_messageQueue.push(m_quitMsg, MessageQueue::cBlock);
    }

    void Thread::waitDone()
    {
        if (!m_started)
            return;
#ifdef __WIIU__
        if (m_osThread)
            OSJoinThread(m_osThread, nullptr);
#else
        if (m_osThread.joinable())
            m_osThread.join();
#endif
        m_started = false;
    }

    void Thread::quitAndDestroySingleThread(bool isJam)
    {
        quit(isJam);
        waitDone();
    }

    void Thread::quitAndWaitDoneSingleThread(bool isJam)
    {
        quit(isJam);
        waitDone();
    }

    bool Thread::sendMessage(MessageQueue::Element msg, MessageQueue::BlockType blockType)
    {
        return m_messageQueue.push(msg, blockType);
    }

    MessageQueue::Element Thread::recvMessage(MessageQueue::BlockType blockType)
    {
        return m_messageQueue.pop(blockType);
    }

    bool Thread::isDone() const
    {
        return m_done;
    }

    void Thread::sleep(TickSpan howLong)
    {
#ifdef __WIIU__
        OSSleepTicks(howLong.toTicks());
#else
        std::this_thread::sleep_for(std::chrono::nanoseconds(howLong.toNanoSeconds()));
#endif
    }

    void Thread::yield()
    {
#ifdef __WIIU__
        OSYieldThread();
#else
        std::this_thread::yield();
#endif
    }

    void Thread::setPriority(s32 priority)
    {
        m_priority = priority;
#ifdef __WIIU__
        if (m_osThread && m_started)
            OSSetThreadPriority(m_osThread, priority);
#endif
    }

    void Thread::setAffinity(const CoreIdMask &affinity)
    {
        m_affinity = affinity;
#ifdef __WIIU__
        if (m_osThread && m_started)
            OSSetThreadAffinity(m_osThread, ToOsAffinity(affinity));
#endif
    }

    void Thread::run_()
    {
        for (;;)
        {
            MessageQueue::Element msg = m_messageQueue.pop((MessageQueue::BlockType) m_blockType);
            if (msg == m_quitMsg)
                break;
            calc_(msg);
        }
    }

    void Thread::threadEntry_(Thread *thread)
    {
        ThreadMgr::setCurrentThread_(thread);
        thread->run_();
        thread->m_done = true;
        ThreadMgr::setCurrentThread_(nullptr);
    }

    DelegateThread::DelegateThread(const SafeString &name, IDelegate2<Thread *, MessageQueue::Element> *delegate, Heap *heap, s32 priority,
                                   MessageQueueBlockType blockType, MessageQueue::Element quitMsg, s32 stackSize, s32 messageQueueSize)
        : Thread(name, heap, priority, blockType, quitMsg, stackSize, messageQueueSize), m_delegate(delegate)
    {}

    DelegateThread::~DelegateThread() = default;

    void DelegateThread::calc_(MessageQueue::Element msg)
    {
        if (m_delegate)
            m_delegate->invoke(this, msg);
    }

    MainThread::MainThread(Heap *heap, MessageQueueBlockType blockType, MessageQueue::Element quitMsg, s32 messageQueueSize)
        : Thread("pead_main_thread", heap, cDefaultPriority, blockType, quitMsg, 0, messageQueueSize)
    {}

    MainThread::~MainThread() = default;

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif
    ThreadMgr::ThreadMgr() : m_threads((s32) offsetof(Thread, m_listNode)), m_mainThread(nullptr)
    {}
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

    ThreadMgr::~ThreadMgr() = default;

    void ThreadMgr::initialize(Heap *heap)
    {
        if (s_instance)
            return;
        s_instance = new (heap) ThreadMgr();
        if (!s_instance)
            return;
        s_instance->m_mainThread =
                new (heap) MainThread(heap, Thread::cMessageQueueBlockType_NoBlock, Thread::cQuitMsg, Thread::cDefaultMessageQueueSize);
        setCurrentThread_(s_instance->m_mainThread);
    }

    void ThreadMgr::destroy()
    {
        if (!s_instance)
            return;
        ThreadMgr *mgr = s_instance;
        setCurrentThread_(nullptr);
        Delete(mgr->m_mainThread);
        s_instance = nullptr;
        mgr->~ThreadMgr();
        freeToSeadHeap(mgr);
    }

    ThreadMgr *ThreadMgr::instance()
    {
        return s_instance;
    }

    Thread *ThreadMgr::getCurrentThread() const
    {
#ifdef __WIIU__
        return (Thread *) OSGetThreadSpecific(OS_THREAD_SPECIFIC_0);
#else
        return t_currentThread;
#endif
    }

    void ThreadMgr::setCurrentThread_(Thread *thread)
    {
#ifdef __WIIU__
        OSSetThreadSpecific(OS_THREAD_SPECIFIC_0, thread);
#else
        t_currentThread = thread;
#endif
    }

    void ThreadMgr::quitAndWaitDoneMultipleThread(bool isJam)
    {
        {
            ScopedLock<detail::RecursiveLock> lock(&m_lock);
            for (Thread &thread : m_threads)
            {
                if (&thread != m_mainThread)
                    thread.quit(isJam);
            }
        }
        waitDoneMultipleThread();
    }

    void ThreadMgr::waitDoneMultipleThread()
    {
        ScopedLock<detail::RecursiveLock> lock(&m_lock);
        for (Thread &thread : m_threads)
        {
            if (&thread != m_mainThread)
                thread.waitDone();
        }
    }

    void ThreadMgr::add_(Thread *thread)
    {
        ScopedLock<detail::RecursiveLock> lock(&m_lock);
        m_threads.pushBack(thread);
    }

    void ThreadMgr::remove_(Thread *thread)
    {
        ScopedLock<detail::RecursiveLock> lock(&m_lock);
        if (thread->m_listNode.isLinked())
            m_threads.erase(thread);
    }

} // namespace pead
