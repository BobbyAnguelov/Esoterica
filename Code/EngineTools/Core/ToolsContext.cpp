#include "ToolsContext.h"
#include "EngineTools/FileSystem/FileRegistry.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Entity/EntityWorldManager.h"

//-------------------------------------------------------------------------

namespace EE
{
    FileSystem::Path const& ToolsContext::GetSourceDataDirectory() const { return m_pFileRegistry->GetSourceDataDirectoryPath(); }
    FileSystem::Path const& ToolsContext::GetCompiledResourceDirectory() const { return m_pFileRegistry->GetCompiledResourceDirectoryPath(); }

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