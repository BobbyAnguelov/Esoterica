#include "EntityWorld.h"
#include "EntityWorldUpdateContext.h"
#include "EntityWorldDebugView.h"
#include "System/Resource/ResourceSystem.h"
#include "System/Profiling.h"
#include "System/TypeSystem/TypeRegistry.h"
#include <eastl/sort.h>

//-------------------------------------------------------------------------

namespace EE
{
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
        EE_ASSERT( m_debugViews.empty() );

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

        m_loadingContext = EntityModel::EntityLoadingContext( m_pTaskSystem, systemsRegistry.GetSystem<TypeSystem::TypeRegistry>(), systemsRegistry.GetSystem<Resource::ResourceSystem>() );
        EE_ASSERT( m_loadingContext.IsValid() );

        m_activationContext = EntityModel::ActivationContext( m_pTaskSystem );
        EE_ASSERT( m_activationContext.IsValid() );

        // Create World Systems
        //-------------------------------------------------------------------------

        for ( auto pTypeInfo : worldSystemTypeInfos )
        {
            // Create and initialize world system
            auto pWorldSystem = Cast<IEntityWorldSystem>( pTypeInfo->CreateType() );
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
                auto comparator = [i] ( IEntityWorldSystem* const& pSystemA, IEntityWorldSystem* const& pSystemB )
                {
                    uint8_t const A = pSystemA->GetRequiredUpdatePriorities().GetPriorityForStage( (UpdateStage) i );
                    uint8_t const B = pSystemB->GetRequiredUpdatePriorities().GetPriorityForStage( (UpdateStage) i );
                    return A > B;
                };

                eastl::sort( m_systemUpdateLists[i].begin(), m_systemUpdateLists[i].end(), comparator );
            }
        }

        // Create and activate the persistent map
        //-------------------------------------------------------------------------

        m_maps.emplace_back( EntityModel::EntityMap() );
        m_maps[0].Load( m_loadingContext );
        m_maps[0].Activate( m_activationContext );

        //-------------------------------------------------------------------------

        m_initialized = true;
    }

    void EntityWorld::Shutdown()
    {
        // Unload maps
        //-------------------------------------------------------------------------
        
        for ( auto& map : m_maps )
        {
            map.Unload( m_loadingContext );
        }

        // Run the loading update as this will immediately unload all maps
        //-------------------------------------------------------------------------

        while ( !m_maps.empty() )
        {
            UpdateLoading();
        }

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
    // Debug Views
    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void EntityWorld::InitializeDebugViews( SystemRegistry const& systemsRegistry, TVector<TypeSystem::TypeInfo const*> debugViewTypeInfos )
    {
        for ( auto pTypeInfo : debugViewTypeInfos )
        {
            auto pDebugView = Cast<EntityWorldDebugView>( pTypeInfo->CreateType() );
            pDebugView->Initialize( systemsRegistry, this );
            m_debugViews.push_back( pDebugView );
        }
    }

    void EntityWorld::ShutdownDebugViews()
    {
        for ( auto pDebugView : m_debugViews )
        {
            // Shutdown and destroy world system
            pDebugView->Shutdown();
            EE::Delete( pDebugView );
        }

        m_debugViews.clear();
    }
    #endif

    //-------------------------------------------------------------------------
    // Misc
    //-------------------------------------------------------------------------

    IEntityWorldSystem* EntityWorld::GetWorldSystem( uint32_t worldSystemID ) const
    {
        for ( IEntityWorldSystem* pWorldSystem : m_worldSystems )
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
        // This will fill the world activation/registration lists used below
        // This will also handle all hot-reload unload/load requests

        for ( int32_t i = (int32_t) m_maps.size() - 1; i >= 0; i-- )
        {
            if ( m_maps[i].UpdateState( m_loadingContext, m_activationContext ) )
            {
                if ( m_maps[i].IsLoaded() )
                {
                    m_maps[i].Activate( m_activationContext );
                }
                else if ( m_maps[i].IsUnloaded() )
                {
                    m_maps.erase_unsorted( m_maps.begin() + i );
                }
            }
        }

        //-------------------------------------------------------------------------

        ProcessEntityRegistrationRequests();
        ProcessComponentRegistrationRequests();
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

                for ( auto pAttachedEntity : pEntity->m_attachedEntities )
                {
                    RecursiveEntityUpdate( pAttachedEntity );
                }
            }

            virtual void ExecuteRange( TaskSetPartition range, uint32_t threadnum ) override final
            {
                for ( uint64_t i = range.start; i < range.end; ++i )
                {
                    auto pEntity = m_updateList[i];

                    // Ignore any entities with spatial parents these will be removed from the update list
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

    void EntityWorld::ProcessEntityRegistrationRequests()
    {
        {
            EE_PROFILE_SCOPE_ENTITY( "Entity Unregistration" );

            Entity* pEntityToUnregister = nullptr;
            while ( m_activationContext.m_unregisterForEntityUpdate.try_dequeue( pEntityToUnregister ) )
            {
                EE_ASSERT( pEntityToUnregister != nullptr && pEntityToUnregister->m_updateRegistrationStatus == Entity::RegistrationStatus::QueuedForUnregister );
                m_entityUpdateList.erase_first_unsorted( pEntityToUnregister );
                pEntityToUnregister->m_updateRegistrationStatus = Entity::RegistrationStatus::Unregistered;
            }
        }

        //-------------------------------------------------------------------------

        {
            EE_PROFILE_SCOPE_ENTITY( "Entity Registration" );

            Entity* pEntityToRegister = nullptr;
            while ( m_activationContext.m_registerForEntityUpdate.try_dequeue( pEntityToRegister ) )
            {
                EE_ASSERT( pEntityToRegister != nullptr && pEntityToRegister->IsActivated() && pEntityToRegister->m_updateRegistrationStatus == Entity::RegistrationStatus::QueuedForRegister );
                EE_ASSERT( !pEntityToRegister->HasSpatialParent() ); // Attached entities are not allowed to be directly updated
                m_entityUpdateList.push_back( pEntityToRegister );
                pEntityToRegister->m_updateRegistrationStatus = Entity::RegistrationStatus::Registered;
            }
        }
    }

    void EntityWorld::ProcessComponentRegistrationRequests()
    {
        // Create a task that splits per-system registration across multiple threads
        struct ComponentRegistrationTask : public ITaskSet
        {
            ComponentRegistrationTask( TVector<IEntityWorldSystem*> const& worldSystems, TVector< TPair<Entity*, EntityComponent*> > const& componentsToRegister, TVector< TPair<Entity*, EntityComponent*> > const& componentsToUnregister )
                : m_worldSystems( worldSystems )
                , m_componentsToRegister( componentsToRegister )
                , m_componentsToUnregister( componentsToUnregister )
            {
                m_SetSize = (uint32_t) worldSystems.size();
            }

            virtual void ExecuteRange( TaskSetPartition range, uint32_t threadnum ) override final
            {
                EE_PROFILE_SCOPE_ENTITY( "Component World Registration Task" );

                for ( uint64_t i = range.start; i < range.end; ++i )
                {
                    auto pSystem = m_worldSystems[i];

                    // Unregister components
                    //-------------------------------------------------------------------------

                    size_t const numComponentsToUnregister = m_componentsToUnregister.size();
                    for ( auto c = 0u; c < numComponentsToUnregister; c++ )
                    {
                        auto pEntity = m_componentsToUnregister[c].first;
                        auto pComponent = m_componentsToUnregister[c].second;

                        EE_ASSERT( pEntity != nullptr );
                        EE_ASSERT( pComponent != nullptr && pComponent->IsInitialized() && pComponent->m_isRegisteredWithWorld );
                        pSystem->UnregisterComponent( pEntity, pComponent );
                    }

                    // Register components
                    //-------------------------------------------------------------------------

                    size_t const numComponentsToRegister = m_componentsToRegister.size();
                    for ( auto c = 0u; c < numComponentsToRegister; c++ )
                    {
                        auto pEntity = m_componentsToRegister[c].first;
                        auto pComponent = m_componentsToRegister[c].second;

                        EE_ASSERT( pEntity != nullptr && pEntity->IsActivated() && !pComponent->m_isRegisteredWithWorld );
                        EE_ASSERT( pComponent != nullptr && pComponent->IsInitialized() );
                        pSystem->RegisterComponent( pEntity, pComponent );
                    }
                }
            }

        private:

            TVector<IEntityWorldSystem*> const&                         m_worldSystems;
            TVector< TPair<Entity*, EntityComponent*> > const&          m_componentsToRegister;
            TVector< TPair<Entity*, EntityComponent*> > const&          m_componentsToUnregister;
        };

        //-------------------------------------------------------------------------

        {
            EE_PROFILE_SCOPE_ENTITY( "Component World Registration" );

            // Get Components to register/unregister
            //-------------------------------------------------------------------------

            TVector< TPair<Entity*, EntityComponent*>> componentsToUnregister;
            size_t const numComponentsToUnregister = m_activationContext.m_componentsToUnregister.size_approx();
            componentsToUnregister.resize( numComponentsToUnregister );

            size_t numDequeued = m_activationContext.m_componentsToUnregister.try_dequeue_bulk( componentsToUnregister.data(), numComponentsToUnregister );
            EE_ASSERT( numComponentsToUnregister == numDequeued );

            //-------------------------------------------------------------------------

            TVector< TPair<Entity*, EntityComponent*>> componentsToRegister;
            size_t const numComponentsToRegister = m_activationContext.m_componentsToRegister.size_approx();
            componentsToRegister.resize( numComponentsToRegister );

            numDequeued = m_activationContext.m_componentsToRegister.try_dequeue_bulk( componentsToRegister.data(), numComponentsToRegister );
            EE_ASSERT( numComponentsToRegister == numDequeued );

            // Run registration task
            //-------------------------------------------------------------------------

            if ( ( numComponentsToUnregister + numComponentsToRegister ) > 0 )
            {
                ComponentRegistrationTask componentRegistrationTask( m_worldSystems, componentsToRegister, componentsToUnregister );
                m_loadingContext.m_pTaskSystem->ScheduleTask( &componentRegistrationTask );
                m_loadingContext.m_pTaskSystem->WaitForTask( &componentRegistrationTask );
            }

            // Finalize component registration
            //-------------------------------------------------------------------------
            // Update component registration flags
            // Add to type tracking table

            for ( auto const& unregisteredComponent : componentsToUnregister )
            {
                auto pComponent = unregisteredComponent.second;
                pComponent->m_isRegisteredWithWorld = false;

                #if EE_DEVELOPMENT_TOOLS
                auto const castableTypeIDs = m_loadingContext.m_pTypeRegistry->GetAllCastableTypes( pComponent );
                for ( auto castableTypeID : castableTypeIDs )
                {
                    m_componentTypeLookup[castableTypeID].erase_first_unsorted( pComponent );
                }
                #endif
            }

            for ( auto const& registeredComponent : componentsToRegister )
            {
                auto pComponent = registeredComponent.second;
                pComponent->m_isRegisteredWithWorld = true;

                #if EE_DEVELOPMENT_TOOLS
                auto const castableTypeIDs = m_loadingContext.m_pTypeRegistry->GetAllCastableTypes( pComponent );
                for ( auto castableTypeID : castableTypeIDs )
                {
                    m_componentTypeLookup[castableTypeID].emplace_back( pComponent );
                }
                #endif
            }
        }
    }

    //-------------------------------------------------------------------------
    // Maps
    //-------------------------------------------------------------------------

    bool EntityWorld::IsBusyLoading() const
    {
        for ( auto const& map : m_maps )
        {
            if( map.IsLoading() ) 
            {
                return true;
            }
        }

        return false;
    }

    bool EntityWorld::HasMap( ResourceID const& mapResourceID ) const
    {
        // Make sure the map isn't already loaded or loading, since duplicate loads are not allowed
        for ( auto const& map : m_maps )
        {
            if ( map.GetMapResourceID() == mapResourceID )
            {
                return true;
            }
        }

        return false;
    }

    bool EntityWorld::IsMapActive( ResourceID const& mapResourceID ) const
    {
        // Make sure the map isn't already loaded or loading, since duplicate loads are not allowed
        for ( auto const& map : m_maps )
        {
            if ( map.GetMapResourceID() == mapResourceID )
            {
                return map.IsActivated();
            }
        }

        // Dont call this function with an unknown map
        EE_UNREACHABLE_CODE();
        return false;
    }

    bool EntityWorld::IsMapActive( EntityMapID const& mapID ) const
    {
        // Make sure the map isn't already loaded or loading, since duplicate loads are not allowed
        for ( auto const& map : m_maps )
        {
            if ( map.GetID() == mapID )
            {
                return map.IsActivated();
            }
        }

        // Dont call this function with an unknown map
        EE_UNREACHABLE_CODE();
        return false;
    }

    EntityModel::EntityMap* EntityWorld::CreateTransientMap()
    {
        EntityModel::EntityMap& newMap = m_maps.emplace_back( EntityModel::EntityMap() );
        newMap.Load( m_loadingContext );
        newMap.Activate( m_activationContext );
        return &newMap;
    }

    EntityModel::EntityMap const* EntityWorld::GetMap( ResourceID const& mapResourceID ) const
    {
        EE_ASSERT( mapResourceID.IsValid() && mapResourceID.GetResourceTypeID() == EntityModel::EntityMapDescriptor::GetStaticResourceTypeID() );
        auto const foundMapIter = VectorFind( m_maps, mapResourceID, [] ( EntityModel::EntityMap const& map, ResourceID const& mapResourceID ) { return map.GetMapResourceID() == mapResourceID; } );
        EE_ASSERT( foundMapIter != m_maps.end() );
        return foundMapIter;
    }

    EntityModel::EntityMap const* EntityWorld::GetMap( EntityMapID const& mapID ) const
    {
        EE_ASSERT( mapID.IsValid() );
        auto const foundMapIter = VectorFind( m_maps, mapID, [] ( EntityModel::EntityMap const& map, EntityMapID const& mapID ) { return map.GetID() == mapID; } );
        EE_ASSERT( foundMapIter != m_maps.end() );
        return foundMapIter;
    }

    void EntityWorld::LoadMap( ResourceID const& mapResourceID )
    {
        EE_ASSERT( mapResourceID.IsValid() && mapResourceID.GetResourceTypeID() == EntityModel::EntityMapDescriptor::GetStaticResourceTypeID() );

        EE_ASSERT( !HasMap( mapResourceID ) );
        auto& map = m_maps.emplace_back( EntityModel::EntityMap( mapResourceID ) );
        map.Load( m_loadingContext );
    }

    void EntityWorld::UnloadMap( ResourceID const& mapResourceID )
    {
        EE_ASSERT( mapResourceID.IsValid() && mapResourceID.GetResourceTypeID() == EntityModel::EntityMapDescriptor::GetStaticResourceTypeID() );

        auto const foundMapIter = VectorFind( m_maps, mapResourceID, [] ( EntityModel::EntityMap const& map, ResourceID const& mapResourceID ) { return map.GetMapResourceID() == mapResourceID; } );
        EE_ASSERT( foundMapIter != m_maps.end() );
        foundMapIter->Unload( m_loadingContext );
    }

    //-------------------------------------------------------------------------
    // Editing / Hot Reload
    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void EntityWorld::PrepareComponentForEditing( EntityMapID const& mapID, EntityID const& entityID, ComponentID const& componentID )
    {
        auto pMap = GetMap( mapID );
        EE_ASSERT( pMap != nullptr );
        pMap->ComponentEditingDeactivate( m_activationContext, entityID, componentID );

        // Process all deactivation requests
        ProcessEntityRegistrationRequests();
        ProcessComponentRegistrationRequests();

        pMap->ComponentEditingUnload( m_loadingContext, entityID, componentID );
    }

    //-------------------------------------------------------------------------

    void EntityWorld::BeginHotReload( TVector<Resource::ResourceRequesterID> const& usersToReload )
    {
        EE_ASSERT( !usersToReload.empty() );

        // Fill the hot-reload lists per map and deactivate any active entities
        for ( auto& map : m_maps )
        {
            map.HotReloadDeactivateEntities( m_activationContext, usersToReload );
        }

        // Process all deactivation requests
        ProcessEntityRegistrationRequests();
        ProcessComponentRegistrationRequests();

        // Unload all entities that require hot reload
        for ( auto& map : m_maps )
        {
            map.HotReloadUnloadEntities( m_loadingContext );
        }
    }

    void EntityWorld::EndHotReload()
    {
        // Load all entities that require hot reload
        for ( auto& map : m_maps )
        {
            map.HotReloadLoadEntities( m_loadingContext );
        }
    }
    #endif
}