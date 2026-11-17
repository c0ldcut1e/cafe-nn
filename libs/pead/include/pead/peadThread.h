/**
 * @file
 * @brief These are pead's threads. Each one runs a message loop and has its own current heap, and ThreadMgr knows every thread so pead can find the one that is running.
 */

#pragma once

#include <pead/peadDelegate.h>
#include <pead/peadHeap.h>
#include <pead/peadMessageQueue.h>

#ifdef __WIIU__
#include <coreinit/thread.h>
#else
#include <thread>
#endif

namespace pead
{
    class Thread : public IDisposer
    {
    public:
        enum MessageQueueBlockType
        {
            cMessageQueueBlockType_Block   = MessageQueue::cBlock,
            cMessageQueueBlockType_NoBlock = MessageQueue::cNoBlock,
        };

        // This message tells run() to return, which quit() sends.
        static constexpr MessageQueue::Element cQuitMsg = 0x7FFFFFFF;
        static constexpr s32 cDefaultPriority           = 16;
        static constexpr s32 cDefaultStackSize          = 0x4000;
        static constexpr s32 cDefaultMessageQueueSize   = 32;

        /**
         * @brief This sets the thread up without starting it.
         *
         * @param name The thread's name. It is copied.
         * @param heap The heap the thread's stack and queue come from, and its first current heap.
         * @param priority The priority, where a lower number runs first on Wii U.
         * @param blockType Whether calc waits for messages or polls.
         * @param quitMsg The message that ends the loop.
         * @param stackSize The stack size in bytes.
         * @param messageQueueSize How many messages the queue holds.
         */
        Thread(const SafeString &name, Heap *heap, s32 priority, MessageQueueBlockType blockType, MessageQueue::Element quitMsg, s32 stackSize,
               s32 messageQueueSize);
        ~Thread() override;
        Thread(const Thread &)            = delete;
        Thread &operator=(const Thread &) = delete;

        /**
         * @brief This starts the thread, which calls run.
         */
        virtual void start();

        /**
         * @brief This asks the thread to stop by sending its quit message.
         *
         * @param isJam True to put the message ahead of everything already queued.
         */
        virtual void quit(bool isJam = false);

        /**
         * @brief This waits for the thread to return from run.
         */
        virtual void waitDone();
        virtual void quitAndDestroySingleThread(bool isJam = false);
        virtual void quitAndWaitDoneSingleThread(bool isJam = false);

        /**
         * @brief This sends msg to the thread's queue.
         *
         * @param msg The message, which can't be cNullElement.
         * @param blockType Whether to wait while the queue is full.
         * @return False when the queue was full and blockType was cNoBlock.
         */
        bool sendMessage(MessageQueue::Element msg, MessageQueue::BlockType blockType);
        MessageQueue::Element recvMessage(MessageQueue::BlockType blockType);
        bool isDone() const;

        /**
         * @brief This makes the calling thread sleep.
         */
        static void sleep(TickSpan howLong);
        static void yield();

        const SafeString &getName() const
        {
            return m_name;
        }
        Heap *getCurrentHeap() const
        {
            return m_currentHeap;
        }
        Heap *setCurrentHeap_(Heap *heap)
        {
            Heap *previous = m_currentHeap;
            m_currentHeap  = heap;
            return previous;
        }
        s32 getPriority() const
        {
            return m_priority;
        }
        void setPriority(s32 priority);
        CoreIdMask getAffinity() const
        {
            return m_affinity;
        }
        void setAffinity(const CoreIdMask &affinity);
        u32 getId() const
        {
            return m_id;
        }
        MessageQueue &getMessageQueue()
        {
            return m_messageQueue;
        }

    protected:
        /**
         * @brief This is the thread's body. The default receives messages and passes each one to calc until the quit message arrives.
         */
        virtual void run_();

        /**
         * @brief This handles one message. A poll thread gets cNullElement when the queue was empty.
         */
        virtual void calc_(MessageQueue::Element msg) = 0;

        /**
         * @brief This is the entry the platform thread starts in. It records the thread as current and calls run_.
         */
        static void threadEntry_(Thread *thread);
        // OSCreateThread passes argv through untouched, so on Wii U we smuggle the Thread pointer in it.
        static int osThreadEntry_(int argc, const char **argv);

        FixedSafeString<32> m_name;
        MessageQueue m_messageQueue;
        MessageQueueBlockType m_blockType;
        MessageQueue::Element m_quitMsg;
        Heap *m_currentHeap;
        s32 m_priority;
        s32 m_stackSize;
        CoreIdMask m_affinity;
        u32 m_id;
        ListNode m_listNode;
        bool m_started;
        bool m_done;
#ifdef __WIIU__
        OSThread *m_osThread;
        u8 *m_stack;
#else
        std::thread m_osThread;
#endif

        friend class ThreadMgr;
    };

    /**
     * @brief This thread calls a delegate for each message, so a class can run a thread without deriving from Thread.
     */
    class DelegateThread : public Thread
    {
    public:
        DelegateThread(const SafeString &name, IDelegate2<Thread *, MessageQueue::Element> *delegate, Heap *heap, s32 priority,
                       MessageQueueBlockType blockType, MessageQueue::Element quitMsg, s32 stackSize, s32 messageQueueSize);
        ~DelegateThread() override;

    protected:
        void calc_(MessageQueue::Element msg) override;

        IDelegate2<Thread *, MessageQueue::Element> *m_delegate;
    };

    /**
     * @brief This stands for the thread that initialized pead. It never starts, since it is already running.
     */
    class MainThread : public Thread
    {
    public:
        MainThread(Heap *heap, MessageQueueBlockType blockType, MessageQueue::Element quitMsg, s32 messageQueueSize);
        ~MainThread() override;

        void start() override
        {}
        void quit(bool) override
        {}
        void waitDone() override
        {}

    protected:
        void calc_(MessageQueue::Element) override
        {}
    };

    class ThreadMgr
    {
    public:
        /**
         * @brief This makes the calling thread the main thread.
         *
         * @param heap The heap the main thread's objects come from.
         */
        static void initialize(Heap *heap);
        static void destroy();
        static ThreadMgr *instance();

        /**
         * @brief This returns the pead thread that is running, or null for a thread pead did not create.
         */
        Thread *getCurrentThread() const;
        MainThread *getMainThread() const
        {
            return m_mainThread;
        }
        bool isMainThread() const
        {
            return getCurrentThread() == m_mainThread;
        }

        /**
         * @brief This sends quitMsg to every thread and waits for them, leaving only the main thread.
         */
        void quitAndWaitDoneMultipleThread(bool isJam = false);
        void waitDoneMultipleThread();
        s32 getThreadCount() const
        {
            return m_threads.size();
        }

    private:
        friend class Thread;

        ThreadMgr();
        ~ThreadMgr();

        void add_(Thread *thread);
        void remove_(Thread *thread);
        static void setCurrentThread_(Thread *thread);

        OffsetList<Thread> m_threads;
        MainThread *m_mainThread;
        detail::RecursiveLock m_lock;

        static ThreadMgr *s_instance;
    };

} // namespace pead
