#pragma once

#include "Base/Threading/Threading.h"
#include "EntityComponent.h"

//-------------------------------------------------------------------------
// Entity World System Signaling
//-------------------------------------------------------------------------
// This pattern allows for components to efficiently signal state changes or requests to world systems
// The component needs to have a TEntityWorldSystemSignal<T> internally which the world system can register itself to
// All signals will go onto a lock-free queue that the world system can then deque

namespace EE
{
    template<typename T>
    class TEntityWorldSystemSignal;

    template<typename T>
    class TEntityMessageQueue
    {
        friend class TEntityWorldSystemSignal<T>;

    public:

        struct Message
        {
            Message() = default;

            Message( T* pComponent, uint64_t extraData = 0 )
                : m_pComponent( pComponent )
                , m_extraData()
            {}

        public:

            T*                  m_pComponent = nullptr;
            uint64_t            m_extraData = 0;
        };

    public:

        // Get the approximate size of the queue
        EE_FORCE_INLINE size_t GetSize() const { return m_queue.size_approx(); }

        // Is the queue empty?
        EE_FORCE_INLINE bool IsEmpty() const { return m_queue.size_approx() == 0; }

        // Bind this queue to a signal
        void Bind( T* pComponent, TEntityWorldSystemSignal<T>* pSignal )
        {
            Threading::ScopeLock const sl( pSignal->m_mutex );
            EE_ASSERT( !VectorContains( pSignal->m_registeredQueues, this ) );
            pSignal->m_registeredQueues.emplace_back( this );

            m_ignoredComponents.erase_first_unsorted( pComponent );
        }

        // Unbind this queue from a signal, automatically add the component to the ignored components list
        void Unbind( T* pComponent, TEntityWorldSystemSignal<T>* pSignal )
        {
            Threading::ScopeLock const sl( pSignal->m_mutex );

            auto foundIter = eastl::find( pSignal->m_registeredQueues.begin(), pSignal->m_registeredQueues.end(), this );
            EE_ASSERT( foundIter != pSignal->m_registeredQueues.end() );
            pSignal->m_registeredQueues.erase( foundIter );

            m_ignoredComponents.emplace_back( pComponent );
        }

        // Enqueue message
        void Enqueue( T* pComponent, uint64_t extraData = 0 )
        {
            m_queue.enqueue( Message( pComponent, extraData ) );
        }

        // Try to dequeue a signal message, returns false when the queue is empty
        bool Dequeue( Message& msg );

        // Ignore all messages from this component, this is needed when unregistered components that might have queued messages
        // WARNING: Not threadsafe!!!!
        void AddComponentToIgnore( T* pComponent )
        {
            m_ignoredComponents.emplace_back( pComponent );
        }

        // Clear the list of the components to be ignored, only do that once you've emptied the full list of messages
        // WARNING: Not threadsafe!!!!
        void ClearIgnoredComponents() 
        {
            EE_ASSERT( m_queue.size_approx() == 0 );
            m_ignoredComponents.clear();
        }

    private:

        Threading::TLockFreeQueue<Message>                  m_queue;
        TInlineVector<T*, 10>                               m_ignoredComponents;
    };

    //-------------------------------------------------------------------------

    template<typename T>
    class TEntityWorldSystemSignal
    {
        friend class TEntityMessageQueue<T>;

    public:

        void Send( T* pComponent, uint64_t extraData = 0 )
        {
            for ( auto& pQueue : m_registeredQueues )
            {
                pQueue->Enqueue( pComponent, extraData );
            }
        }

    private:

        TInlineVector<TEntityMessageQueue<T>*, 2>           m_registeredQueues;
        Threading::Mutex                                    m_mutex;
    };

    //-------------------------------------------------------------------------

    template<typename T>
    bool EE::TEntityMessageQueue<T>::Dequeue( Message& msg )
    {
        bool dequeueSuccess = m_queue.try_dequeue( msg );
        while ( dequeueSuccess )
        {
            // Is this a valid message
            if ( !VectorContains( m_ignoredComponents, msg.m_pComponent ) )
            {
                return true;
            }

            dequeueSuccess = m_queue.try_dequeue( msg );
        }

        return false;
    }
}