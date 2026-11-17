/**
 * @file
 * @brief These are pead's delegates, which bind an object to one of its member functions so a thread or event can call back into it. DelegateThread and PrintConfig take them.
 */

#pragma once

#include <pead/peadList.h>
#include <pead/peadNew.h>

namespace pead
{
    class IDelegate
    {
    public:
        virtual ~IDelegate()  = default;
        virtual void invoke() = 0;
        void operator()()
        {
            invoke();
        }
        virtual IDelegate *clone(Heap *heap) const = 0;
    };

    template<typename A1>
    class IDelegate1
    {
    public:
        virtual ~IDelegate1()      = default;
        virtual void invoke(A1 a1) = 0;
        void operator()(A1 a1)
        {
            invoke(a1);
        }

        /**
         * @brief This copies the delegate into heap, so the copy outlives a delegate that was on the stack.
         *
         * @param heap The heap to allocate from, or null for the current one.
         * @return The copy, which the caller frees with pead::Delete.
         */
        virtual IDelegate1 *clone(Heap *heap) const = 0;
    };

    template<typename A1, typename A2>
    class IDelegate2
    {
    public:
        virtual ~IDelegate2()             = default;
        virtual void invoke(A1 a1, A2 a2) = 0;
        void operator()(A1 a1, A2 a2)
        {
            invoke(a1, a2);
        }
        virtual IDelegate2 *clone(Heap *heap) const = 0;
    };

    template<typename T>
    class Delegate : public IDelegate
    {
    public:
        using MethodPtr = void (T::*)();

        Delegate() : m_object(nullptr), m_method(nullptr)
        {}
        Delegate(T *object, MethodPtr method) : m_object(object), m_method(method)
        {}

        void bind(T *object, MethodPtr method)
        {
            m_object = object;
            m_method = method;
        }
        void invoke() override
        {
            if (m_object && m_method)
                (m_object->*m_method)();
        }
        IDelegate *clone(Heap *heap) const override
        {
            return new (heap) Delegate(*this);
        }

    private:
        T *m_object;
        MethodPtr m_method;
    };

    template<typename T, typename A1>
    class Delegate1 : public IDelegate1<A1>
    {
    public:
        using MethodPtr = void (T::*)(A1);

        Delegate1() : m_object(nullptr), m_method(nullptr)
        {}
        Delegate1(T *object, MethodPtr method) : m_object(object), m_method(method)
        {}

        void bind(T *object, MethodPtr method)
        {
            m_object = object;
            m_method = method;
        }
        void invoke(A1 a1) override
        {
            if (m_object && m_method)
                (m_object->*m_method)(a1);
        }
        IDelegate1<A1> *clone(Heap *heap) const override
        {
            return new (heap) Delegate1(*this);
        }

    private:
        T *m_object;
        MethodPtr m_method;
    };

    template<typename T, typename A1, typename A2>
    class Delegate2 : public IDelegate2<A1, A2>
    {
    public:
        using MethodPtr = void (T::*)(A1, A2);

        Delegate2() : m_object(nullptr), m_method(nullptr)
        {}
        Delegate2(T *object, MethodPtr method) : m_object(object), m_method(method)
        {}

        void bind(T *object, MethodPtr method)
        {
            m_object = object;
            m_method = method;
        }
        void invoke(A1 a1, A2 a2) override
        {
            if (m_object && m_method)
                (m_object->*m_method)(a1, a2);
        }
        IDelegate2<A1, A2> *clone(Heap *heap) const override
        {
            return new (heap) Delegate2(*this);
        }

    private:
        T *m_object;
        MethodPtr m_method;
    };

    /**
     * @brief This binds a free function, for callers that have no object.
     */
    template<typename A1>
    class StaticDelegate1 : public IDelegate1<A1>
    {
    public:
        using FuncPtr = void (*)(A1);

        explicit StaticDelegate1(FuncPtr func = nullptr) : m_func(func)
        {}

        void invoke(A1 a1) override
        {
            if (m_func)
                m_func(a1);
        }
        IDelegate1<A1> *clone(Heap *heap) const override
        {
            return new (heap) StaticDelegate1(*this);
        }

    private:
        FuncPtr m_func;
    };

    /**
     * @brief This calls every connected slot, in the order they were connected, when fired.
     */
    template<typename A>
    class DelegateEvent
    {
    public:
        /**
         * @brief A slot holds a delegate and links into one event at a time. It disconnects itself when destroyed.
         */
        class Slot
        {
        public:
            Slot() : m_delegate(nullptr), m_event(nullptr)
            {}
            explicit Slot(IDelegate1<A> *delegate) : m_delegate(delegate), m_event(nullptr)
            {}
            ~Slot()
            {
                release();
            }
            Slot(const Slot &)            = delete;
            Slot &operator=(const Slot &) = delete;

            void bind(IDelegate1<A> *delegate)
            {
                m_delegate = delegate;
            }
            void release()
            {
                if (m_event)
                    m_event->disconnect(this);
            }

        private:
            friend class DelegateEvent;

            ListNode m_node;
            IDelegate1<A> *m_delegate;
            DelegateEvent *m_event;
        };

        DelegateEvent() : m_slots((s32) offsetof(Slot, m_node))
        {}
        ~DelegateEvent()
        {
            while (Slot *slot = m_slots.popFront()) slot->m_event = nullptr;
        }
        DelegateEvent(const DelegateEvent &)            = delete;
        DelegateEvent &operator=(const DelegateEvent &) = delete;

        void connect(Slot &slot)
        {
            slot.release();
            m_slots.pushBack(&slot);
            slot.m_event = this;
        }
        void disconnect(Slot *slot)
        {
            if (slot->m_event == this)
            {
                m_slots.erase(slot);
                slot->m_event = nullptr;
            }
        }
        void fire(A arg)
        {
            for (Slot &slot : m_slots)
            {
                if (slot.m_delegate)
                    slot.m_delegate->invoke(arg);
            }
        }
        s32 getSlotLength() const
        {
            return m_slots.size();
        }

    private:
        OffsetList<Slot> m_slots;
    };

} // namespace pead
