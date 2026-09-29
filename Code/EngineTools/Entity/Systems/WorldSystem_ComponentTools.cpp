#include "WorldSystem_ComponentTools.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    void ComponentToolsSystem::RegisterComponent( Entity* pEntity, EntityComponent* pComponent )
    {
        m_manager.RegisterComponent( pEntity, pComponent );
    }

    void ComponentToolsSystem::UnregisterComponent( Entity* pEntity, EntityComponent* pComponent )
    {
        m_manager.UnregisterComponent( pEntity, pComponent );
    }
}