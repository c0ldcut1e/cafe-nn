/**
 * @file
 * @brief This is a fixed-size queue of integer messages, which is how pead threads receive work and quit requests.
 */

#pragma once

#include <pead/peadTypes.h>

#ifdef __WIIU__
#include <coreinit/messagequeue.h>
#else
#include <condition_variable>
#include <mutex>
#endif

namespace pead
{
    class Heap;

    class MessageQueue
    {
    public:
        using Element = s32;

        enum BlockType
        {
            cBlock   = 0,
            cNoBlock = 1,
        };

        // pop returns this when the queue is empty and the call does not block, so it can't be sent as a message.
        static constexpr Element cNullElement = 0;

        MessageQueue();
        ~MessageQueue();
        MessageQueue(const MessageQueue &)            = delete;
        MessageQueue &operator=(const MessageQueue &) = delete;

        /**
         * @brief This allocates room for size messages.
         *
         * @param size How many messages fit before post blocks or fails.
         * @param heap The heap to allocate from, or null for the current one.
         */
        void allocate(s32 size, Heap *heap);
        void free();

        /**
         * @brief This adds a message at the back.
         *
         * @param message The message, which can't be cNullElement.
         * @param blockType cBlock waits for room and cNoBlock gives up when full.
         * @return False when the queue was full and the call didn't block.
         */
        bool push(Element message, BlockType blockType);

        /**
         * @brief This adds a message at the front, so it is popped before everything already queued.
         */
        bool jam(Element message, BlockType blockType);

        /**
         * @brief This takes the front message.
         *
         * @param blockType cBlock waits for a message and cNoBlock returns cNullElement when empty.
         * @return The message, or cNullElement.
         */
        Element pop(BlockType blockType);
        Element peek() const;

    private:
        Element *m_buffer;
        s32 m_size;
#ifdef __WIIU__
        OSMessageQueue m_queue;
        OSMessage *m_messages;
#else
        mutable std::mutex m_mutex;
        std::condition_variable m_notEmpty;
        std::condition_variable m_notFull;
        s32 m_head;
        s32 m_count;
#endif
    };

} // namespace pead
