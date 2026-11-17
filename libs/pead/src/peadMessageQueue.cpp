#include <pead/peadMessageQueue.h>
#include <pead/peadNew.h>

namespace pead
{

    MessageQueue::MessageQueue() : m_buffer(nullptr), m_size(0)
    {
#ifdef __WIIU__
        m_messages = nullptr;
#else
        m_head  = 0;
        m_count = 0;
#endif
    }

    MessageQueue::~MessageQueue()
    {
        free();
    }

#ifdef __WIIU__
    void MessageQueue::allocate(s32 size, Heap *heap)
    {
        free();
        m_messages = (OSMessage *) allocateFromSeadHeap((u32) (sizeof(OSMessage) * size), heap, 4);
        if (!m_messages)
            return;
        m_size = size;
        OSInitMessageQueue(&m_queue, m_messages, size);
    }

    void MessageQueue::free()
    {
        freeToSeadHeap(m_messages);
        m_messages = nullptr;
        m_size     = 0;
    }

    bool MessageQueue::push(Element message, BlockType blockType)
    {
        OSMessage msg = {};
        msg.message   = (void *) (uintptr_t) message;
        return OSSendMessage(&m_queue, &msg, blockType == cBlock ? OS_MESSAGE_FLAGS_BLOCKING : OS_MESSAGE_FLAGS_NONE);
    }

    bool MessageQueue::jam(Element message, BlockType blockType)
    {
        OSMessage msg = {};
        msg.message   = (void *) (uintptr_t) message;
        // wut has no OSJamMessage, and the high priority flag is how coreinit puts a message at the front.
        u32 flags = OS_MESSAGE_FLAGS_HIGH_PRIORITY | (blockType == cBlock ? OS_MESSAGE_FLAGS_BLOCKING : OS_MESSAGE_FLAGS_NONE);
        return OSSendMessage(&m_queue, &msg, (OSMessageFlags) flags);
    }

    MessageQueue::Element MessageQueue::pop(BlockType blockType)
    {
        OSMessage msg = {};
        if (!OSReceiveMessage(&m_queue, &msg, blockType == cBlock ? OS_MESSAGE_FLAGS_BLOCKING : OS_MESSAGE_FLAGS_NONE))
            return cNullElement;
        return (Element) (uintptr_t) msg.message;
    }

    MessageQueue::Element MessageQueue::peek() const
    {
        OSMessage msg = {};
        if (!OSPeekMessage((OSMessageQueue *) &m_queue, &msg))
            return cNullElement;
        return (Element) (uintptr_t) msg.message;
    }
#else
    void MessageQueue::allocate(s32 size, Heap *heap)
    {
        free();
        m_buffer = (Element *) allocateFromSeadHeap((u32) (sizeof(Element) * size), heap, 4);
        if (!m_buffer)
            return;
        m_size  = size;
        m_head  = 0;
        m_count = 0;
    }

    void MessageQueue::free()
    {
        freeToSeadHeap(m_buffer);
        m_buffer = nullptr;
        m_size   = 0;
        m_count  = 0;
    }

    bool MessageQueue::push(Element message, BlockType blockType)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_count == m_size)
        {
            if (blockType == cNoBlock || m_size == 0)
                return false;
            m_notFull.wait(lock,
                           [this]
                           {
                               return m_count < m_size;
                           });
        }
        m_buffer[(m_head + m_count) % m_size] = message;
        m_count++;
        m_notEmpty.notify_one();
        return true;
    }

    bool MessageQueue::jam(Element message, BlockType blockType)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_count == m_size)
        {
            if (blockType == cNoBlock || m_size == 0)
                return false;
            m_notFull.wait(lock,
                           [this]
                           {
                               return m_count < m_size;
                           });
        }
        m_head           = (m_head + m_size - 1) % m_size;
        m_buffer[m_head] = message;
        m_count++;
        m_notEmpty.notify_one();
        return true;
    }

    MessageQueue::Element MessageQueue::pop(BlockType blockType)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_count == 0)
        {
            if (blockType == cNoBlock || m_size == 0)
                return cNullElement;
            m_notEmpty.wait(lock,
                            [this]
                            {
                                return m_count > 0;
                            });
        }
        Element message = m_buffer[m_head];
        m_head          = (m_head + 1) % m_size;
        m_count--;
        m_notFull.notify_one();
        return message;
    }

    MessageQueue::Element MessageQueue::peek() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_count > 0 ? m_buffer[m_head] : cNullElement;
    }
#endif

} // namespace pead
