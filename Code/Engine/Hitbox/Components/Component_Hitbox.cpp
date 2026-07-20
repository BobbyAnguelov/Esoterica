#include "Component_Hitbox.h"

//-------------------------------------------------------------------------

namespace EE
{
    void HitboxComponent::Initialize()
    {
        EntityComponent::Initialize();

        if ( m_hitboxDefinition.IsSet() && m_hitboxDefinition.IsLoaded() )
        {
            m_pHitbox = EE::New<Hitbox>( m_hitboxDefinition.GetPtr() );
        }
    }

    void HitboxComponent::Shutdown()
    {
        if ( m_pHitbox != nullptr )
        {
            EE::Delete( m_pHitbox );
        }

        EntityComponent::Shutdown();
    }
}