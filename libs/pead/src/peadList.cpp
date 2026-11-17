#include <pead/peadList.h>

namespace pead
{

    void ListNode::insertFront_(ListNode *node)
    {
        ListImpl *list = m_list ? m_list : (ListImpl *) ((u8 *) this - offsetof(ListImpl, m_startEnd));
        node->m_prev   = m_prev;
        node->m_next   = this;
        m_prev->m_next = node;
        m_prev         = node;
        node->m_list   = list;
        list->m_count++;
    }

    void ListNode::insertBack_(ListNode *node)
    {
        ListImpl *list = m_list ? m_list : (ListImpl *) ((u8 *) this - offsetof(ListImpl, m_startEnd));
        node->m_prev   = this;
        node->m_next   = m_next;
        m_next->m_prev = node;
        m_next         = node;
        node->m_list   = list;
        list->m_count++;
    }

    void ListNode::erase_()
    {
        if (!m_list)
            return;
        m_list->m_count--;
        m_prev->m_next = m_next;
        m_next->m_prev = m_prev;
        init_();
    }

    ListNode *ListImpl::popBack()
    {
        ListNode *node = back();
        if (node)
            node->erase_();
        return node;
    }

    ListNode *ListImpl::popFront()
    {
        ListNode *node = front();
        if (node)
            node->erase_();
        return node;
    }

    ListNode *ListImpl::nth(s32 index) const
    {
        if (index < 0 || index >= m_count)
            return nullptr;
        ListNode *node = m_startEnd.m_next;
        while (index-- > 0) node = node->m_next;
        return node;
    }

    s32 ListImpl::indexOf(const ListNode *node) const
    {
        s32 index = 0;
        for (const ListNode *n = m_startEnd.m_next; n != &m_startEnd; n = n->m_next, index++)
        {
            if (n == node)
                return index;
        }
        return -1;
    }

    void ListImpl::clear()
    {
        while (popFront())
        {
        }
    }

    void ListImpl::sort(s32 offset, CompareCallbackImpl cmp)
    {
        if (m_count < 2)
            return;

        // We merge runs of doubling width in place, the bottom-up form of merge sort, so it needs no buffer and stays stable.
        ListNode *head            = m_startEnd.m_next;
        m_startEnd.m_prev->m_next = nullptr;
        for (s32 width = 1;; width *= 2)
        {
            ListNode *result = nullptr;
            ListNode **tail  = &result;
            s32 merges       = 0;
            ListNode *p      = head;
            while (p)
            {
                merges++;
                ListNode *q = p;
                s32 pSize   = 0;
                for (s32 i = 0; i < width && q; i++)
                {
                    pSize++;
                    q = q->m_next;
                }
                s32 qSize = width;
                while (pSize > 0 || (qSize > 0 && q))
                {
                    ListNode *take;
                    if (pSize == 0)
                    {
                        take = q;
                        q    = q->m_next;
                        qSize--;
                    }
                    else if (qSize == 0 || !q || cmp((u8 *) p - offset, (u8 *) q - offset) <= 0)
                    {
                        take = p;
                        p    = p->m_next;
                        pSize--;
                    }
                    else
                    {
                        take = q;
                        q    = q->m_next;
                        qSize--;
                    }
                    *tail = take;
                    tail  = &take->m_next;
                }
                p = q;
            }
            *tail = nullptr;
            head  = result;
            if (merges <= 1)
                break;
        }

        ListNode *prev = &m_startEnd;
        for (ListNode *n = head; n; n = n->m_next)
        {
            n->m_prev    = prev;
            prev->m_next = n;
            prev         = n;
        }
        prev->m_next      = &m_startEnd;
        m_startEnd.m_prev = prev;
    }

} // namespace pead
