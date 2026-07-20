#include "EntityWorldManager.h"
#include "EntityWorld.h"
#include "EntityLog.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "Engine/UpdateContext.h"
#include "Base/Systems.h"

//-------------------------------------------------------------------------

namespace EE
{
    EntityWorldManager::~EntityWorldManager()
    {
        EE_ASSERT( m_worlds.empty() && m_worldSystemTypeInfos.empty() );
    }

    void EntityWorldManager::Initialize( SystemRegistry const& systemsRegistry )
    {
        m_pSystemsRegistry = &systemsRegistry;

        //-------------------------------------------------------------------------

        auto pTypeRegistry = systemsRegistry.GetSystem<TypeSystem::TypeRegistry>();
        EE_ASSERT( pTypeRegistry != nullptr );
        m_worldSystemTypeInfos = pTypeRegistry->GetAllDerivedLeafTypes( EntityWorldSystem::GetStaticTypeID(), false, false, true );

        // Create a game world
        //-------------------------------------------------------------------------

        CreateWorld( EntityWorldType::Game );
    }

    void EntityWorldManager::Shutdown()
    {
        for ( auto& pWorld : m_worlds )
        {
            pWorld->Shutdown();
            EE::Delete( pWorld );
        }
        m_worlds.clear();

        //-------------------------------------------------------------------------

        m_worldSystemTypeInfos.clear();
        m_pSystemsRegistry = nullptr;
    }

    //-------------------------------------------------------------------------

    void EntityWorldManager::StartFrame()
    {
        #if EE_DEVELOPMENT_TOOLS
        for ( auto& pWorld : m_worlds )
        {
            pWorld->ResetDebugDrawingSystem();
        }
        #endif
    }

    void EntityWorldManager::EndFrame()
    {
        // Run loading phase here to ensure that any un-registration and re-registration requests do not take multiple updates
        UpdateLoading();
    }

    //-------------------------------------------------------------------------

    EntityWorld* EntityWorldManager::GetGameWorld()
    {
        for ( auto const& pWorld : m_worlds )
        {
            if ( pWorld->IsGameWorld() )
            {
                return pWorld;
            }
        }

        return nullptr;
    }

    EntityWorld* EntityWorldManager::CreateWorld( EntityWorldType worldType )
    {
        EE_ASSERT( m_pSystemsRegistry != nullptr );

        //-------------------------------------------------------------------------

        // Only a single game world is allowed
        if ( worldType == EntityWorldType::Game )
        {
            if ( GetGameWorld() != nullptr )
            {
                EE_UNREACHABLE_CODE();
                return nullptr;
            }
        }

        //-------------------------------------------------------------------------

        auto pNewWorld = EE::New<EntityWorld>( worldType );
        pNewWorld->Initialize( *m_pSystemsRegistry, m_worldSystemTypeInfos );
        m_worlds.emplace_back( pNewWorld );

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        if ( worldType == EntityWorldType::Game )
        {
            pNewWorld->SetDebugName( "Game" );
        }
        else
        {
            pNewWorld->SetDebugName( "Editor" );
        }
        #endif

        return pNewWorld;
    }

    void EntityWorldManager::DestroyWorld( EntityWorld* pWorld )
    {
        EE_ASSERT( Threading::IsMainThread() );

        // Remove world from worlds list
        auto foundWorldIter = eastl::find( m_worlds.begin(), m_worlds.end(), pWorld );
        EE_ASSERT( foundWorldIter != m_worlds.end() );
        m_worlds.erase( foundWorldIter );

        // Shutdown and destroy world
        pWorld->Shutdown();
        EE::Delete( pWorld );
    }

    //-------------------------------------------------------------------------

    bool EntityWorldManager::IsBusyLoading() const
    {
        for ( auto const& pWorld : m_worlds )
        {
            if ( pWorld->IsBusyLoading() )
            {
                return true;
            }
        }

        return false;
    }

    void EntityWorldManager::UpdateLoading()
    {
        for ( EntityWorld* const& pWorld : m_worlds )
        {
            pWorld->UpdateLoading();
        }
    }

    void EntityWorldManager::UpdateWorlds( UpdateContext const& context )
    {
        //-------------------------------------------------------------------------
        // World Update
        //-------------------------------------------------------------------------

        for ( auto const& pWorld : m_worlds )
        {
            if ( pWorld->IsSuspended() )
            {
                continue;
            }

            // Run world updates
            //-------------------------------------------------------------------------

            pWorld->Update( context );
        }

        //-------------------------------------------------------------------------
        // Handle Entity Log
        //-------------------------------------------------------------------------

        if ( context.GetUpdateStage() == UpdateStage::FrameEnd )
        {
            #if EE_DEVELOPMENT_TOOLS
            auto queuedLogRequests = EntityModel::RetrieveQueuedLogRequests();
            for ( auto const& request : queuedLogRequests )
            {
                // Resolve all IDs
                //-------------------------------------------------------------------------

                EntityWorld const* pFoundWorld = nullptr;
                Entity const* pFoundEntity = nullptr;
                EntityComponent const* pFoundComponent = nullptr;

                for ( auto pWorld : m_worlds )
                {
                    pFoundEntity = pWorld->FindEntity( request.m_entityID );
                    if ( pFoundEntity != nullptr )
                    {
                        pFoundWorld = pWorld;
                        if ( request.m_componentID.IsValid() )
                        {
                            pFoundComponent = pFoundEntity->FindComponent( request.m_componentID );
                        }
                        break;
                    }
                }

                EE_ASSERT( pFoundWorld != nullptr );

                // Create source info
                //-------------------------------------------------------------------------

                InlineString sourceInfoStr;

                if ( pFoundComponent == nullptr )
                {
                    sourceInfoStr.sprintf( "W: %s, E: %s", pFoundWorld->GetDebugName().c_str(), pFoundEntity->GetNameID().c_str() );
                }
                else
                {
                    sourceInfoStr.sprintf( "W: %s, E: %s, C: %s", pFoundWorld->GetDebugName().c_str(), pFoundEntity->GetNameID().c_str(), pFoundComponent->GetNameID().c_str() );
                }

                // Add Log
                //-------------------------------------------------------------------------

                SystemLog::AddEntry( request.m_severity, request.m_category.c_str(), sourceInfoStr.c_str(), request.m_filename.c_str(), request.m_lineNumber, request.m_message.c_str());
            }
            #endif
        }
    }

    #if EE_DEVELOPMENT_TOOLS
    void EntityWorldManager::DebugDrawWorlds( UpdateContext const& context )
    {
        for ( auto const& pWorld : m_worlds )
        {
            if ( pWorld->IsSuspended() )
            {
                continue;
            }

            pWorld->DebugDraw( context );
        }
    }
    #endif

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void EntityWorldManager::HotReload_UnloadEntities( TInlineVector<Resource::ResourceRequesterID, 20> const& usersToReload )
    {
        for ( auto const& pWorld : m_worlds )
        {
            pWorld->HotReload_UnloadEntities( usersToReload );
        }
    }

    void EntityWorldManager::HotReload_ReloadEntities( TInlineVector<Resource::ResourceRequesterID, 20> const& usersToReload )
    {
        for ( auto const& pWorld : m_worlds )
        {
            pWorld->HotReload_ReloadEntities( usersToReload );
        }
    }
    #endif
} 