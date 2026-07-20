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

    ToolsContext::EntityLookupResult ToolsContext::TryFindEntityInAllWorlds( EntityID ID ) const
    {
        EntityLookupResult result;

        for ( EntityWorld* pWorld : GetWorldManager()->GetWorlds() )
        {
            result.m_pEntity = pWorld->FindEntity( ID );
            if ( result.m_pEntity != nullptr )
            {
                result.m_pWorld = pWorld;
                break;
            }
        }

        return result;
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