#include "ToolsContext.h"
#include "EngineTools/Resource/ResourceDatabase.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Entity/EntityWorldManager.h"

//-------------------------------------------------------------------------

namespace EE
{
    FileSystem::Path const& ToolsContext::GetRawResourceDirectory() const { return m_pResourceDatabase->GetRawResourceDirectoryPath(); }
    FileSystem::Path const& ToolsContext::GetCompiledResourceDirectory() const { return m_pResourceDatabase->GetCompiledResourceDirectoryPath(); }

    //-------------------------------------------------------------------------

    Entity* ToolsContext::TryFindEntityInAllWorlds( EntityID ID ) const
    {
        Entity* pFoundEntity = nullptr;
        for ( EntityWorld const* pWorld : GetWorldManager()->GetWorlds() )
        {
            pFoundEntity = pWorld->FindEntity( ID );
            if ( pFoundEntity != nullptr )
            {
                break;
            }
        }

        return pFoundEntity;
    }

    TVector<EntityWorld const*> ToolsContext::GetAllWorlds() const
    {
        TVector<EntityWorld const*> debugWorlds;
        for ( EntityWorld const* pWorld : GetWorldManager()->GetWorlds() )
        {
            debugWorlds.emplace_back( pWorld );
        }
        return debugWorlds;
    }
}