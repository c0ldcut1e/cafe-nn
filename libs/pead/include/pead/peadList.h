/**
 * @file
 * @brief These are pead's intrusive doubly linked lists. Heaps, threads and disposers link themselves into them, so a list never allocates.
 */

#pragma once

#include <pead/peadTypes.h>

namespace pead
{
    class ListImpl;

    class ListNode
    {
    public:
        ListNode() : m_prev(nullptr), m_next(nullptr), m_list(nullptr)
        {}

        ListNode *prev() const
        {
            return m_prev;
        }
        ListNode *next() const
        {
            return m_next;
        }
        bool isLinked() const
        {
            return m_list != nullptr;
        }

    private:
        friend class ListImpl;

        void init_()
        {
            m_prev = nullptr;
            m_next = nullptr;
            m_list = nullptr;
        }
        void insertFront_(ListNode *node);
        void insertBack_(ListNode *node);
        void erase_();

        ListNode *m_prev;
        ListNode *m_next;
        ListImpl *m_list;
    };

    /**
     * @brief This is the untyped list behind TList and OffsetList. It keeps a sentinel node, so the first node's prev and the last node's next point at it.
     */
    class ListImpl
    {
    public:
        using CompareCallbackImpl = s32 (*)(const void *, const void *);

        ListImpl() : m_count(0)
        {
            m_startEnd.m_prev = &m_startEnd;
            m_startEnd.m_next = &m_startEnd;
        }
        ListImpl(const ListImpl &)            = delete;
        ListImpl &operator=(const ListImpl &) = delete;

        bool isEmpty() const
        {
            return m_count == 0;
        }
        s32 size() const
        {
            return m_count;
        }

        void pushBack(ListNode *node)
        {
            m_startEnd.insertFront_(node);
        }
        void pushFront(ListNode *node)
        {
            m_startEnd.insertBack_(node);
        }
        ListNode *popBack();
        ListNode *popFront();
        void insertBefore(ListNode *basis, ListNode *node)
        {
            basis->insertFront_(node);
        }
        void insertAfter(ListNode *basis, ListNode *node)
        {
            basis->insertBack_(node);
        }
        void erase(ListNode *node)
        {
            node->erase_();
        }
        ListNode *front() const
        {
            return m_count > 0 ? m_startEnd.m_next : nullptr;
        }
        ListNode *back() const
        {
            return m_count > 0 ? m_startEnd.m_prev : nullptr;
        }

        /**
         * @brief This walks from the front, so it costs O(index).
         *
         * @param index The zero-based position.
         * @return The node, or null when index is out of range.
         */
        ListNode *nth(s32 index) const;
        s32 indexOf(const ListNode *node) const;

        /**
         * @brief This unlinks every node without touching the objects that hold them.
         */
        void clear();

        /**
         * @brief This sorts the list in place with a stable merge sort.
         *
         * @param offset The byte offset of the ListNode inside each object, so cmp is given the objects and not the nodes.
         * @param cmp Returns a negative number when the first object goes first.
         */
        void sort(s32 offset, CompareCallbackImpl cmp);

        const ListNode *end() const
        {
            return &m_startEnd;
        }

    private:
        friend class ListNode;

        ListNode m_startEnd;
        s32 m_count;
    };

    /**
     * @brief This is a list of objects that embed a ListNode, found through its byte offset.
     */
    template<typename T>
    class OffsetList : public ListImpl
    {
    public:
        explicit OffsetList(s32 offset = 0) : m_offset(offset)
        {}

        void initOffset(s32 offset)
        {
            m_offset = offset;
        }

        void pushBack(T *obj)
        {
            ListImpl::pushBack(toNode(obj));
        }
        void pushFront(T *obj)
        {
            ListImpl::pushFront(toNode(obj));
        }
        void erase(T *obj)
        {
            ListImpl::erase(toNode(obj));
        }
        T *front() const
        {
            return toObj(ListImpl::front());
        }
        T *back() const
        {
            return toObj(ListImpl::back());
        }
        T *next(const T *obj) const
        {
            ListNode *n = toNode(obj)->next();
            return n == ListImpl::end() ? nullptr : toObj(n);
        }
        T *nth(s32 index) const
        {
            return toObj(ListImpl::nth(index));
        }
        T *popFront()
        {
            return toObj(ListImpl::popFront());
        }
        bool isNodeLinked(const T *obj) const
        {
            return toNode(obj)->isLinked();
        }

        void sort(s32 (*cmp)(const T *, const T *))
        {
            ListImpl::sort(m_offset, (CompareCallbackImpl) cmp);
        }

        class iterator
        {
        public:
            iterator(const OffsetList *list, ListNode *node) : m_list(list), m_node(node)
            {}
            T &operator*() const
            {
                return *m_list->toObj(m_node);
            }
            T *operator->() const
            {
                return m_list->toObj(m_node);
            }
            iterator &operator++()
            {
                m_node = m_node->next();
                return *this;
            }
            bool operator!=(const iterator &other) const
            {
                return m_node != other.m_node;
            }

        private:
            const OffsetList *m_list;
            ListNode *m_node;
        };

        iterator begin() const
        {
            return iterator(this, ListImpl::end()->next());
        }
        iterator end() const
        {
            return iterator(this, (ListNode *) ListImpl::end());
        }

    private:
        ListNode *toNode(const T *obj) const
        {
            return (ListNode *) ((u8 *) obj + m_offset);
        }
        T *toObj(ListNode *node) const
        {
            return node ? (T *) ((u8 *) node - m_offset) : nullptr;
        }

        s32 m_offset;
    };

    template<typename T>
    class TListNode : public ListNode
    {
    public:
        TListNode() : m_data(nullptr)
        {}
        explicit TListNode(T data) : m_data(data)
        {}

        T m_data;
    };

    /**
     * @brief This is a list of TListNode, each of which carries a value, so the listed objects don't have to embed a node.
     */
    template<typename T>
    class TList : public ListImpl
    {
    public:
        void pushBack(TListNode<T> *node)
        {
            ListImpl::pushBack(node);
        }
        void pushFront(TListNode<T> *node)
        {
            ListImpl::pushFront(node);
        }
        TListNode<T> *front() const
        {
            return (TListNode<T> *) ListImpl::front();
        }
        TListNode<T> *back() const
        {
            return (TListNode<T> *) ListImpl::back();
        }
        TListNode<T> *popFront()
        {
            return (TListNode<T> *) ListImpl::popFront();
        }
        TListNode<T> *nth(s32 index) const
        {
            return (TListNode<T> *) ListImpl::nth(index);
        }
    };

} // namespace pead
