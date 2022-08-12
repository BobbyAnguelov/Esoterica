#pragma once

#include "UUID.h"
#include "System/Log.h"
#include <EASTL/functional.h>

//-------------------------------------------------------------------------

namespace EE
{
    class EventBindingID
    {
        template<typename... Args> friend class TEvent;
        friend class SignalInternal;

    public:

        EventBindingID() = default;
        inline bool IsValid() const { return m_ID.IsValid(); }
        inline UUIDString ToString() const { return eastl::move( m_ID.ToString() ); }

    private:

        EventBindingID( UUID& newID ) : m_ID( newID ) {}
        EventBindingID( UUID&& newID ) : m_ID( newID ) {}
        inline void Reset() { m_ID.Clear(); }

    private:

        UUID    m_ID;
    };

    //-------------------------------------------------------------------------

    template<typename... Args>
    class TEventHandle;

    template<typename... Args>
    class TEvent
    {
        friend TEventHandle<Args...>;

        struct BoundUser
        {
            BoundUser( eastl::function<void( Args... )>&& function )
                : m_function( function ) 
            {}

            void Reset()
            {
                m_ID.Clear();
                m_function = nullptr;
            }

        public:

            UUID                                m_ID = UUID::GenerateID();
            eastl::function<void( Args... )>    m_function = nullptr;
        };

    public:

        ~TEvent()
        {
            if ( HasBoundUsers() )
            {
                EE_LOG_ERROR( "System", nullptr, "Event still has bound users at destruction" );
                EE_HALT();
            }
        }

        inline bool HasBoundUsers() const { return !m_boundUsers.empty(); }

        inline EventBindingID Bind( eastl::function<void( Args... )>&& function )
        {
            auto& boundUser = m_boundUsers.emplace_back( BoundUser( eastl::forward<eastl::function<void( Args... )>&&>( function ) ) );
            boundUser.m_function = function;
            return EventBindingID( boundUser.m_ID );
        }

        inline void Unbind( EventBindingID bindingID )
        {
            auto searchPredicate = [] ( BoundUser const& boundUser, EventBindingID const& bindingID ) { return boundUser.m_ID == bindingID.m_ID; };
            auto foundIter = eastl::find( m_boundUsers.begin(), m_boundUsers.end(), bindingID, searchPredicate );

            EE_ASSERT( foundIter != m_boundUsers.end() );
            m_boundUsers.erase( foundIter );
            bindingID.Reset();

            // Always free memory when we completely empty the array (this is need for statically created global events since the allocators are released before the events are destroyed)
            if ( m_boundUsers.empty() )
            {
                m_boundUsers.shrink_to_fit();
            }
        }

        //-------------------------------------------------------------------------

        inline void Execute( Args... args ) const
        {
            for ( auto& boundUser : m_boundUsers )
            {
                EE_ASSERT( boundUser.m_function != nullptr );
                boundUser.m_function( std::forward<Args>( args )... );
            }
        }

    private:

        TVector<BoundUser>              m_boundUsers;
    };

    //-------------------------------------------------------------------------

    // Safety Helper to return events from classes without the risk of the client copying them or firing them
    template< typename... Args>
    class TEventHandle
    {
    public:

        TEventHandle( TEvent<Args...>& event ) : m_pEvent( &event ) {}
        TEventHandle( TEvent<Args...>* event ) : m_pEvent( event ) {}

        [[nodiscard]] inline EventBindingID Bind( eastl::function<void( Args... )>&& function )
        {
            return m_pEvent->Bind( eastl::move( function ) );
        }

        inline void Unbind( EventBindingID& handle )
        {
            m_pEvent->Unbind( handle );
        }

    private:

        TEvent<Args...>* m_pEvent = nullptr;
    };
}