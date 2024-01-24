#include "EntityWorld.h"
#include "EntityWorldUpdateContext.h"
#include "EntityWorldSettings.h"
#include "Base/Resource/ResourceSystem.h"
#include "Base/Profiling.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include <eastl/sort.h>

//-------------------------------------------------------------------------

namespace EE
{
    EntityWorld::EntityWorld( EntityWorldType worldType )
        : m_initializationContext( m_worldSystems, m_entityUpdateList )
        , m_worldType( worldType )
    {}

    EntityWorld::~EntityWorld()
    {
        EE_ASSERT( m_maps.empty());
        EE_ASSERT( m_worldSystems.empty() );
        EE_ASSERT( m_entityUpdateList.empty() );

        for ( int8_t i = 0; i < (int8_t) UpdateStage::NumStages; i++ )
        {
            EE_ASSERT( m_systemUpdateLists[i].empty() );
        }

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        for ( auto& v : m_componentTypeLookup )
        {
            EE_ASSERT( v.second.empty() );
        }
        #endif
    }

    void EntityWorld::Initialize( SystemRegistry const& systemsRegistry, TVector<TypeSystem::TypeInfo const*> worldSystemTypeInfos )
    {
        m_pTaskSystem = systemsRegistry.GetSystem<TaskSystem>();
        EE_ASSERT( m_pTaskSystem != nullptr );

        // Set up Contexts
        //-------------------------------------------------------------------------

        m_loadingContext = EntityModel::LoadingContext( m_pTaskSystem, systemsRegistry.GetSystem<TypeSystem::TypeRegistry>(), systemsRegistry.GetSystem<Resource::ResourceSystem>() );
        EE_ASSERT( m_loadingContext.IsValid() );

        //-------------------------------------------------------------------------

        const_cast<TaskSystem*&>( m_initializationContext.m_pTaskSystem ) = m_pTaskSystem;
        const_cast<TypeSystem::TypeRegistry const*&>( m_initializationContext.m_pTypeRegistry ) = m_loadingContext.m_pTypeRegistry;
        
        #if EE_DEVELOPMENT_TOOLS
        m_initializationContext.SetComponentTypeMapPtr( &m_componentTypeLookup );
        #endif

        EE_ASSERT( m_initializationContext.IsValid() );

        // Create World Systems
        //-------------------------------------------------------------------------

        for ( auto pTypeInfo : worldSystemTypeInfos )
        {
            // Create and initialize world system
            auto pWorldSystem = Cast<EntityWorldSystem>( pTypeInfo->CreateType() );
            pWorldSystem->m_pWorld = this;
            pWorldSystem->InitializeSystem( systemsRegistry );
            m_worldSystems.push_back( pWorldSystem );

            // Add to update lists
            for ( int8_t i = 0; i < (int8_t) UpdateStage::NumStages; i++ )
            {
                if ( pWorldSystem->GetRequiredUpdatePriorities().IsStageEnabled( (UpdateStage) i ) )
                {
                    m_systemUpdateLists[i].push_back( pWorldSystem );
                }

                // Sort update list
                auto comparator = [i] ( EntityWorldSystem* const& pSystemA, EntityWorldSystem* const& pSystemB )
                {
                    uint8_t const A = pSystemA->GetRequiredUpdatePriorities().GetPriorityForStage( (UpdateStage) i );
                    uint8_t const B = pSystemB->GetRequiredUpdatePriorities().GetPriorityForStage( (UpdateStage) i );
                    return A > B;
                };

                eastl::sort( m_systemUpdateLists[i].begin(), m_systemUpdateLists[i].end(), comparator );
            }
        }

        // Create World Settings
        //-------------------------------------------------------------------------

        EE_ASSERT( m_pSettingsRegistry == nullptr );
        m_pSettingsRegistry = systemsRegistry.GetSystem<Settings::SettingsRegistry>();
        m_pSettingsRegistry->CreateGroup( m_worldID.m_value );

        TVector<TypeSystem::TypeInfo const*> settingsTypes = m_loadingContext.m_pTypeRegistry->GetAllDerivedTypes( IEntityWorldSettings::GetStaticTypeID(), false, false, false );
        for ( auto pTypeInfo : settingsTypes )
        {
            m_pSettingsRegistry->CreateSettings( m_worldID.m_value, pTypeInfo );
        }

        // Create and initialize the persistent map
        //-------------------------------------------------------------------------

        m_maps.emplace_back( EE::New<EntityModel::EntityMap>() );
        m_maps[0]->Load( m_loadingContext, m_initializationContext );

        //-------------------------------------------------------------------------

        m_initialized = true;
    }

    void EntityWorld::Shutdown()
    {
        // Unload maps
        //-------------------------------------------------------------------------
        
        for ( auto& pMap : m_maps )
        {
            pMap->Unload( m_loadingContext, m_initializationContext );
        }

        // Run the loading update as this will immediately unload all maps
        //-------------------------------------------------------------------------

        while ( !m_maps.empty() )
        {
            UpdateLoading();
        }

        // Destroy all settings
        //-------------------------------------------------------------------------

        m_pSettingsRegistry->DestroyGroup( m_worldID.m_value );
        m_pSettingsRegistry = nullptr;

        // Shutdown all world systems
        //-------------------------------------------------------------------------

        for( auto pWorldSystem : m_worldSystems )
        {
            // Remove from update lists
            for ( int8_t i = 0; i < (int8_t) UpdateStage::NumStages; i++ )
            {
                if ( pWorldSystem->GetRequiredUpdatePriorities().IsStageEnabled( (UpdateStage) i ) )
                {
                    auto updateIter = VectorFind( m_systemUpdateLists[i], pWorldSystem );
                    EE_ASSERT( updateIter != m_systemUpdateLists[i].end() );
                    m_systemUpdateLists[i].erase( updateIter );
                }
            }

            // Shutdown and destroy world system
            pWorldSystem->ShutdownSystem();
            EE::Delete( pWorldSystem );
        }

        m_worldSystems.clear();

        //-------------------------------------------------------------------------

        m_pTaskSystem = nullptr;
        m_initialized = false;
    }

    //-------------------------------------------------------------------------
    // Misc
    //-------------------------------------------------------------------------

    EntityWorldSystem* EntityWorld::GetWorldSystem( uint32_t worldSystemID ) const
    {
        for ( EntityWorldSystem* pWorldSystem : m_worldSystems )
        {
            if ( pWorldSystem->GetSystemID() == worldSystemID )
            {
                return pWorldSystem;
            }
        }

        EE_UNREACHABLE_CODE();
        return nullptr;
    }

    //-------------------------------------------------------------------------
    // Frame Update
    //-------------------------------------------------------------------------

    void EntityWorld::UpdateLoading()
    {
        EE_PROFILE_SCOPE_ENTITY( "World Loading" );

        // Update all maps internal loading state
        //-------------------------------------------------------------------------
        // This will fill the world initialization/registration lists used below
        // This will also handle all hot-reload unload/load requests

        for ( int32_t i = (int32_t) m_maps.size() - 1; i >= 0; i-- )
        {
            if ( m_maps[i]->UpdateLoadingAndStateChanges( m_loadingContext, m_initializationContext ) )
            {
                if ( m_maps[i]->IsUnloaded() )
                {
                    EE::Delete( m_maps[i] );
                    m_maps.erase_unsorted( m_maps.begin() + i );
                }
            }
        }
    }

    void EntityWorld::Update( UpdateContext const& context )
    {
        EE_ASSERT( Threading::IsMainThread() );
        EE_ASSERT( !m_isSuspended );

        struct EntityUpdateTask final : public ITaskSet
        {
            EntityUpdateTask( EntityWorldUpdateContext const& context, TVector<Entity*>& updateList )
                : m_context( context )
                , m_updateList( updateList )
            {
                m_SetSize = (uint32_t) updateList.size();
            }

            // Only used for spatial dependency chain updates
            inline void RecursiveEntityUpdate( Entity* pEntity )
            {
                pEntity->UpdateSystems( m_context );

                for ( auto pAttachedEntity : pEntity->GetAttachedEntities() )
                {
                    RecursiveEntityUpdate( pAttachedEntity );
                }
            }

            virtual void ExecuteRange( TaskSetPartition range, uint32_t threadnum ) override final
            {
                for ( uint64_t i = range.start; i < range.end; ++i )
                {
                    auto pEntity = m_updateList[i];

                    // Ignore any entities with spatial parents, these will be updated by their parents
                    if ( pEntity->HasSpatialParent() )
                    {
                        continue;
                    }

                    //-------------------------------------------------------------------------

                    if ( pEntity->HasAttachedEntities() )
                    {
                        EE_PROFILE_SCOPE_ENTITY( "Update Entity Chain" );
                        RecursiveEntityUpdate( pEntity );
                    }
                    else // Direct entity update
                    {
                        EE_PROFILE_SCOPE_ENTITY( "Update Entity" );
                        pEntity->UpdateSystems( m_context );
                    }
                }
            }

        private:

            EntityWorldUpdateContext const&              m_context;
            TVector<Entity*>&                            m_updateList;
        };

        //-------------------------------------------------------------------------

        UpdateStage const updateStage = context.GetUpdateStage();
        bool const isWorldPaused = IsPaused() && !m_timeStepRequested;

        // Skip all non-pause updates for paused worlds
        if ( isWorldPaused && updateStage != UpdateStage::Paused )
        {
            return;
        }

        // Skip paused update for non-paused worlds
        if ( context.GetUpdateStage() == UpdateStage::Paused && !isWorldPaused )
        {
            return;
        }

        //-------------------------------------------------------------------------

        EntityWorldUpdateContext entityWorldUpdateContext( context, this );

        // Update entities
        //-------------------------------------------------------------------------

        EntityUpdateTask entityUpdateTask( entityWorldUpdateContext, m_entityUpdateList );
        m_pTaskSystem->ScheduleTask( &entityUpdateTask );
        m_pTaskSystem->WaitForTask( &entityUpdateTask );

        // Force execution on main thread for debugging purposes
        //entityUpdateTask.ExecuteRange( { 0u, (uint32_t) m_entityUpdateList.size() }, 0 );

        // Update systems
        //-------------------------------------------------------------------------

        for ( auto pSystem : m_systemUpdateLists[(int8_t) updateStage] )
        {
            EE_PROFILE_SCOPE_ENTITY( "Update World Systems" );
            EE_ASSERT( pSystem->GetRequiredUpdatePriorities().IsStageEnabled( updateStage ) );
            pSystem->UpdateSystem( entityWorldUpdateContext );
        }

        //-------------------------------------------------------------------------

        if ( updateStage == UpdateStage::FrameEnd )
        {
            m_timeStepRequested = false;
        }
    }

    //-------------------------------------------------------------------------
    // Maps
    //-------------------------------------------------------------------------

    bool EntityWorld::IsBusyLoading() const
    {
        for ( auto const& pMap : m_maps )
        {
            if( pMap->IsLoading() ) 
            {
                return true;
            }
        }

        return false;
    }

    bool EntityWorld::HasMap( ResourceID const& mapResourceID ) const
    {
        for ( auto const& pMap : m_maps )
        {
            if ( pMap->GetMapResourceID() == mapResourceID )
            {
                return true;
            }
        }

        return false;
    }

    bool EntityWorld::HasMap( EntityMapID const& mapID ) const
    {
        for ( auto const& pMap : m_maps )
        {
            if ( pMap->GetID() == mapID )
            {
                return true;
            }
        }

        return false;
    }

    bool EntityWorld::IsMapLoaded( ResourceID const& mapResourceID ) const
    {
        // Make sure the map isn't already loaded or loading, since duplicate loads are not allowed
        for ( auto const& pMap : m_maps )
        {
            if ( pMap->GetMapResourceID() == mapResourceID )
            {
                return pMap->IsLoaded();
            }
        }

        // Dont call this function with an unknown map
        EE_UNREACHABLE_CODE();
        return false;
    }

    bool EntityWorld::IsMapLoaded( EntityMapID const& mapID ) const
    {
        // Make sure the map isn't already loaded or loading, since duplicate loads are not allowed
        for ( auto const& pMap : m_maps )
        {
            if ( pMap->GetID() == mapID )
            {
                return pMap->IsLoaded();
            }
        }

        // Dont call this function with an unknown map
        EE_UNREACHABLE_CODE();
        return false;
    }

    EntityModel::EntityMap* EntityWorld::CreateTransientMap()
    {
        EntityModel::EntityMap* pNewMap = m_maps.emplace_back( EE::New<EntityModel::EntityMap>() );
        pNewMap->Load( m_loadingContext, m_initializationContext );
        return pNewMap;
    }

    EntityModel::EntityMap const* EntityWorld::GetMap( ResourceID const& mapResourceID ) const
    {
        EE_ASSERT( mapResourceID.IsValid() && mapResourceID.GetResourceTypeID() == EntityModel::SerializedEntityMap::GetStaticResourceTypeID() );
        auto const foundMapIter = VectorFind( m_maps, mapResourceID, [] ( EntityModel::EntityMap const* pMap, ResourceID const& mapResourceID ) { return pMap->GetMapResourceID() == mapResourceID; } );
        EE_ASSERT( foundMapIter != m_maps.end() );
        return *foundMapIter;
    }

    EntityModel::EntityMap const* EntityWorld::GetMap( EntityMapID const& mapID ) const
    {
        EE_ASSERT( mapID.IsValid() );
        auto const foundMapIter = VectorFind( m_maps, mapID, [] ( EntityModel::EntityMap const* pMap, EntityMapID const& mapID ) { return pMap->GetID() == mapID; } );
        EE_ASSERT( foundMapIter != m_maps.end() );
        return *foundMapIter;
    }

    EntityModel::EntityMap const* EntityWorld::GetMapForEntity( Entity const* pEntity ) const
    {
        EE_ASSERT( pEntity != nullptr && pEntity->IsAddedToMap() );
        auto const& mapID = pEntity->GetMapID();
        auto pMap = GetMap( mapID );
        EE_ASSERT( pMap != nullptr );
        return pMap;
    }

    EntityMapID EntityWorld::LoadMap( ResourceID const& mapResourceID )
    {
        EE_ASSERT( mapResourceID.IsValid() && mapResourceID.GetResourceTypeID() == EntityModel::SerializedEntityMap::GetStaticResourceTypeID() );

        EE_ASSERT( !HasMap( mapResourceID ) );
        auto pNewMap = m_maps.emplace_back( EE::New<EntityModel::EntityMap>( mapResourceID ) );
        pNewMap->Load( m_loadingContext, m_initializationContext );
        return pNewMap->GetID();
    }

    void EntityWorld::UnloadMap( ResourceID const& mapResourceID )
    {
        EE_ASSERT( mapResourceID.IsValid() && mapResourceID.GetResourceTypeID() == EntityModel::SerializedEntityMap::GetStaticResourceTypeID() );

        auto const foundMapIter = VectorFind( m_maps, mapResourceID, [] ( EntityModel::EntityMap const* pMap, ResourceID const& mapResourceID ) { return pMap->GetMapResourceID() == mapResourceID; } );
        EE_ASSERT( foundMapIter != m_maps.end() );
        ( *foundMapIter )->Unload( m_loadingContext, m_initializationContext );
    }

    //-------------------------------------------------------------------------
    // Editing / Hot Reload
    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS

    void EntityWorld::BeginComponentEdit( Entity* pEntity )
    {
        EE_ASSERT( pEntity != nullptr );

        auto pMap = GetMap( pEntity->GetMapID() );
        EE_ASSERT( pMap != nullptr );
        pMap->BeginComponentEdit( m_loadingContext, m_initializationContext, pEntity->GetID() );
    }

    void EntityWorld::EndComponentEdit( Entity* pEntity )
    {
        EE_ASSERT( pEntity != nullptr );

        auto pMap = GetMap( pEntity->GetMapID() );
        EE_ASSERT( pMap != nullptr );
        pMap->EndComponentEdit( m_loadingContext, m_initializationContext, pEntity->GetID() );
    }

    void EntityWorld::BeginComponentEdit( EntityComponent* pComponent )
    {
        EE_ASSERT( pComponent != nullptr );

        auto pEntity = FindEntity( pComponent->GetEntityID() );
        EE_ASSERT( pEntity != nullptr );

        auto pMap = GetMap( pEntity->GetMapID() );
        EE_ASSERT( pMap != nullptr );
        pMap->BeginComponentEdit( m_loadingContext, m_initializationContext, pEntity->GetID() );
    }

    void EntityWorld::EndComponentEdit( EntityComponent* pComponent )
    {
        EE_ASSERT( pComponent != nullptr );

        auto pEntity = FindEntity( pComponent->GetEntityID() );
        EE_ASSERT( pEntity != nullptr );

        auto pMap = GetMap( pEntity->GetMapID() );
        EE_ASSERT( pMap != nullptr );
        pMap->EndComponentEdit( m_loadingContext, m_initializationContext, pEntity->GetID() );
    }

    //-------------------------------------------------------------------------

    void EntityWorld::HotReload_UnloadEntities( TVector<Resource::ResourceRequesterID> const& usersToReload )
    {
        EE_ASSERT( !usersToReload.empty() );
        for ( auto& pMap : m_maps )
        {
            pMap->HotReload_UnloadEntities( m_loadingContext, m_initializationContext, usersToReload );
        }
    }

    void EntityWorld::HotReload_ReloadEntities()
    {
        for ( auto& pMap : m_maps )
        {
            pMap->HotReload_ReloadEntities( m_loadingContext );
        }
    }
    #endif
}