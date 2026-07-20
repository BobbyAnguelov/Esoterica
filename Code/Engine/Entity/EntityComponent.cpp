#include "EntityComponent.h"

//-------------------------------------------------------------------------

namespace EE
{
    EntityComponent::~EntityComponent()
    {
        EE_ASSERT( m_status == Status::Unloaded );
        EE_ASSERT( m_isRegisteredWithEntity == false && m_isRegisteredWithWorld == false );
    }

    void EntityComponent::RequestRuntimeResourceChange( TFunction<void()>&& stateChangeFunction )
    {
        if ( IsUnloaded() )
        {
            stateChangeFunction();
            return;
        }

        EE_ASSERT( m_componentResourceStateChangeFunction != nullptr );
        m_componentResourceStateChangeFunction( this, eastl::forward<TFunction<void()>>( stateChangeFunction ) );
    }
}