#include "WorldSystem_CoverManager.h"
#include "Game/Cover/Components/Component_CoverVolume.h"
#include "Engine/Entity/Entity.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/Entity/EntityMap.h"

//-------------------------------------------------------------------------

namespace EE
{
    void CoverManager::ShutdownSystem()
    {
        EE_ASSERT( m_coverVolumes.empty() );
    }

    void CoverManager::RegisterComponent( Entity const* pEntity, EntityComponent* pComponent )
    {
        if ( auto pCoverComponent = TryCast<CoverVolumeComponent>( pComponent ) )
        {
            m_coverVolumes.Add( pCoverComponent );
        }
    }

    void CoverManager::UnregisterComponent( Entity const* pEntity, EntityComponent* pComponent )
    {
        if ( auto pCoverComponent = TryCast<CoverVolumeComponent>( pComponent ) )
        {
            m_coverVolumes.Remove( pCoverComponent->GetID() );
        }
    }

    //-------------------------------------------------------------------------

    void CoverManager::UpdateSystem( EntityWorldUpdateContext const& ctx )
    {
    }
}