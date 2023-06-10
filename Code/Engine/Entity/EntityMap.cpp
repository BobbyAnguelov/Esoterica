#include "EntityMap.h"
#include "EntityLog.h"
#include "EntityContexts.h"
#include "EntitySerialization.h"
#include "EntityWorldSystem.h"
#include "Entity.h"
#include "System/Resource/ResourceSystem.h"
#include "System/TypeSystem/TypeRegistry.h"
#include "System/Profiling.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    EntityMap::EntityMap()
        : m_entityUpdateEventBindingID( Entity::OnEntityInternalStateUpdated().Bind( [this] ( Entity* pEntity ) { OnEntityStateUpdated( pEntity ); } ) )
        , m_isTransientMap( true )
    {}

    EntityMap::EntityMap( ResourceID mapResourceID )
        : m_pMapDesc( mapResourceID )
        , m_entityUpdateEventBindingID( Entity::OnEntityInternalStateUpdated().Bind( [this] ( Entity* pEntity ) { OnEntityStateUpdated( pEntity ); } ) )
    {}

    EntityMap::EntityMap( EntityMap const& map )
        : m_entityUpdateEventBindingID( Entity::OnEntityInternalStateUpdated().Bind( [this] ( Entity* pEntity ) { OnEntityStateUpdated( pEntity ); } ) )
    {
        operator=( map );
    }

    EntityMap::EntityMap( EntityMap&& map )
        : m_entityUpdateEventBindingID( Entity::OnEntityInternalStateUpdated().Bind( [this] ( Entity* pEntity ) { OnEntityStateUpdated( pEntity ); } ) )
    {
        operator=( eastl::move( map ) );
    }

    EntityMap::~EntityMap()
    {
        EE_ASSERT( IsUnloaded() );
        EE_ASSERT( m_entities.empty() && m_entityIDLookupMap.empty() );
        EE_ASSERT( m_entitiesToLoad.empty() && m_entitiesToRemove.empty() );

        #if EE_DEVELOPMENT_TOOLS
        EE_ASSERT( m_entitiesToHotReload.empty() );
        EE_ASSERT( m_editedEntities.empty() );
        #endif

        Entity::OnEntityInternalStateUpdated().Unbind( m_entityUpdateEventBindingID );
    }

    //-------------------------------------------------------------------------

    EntityMap& EntityMap::operator=( EntityMap const& map )
    {
        // Only allow copy constructor for unloaded maps
        EE_ASSERT( m_status == Status::Unloaded && map.m_status == Status::Unloaded );

        #if EE_DEVELOPMENT_TOOLS
        EE_ASSERT( map.m_entitiesToHotReload.empty() );
        #endif

        m_pMapDesc = map.m_pMapDesc;
        const_cast<bool&>( m_isTransientMap ) = map.m_isTransientMap;
        return *this;
    }

    EntityMap& EntityMap::operator=( EntityMap&& map )
    {
        #if EE_DEVELOPMENT_TOOLS
        EE_ASSERT( map.m_entitiesToHotReload.empty() );
        #endif

        m_ID = map.m_ID;
        m_entities.swap( map.m_entities );
        m_entityIDLookupMap.swap( map.m_entityIDLookupMap );
        m_pMapDesc = eastl::move( map.m_pMapDesc );
        m_entitiesCurrentlyLoading = eastl::move( map.m_entitiesCurrentlyLoading );
        m_status = map.m_status;
        const_cast<bool&>( m_isTransientMap ) = map.m_isTransientMap;

        // Clear source map
        map.m_ID.Clear();
        map.m_status = Status::Unloaded;
        return *this;
    }

    //-------------------------------------------------------------------------
    // Entity Management
    //-------------------------------------------------------------------------

    void EntityMap::AddEntities( TVector<Entity*> const& entities, Transform const& offsetTransform )
    {
        Threading::RecursiveScopeLock lock( m_mutex );

        m_entityIDLookupMap.reserve( m_entityIDLookupMap.size() + entities.size() );

        #if EE_DEVELOPMENT_TOOLS
        m_entityNameLookupMap.reserve( m_entityNameLookupMap.size() + entities.size() );
        #endif

        //-------------------------------------------------------------------------

        bool const applyOffset = !offsetTransform.IsIdentity();
        for ( auto& pEntity : entities )
        {
            // Shift entity by the specified offset
            if ( applyOffset && pEntity->IsSpatialEntity() )
            {
                pEntity->SetWorldTransform( pEntity->GetWorldTransform() * offsetTransform );
            }

            AddEntity( pEntity );
        }
    }

    void EntityMap::AddEntity( Entity* pEntity )
    {
        // Ensure that the entity to add, is not already part of a collection and that it is not initialized
        EE_ASSERT( pEntity != nullptr && !pEntity->IsAddedToMap() && !pEntity->HasRequestedComponentLoad() );
        EE_ASSERT( !VectorContains( m_entitiesToLoad, pEntity ) );

        // Entity validation
        //-------------------------------------------------------------------------
        // Ensure spatial parenting and unique name

        #if EE_DEVELOPMENT_TOOLS
        if ( pEntity->HasSpatialParent() )
        {
            EE_ASSERT( ContainsEntity( pEntity->GetSpatialParent()->GetID() ) );
        }

        pEntity->m_name = GenerateUniqueEntityNameID( pEntity->m_name );
        #endif

        // Add entity
        //-------------------------------------------------------------------------

        Threading::RecursiveScopeLock lock( m_mutex );

        pEntity->m_mapID = m_ID;
        m_entities.emplace_back( pEntity );
        m_entitiesToLoad.emplace_back( pEntity );

        // Add to lookup maps
        //-------------------------------------------------------------------------

        m_entityIDLookupMap.insert( TPair<EntityID, Entity*>( pEntity->m_ID, pEntity ) );
        #if EE_DEVELOPMENT_TOOLS
        m_entityNameLookupMap.insert( TPair<StringID, Entity*>( pEntity->m_name, pEntity ) );
        #endif
    }

    void EntityMap::AddEntityCollection( TaskSystem* pTaskSystem, TypeSystem::TypeRegistry const& typeRegistry, SerializedEntityCollection const& entityCollectionDesc, Transform const& offsetTransform )
    {
        TVector<Entity*> const createdEntities = Serializer::CreateEntities( pTaskSystem, typeRegistry, entityCollectionDesc );
        AddEntities( createdEntities, offsetTransform );
    }

    Entity* EntityMap::RemoveEntityInternal( EntityID entityID, bool destroyEntityOnceRemoved )
    {
        Threading::RecursiveScopeLock lock( m_mutex );

        // Handle spatial hierarchy
        //-------------------------------------------------------------------------

        Entity* pEntityToRemove = FindEntity( entityID );
        EE_ASSERT( pEntityToRemove != nullptr );

        if ( !pEntityToRemove->m_attachedEntities.empty() )
        {
            // If we have a parent, re-parent all children to it
            if ( pEntityToRemove->m_pParentSpatialEntity != nullptr )
            {
                for ( auto pAttachedEntity : pEntityToRemove->m_attachedEntities )
                {
                    pAttachedEntity->SetSpatialParent( pEntityToRemove->m_pParentSpatialEntity );
                }
            }
            else // If we have no parent, remove spatial parent
            {
                for ( auto pAttachedEntity : pEntityToRemove->m_attachedEntities )
                {
                    pAttachedEntity->ClearSpatialParent();
                }
            }
        }

        // Remove from map
        //-------------------------------------------------------------------------

        m_entities.erase_first_unsorted( pEntityToRemove );

        // Remove from internal lookup maps
        //-------------------------------------------------------------------------

        auto IDLookupIter = m_entityIDLookupMap.find( pEntityToRemove->m_ID );
        EE_ASSERT( IDLookupIter != m_entityIDLookupMap.end() );
        m_entityIDLookupMap.erase( IDLookupIter );

        #if EE_DEVELOPMENT_TOOLS
        auto nameLookupIter = m_entityNameLookupMap.find( pEntityToRemove->m_name );
        EE_ASSERT( nameLookupIter != m_entityNameLookupMap.end() );
        m_entityNameLookupMap.erase( nameLookupIter );
        #endif

        // Schedule unload
        //-------------------------------------------------------------------------

        // Check if the entity is in the add queue, if so just cancel the request
        int32_t const entityIdx = VectorFindIndex( m_entitiesToLoad, entityID, [] ( Entity* pEntity, EntityID entityID ) { return pEntity->GetID() == entityID; } );
        if ( entityIdx != InvalidIndex )
        {
            pEntityToRemove = m_entitiesToLoad[entityIdx];
            m_entitiesToLoad.erase_unsorted( m_entitiesToLoad.begin() + entityIdx );

            if ( destroyEntityOnceRemoved )
            {
                EE::Delete( pEntityToRemove );
            }
        }
        else // Queue removal
        {
            m_entitiesToRemove.emplace_back( RemovalRequest( pEntityToRemove, destroyEntityOnceRemoved ) );
        }

        //-------------------------------------------------------------------------

        // Do not return anything if we are requesting destruction
        if ( destroyEntityOnceRemoved )
        {
            pEntityToRemove = nullptr;
        }

        return pEntityToRemove;
    }

    Entity* EntityMap::RemoveEntity( EntityID entityID )
    {
        Entity* pEntityToRemove = RemoveEntityInternal( entityID, false );
        EE_ASSERT( pEntityToRemove != nullptr );
        return pEntityToRemove;
    }

    void EntityMap::DestroyEntity( EntityID entityID )
    {
        RemoveEntityInternal( entityID, true );
    }

    void EntityMap::OnEntityStateUpdated( Entity* pEntity )
    {
        if ( pEntity->GetMapID() == m_ID )
        {
            EE_ASSERT( FindEntity( pEntity->GetID() ) );
            Threading::RecursiveScopeLock lock( m_mutex );
            if ( !VectorContains( m_entitiesCurrentlyLoading, pEntity ) )
            {
                m_entitiesCurrentlyLoading.emplace_back( pEntity );
            }
        }
    }

    //-------------------------------------------------------------------------
    // Loading
    //-------------------------------------------------------------------------

    void EntityMap::Load( LoadingContext const& loadingContext, InitializationContext& initializationContext )
    {
        EE_ASSERT( Threading::IsMainThread() && loadingContext.IsValid() );
        EE_ASSERT( m_status == Status::Unloaded );

        Threading::RecursiveScopeLock lock( m_mutex );

        if ( m_isTransientMap )
        {
            m_status = Status::Loaded;
        }
        else // Request loading of map resource
        {
            loadingContext.m_pResourceSystem->LoadResource( m_pMapDesc );
            m_status = Status::Loading;
        }
    }

    void EntityMap::ProcessMapLoading( LoadingContext const& loadingContext )
    {
        EE_PROFILE_SCOPE_ENTITY( "Map Loading" );
        EE_ASSERT( m_status == Status::Loading );
        EE_ASSERT( !m_isTransientMap );

        //-------------------------------------------------------------------------

        // Wait for the map descriptor to load
        if ( m_pMapDesc.IsLoading() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        // Check for load failure
        if ( m_pMapDesc.HasLoadingFailed() )
        {
            m_status = Status::LoadFailed;
            loadingContext.m_pResourceSystem->UnloadResource( m_pMapDesc );
            return;
        }

        //-------------------------------------------------------------------------

        // Instantiate the map
        if ( m_pMapDesc->IsValid() )
        {
            // Create all required entities
            TVector<Entity*> const createdEntities = Serializer::CreateEntities( loadingContext.m_pTaskSystem, *loadingContext.m_pTypeRegistry, *m_pMapDesc.GetPtr() );

            // Reserve memory for new entities in internal structures
            m_entities.reserve( m_entities.size() + createdEntities.size() );
            m_entitiesToLoad.reserve( m_entitiesToLoad.size() + createdEntities.size() );
            m_entityIDLookupMap.reserve( m_entityIDLookupMap.size() + createdEntities.size() );
            m_entitiesCurrentlyLoading.reserve( m_entitiesCurrentlyLoading.size() + createdEntities.size() );

            #if EE_DEVELOPMENT_TOOLS
            m_entityNameLookupMap.reserve( m_entityNameLookupMap.size() + createdEntities.size() );
            #endif

            // Add entities
            for ( auto pEntity : createdEntities )
            {
                AddEntity( pEntity );
            }

            m_status = Status::Loaded;
        }
        else // Invalid map data is treated as a failed load
        {
            m_status = Status::LoadFailed;
        }

        // Release map resource ptr once loading has completed
        loadingContext.m_pResourceSystem->UnloadResource( m_pMapDesc );
    }

    void EntityMap::Unload( LoadingContext const& loadingContext, InitializationContext& initializationContext )
    {
        EE_ASSERT( m_status != Status::Unloaded );
        Threading::RecursiveScopeLock lock( m_mutex );
        m_status = Status::Unloading;
    }

    void EntityMap::ProcessMapUnloading( LoadingContext const& loadingContext, InitializationContext& initializationContext )
    {
        EE_PROFILE_SCOPE_ENTITY( "Map Unload" );
        EE_ASSERT( m_status == Status::Unloading );

        // Cancel any load requests
        //-------------------------------------------------------------------------

        m_entitiesCurrentlyLoading.clear();
        m_entitiesToLoad.clear();

        // Shutdown all entities
        //-------------------------------------------------------------------------

        // Shutdown all entities that have been requested for removal
        ProcessEntityShutdownRequests( initializationContext );

        // Shutdown map entities
        {
            struct EntityShutdownTask : public ITaskSet
            {
                EntityShutdownTask( InitializationContext& initializationContext, TVector<Entity*>& entities )
                    : m_initializationContext( initializationContext )
                    , m_entitiesToShutdown( entities )
                {
                    m_SetSize = (uint32_t) m_entitiesToShutdown.size();
                }

                virtual void ExecuteRange( TaskSetPartition range, uint32_t threadnum ) override final
                {
                    EE_PROFILE_SCOPE_ENTITY( "Shutdown Entities" );

                    for ( uint64_t i = range.start; i < range.end; ++i )
                    {
                        auto pEntity = m_entitiesToShutdown[i];
                        if ( pEntity->IsInitialized() )
                        {
                            // Only shutdown non-spatial and root spatial entities. Attached entities will be shutdown by their parents
                            if ( !pEntity->IsSpatialEntity() || !pEntity->HasSpatialParent() )
                            {
                                pEntity->Shutdown( m_initializationContext );
                            }
                        }
                    }
                }

            private:

                InitializationContext&          m_initializationContext;
                TVector<Entity*>&               m_entitiesToShutdown;
            };

            //-------------------------------------------------------------------------

            EntityShutdownTask shutdownTask( initializationContext, m_entities );
            initializationContext.m_pTaskSystem->ScheduleTask( &shutdownTask );
            initializationContext.m_pTaskSystem->WaitForTask( &shutdownTask );
        }

        // Process all unregistration requests
        ProcessEntityRegistrationRequests( initializationContext );

        // Unload and destroy entities
        //-------------------------------------------------------------------------

        // Unload all entities that have been requested for removal
        ProcessEntityRemovalRequests( loadingContext );

        // Unload and destroy map entities
        for ( auto& pEntity : m_entities )
        {
            EE_ASSERT( !pEntity->IsInitialized() );
            if ( pEntity->IsLoaded() )
            {
                pEntity->UnloadComponents( loadingContext );
            }
            EE::Delete( pEntity );
        }

        m_entities.clear();
        m_entityIDLookupMap.clear();
         
        #if EE_DEVELOPMENT_TOOLS
        m_entityNameLookupMap.clear();
        #endif

        // Unload the map resource
        //-------------------------------------------------------------------------

        if ( !m_isTransientMap && m_pMapDesc.WasRequested() )
        {
            loadingContext.m_pResourceSystem->UnloadResource( m_pMapDesc );
        }

        m_status = Status::Unloaded;
    }

    //-------------------------------------------------------------------------

    void EntityMap::ProcessEntityShutdownRequests( InitializationContext& initializationContext )
    {
        EE_PROFILE_SCOPE_ENTITY( "Entity Shutdown" );

        // No point parallelizing this, since this should never have that many entries in it
        for ( int32_t i = (int32_t) m_entitiesToRemove.size() - 1; i >= 0; i-- )
        {
            auto pEntityToShutdown = m_entitiesToRemove[i].m_pEntity;
            if ( pEntityToShutdown->IsInitialized() )
            {
                pEntityToShutdown->Shutdown( initializationContext );
            }
        }
    }

    void EntityMap::ProcessEntityRemovalRequests( LoadingContext const& loadingContext )
    {
        EE_PROFILE_SCOPE_ENTITY( "Entity Removal" );

        for ( int32_t i = (int32_t) m_entitiesToRemove.size() - 1; i >= 0; i-- )
        {
            auto& removalRequest = m_entitiesToRemove[i];
            auto pEntityToRemove = removalRequest.m_pEntity;
            EE_ASSERT( !pEntityToRemove->IsInitialized() );

            // Remove from currently loading list
            m_entitiesCurrentlyLoading.erase_first_unsorted( pEntityToRemove );

            // Unload entity
            pEntityToRemove->UnloadComponents( loadingContext );
            pEntityToRemove->m_mapID.Clear();

            // Destroy the entity if this is a destruction request
            if ( removalRequest.m_shouldDestroy )
            {
                EE::Delete( pEntityToRemove );
            }

            // Remove the request from the list
            m_entitiesToRemove.erase_unsorted( m_entitiesToRemove.begin() + i );
        }
    }

    void EntityMap::ProcessEntityLoadingAndInitialization( LoadingContext const& loadingContext, InitializationContext& initializationContext )
    {
        EE_PROFILE_SCOPE_ENTITY( "Entity Loading/Initialization" );

        // Request load for unloaded entities
        // No point parallelizing this since the resource system locks a mutex for each load/unload call!
        for ( auto pEntityToAdd : m_entitiesToLoad )
        {
            // Ensure that the entity to add, is not already part of a collection and that it's shutdown
            EE_ASSERT( pEntityToAdd != nullptr && pEntityToAdd->m_mapID == m_ID && !pEntityToAdd->IsInitialized() );

            // Request component load
            pEntityToAdd->LoadComponents( loadingContext );
            EE_ASSERT( !VectorContains( m_entitiesCurrentlyLoading, pEntityToAdd ) );
            m_entitiesCurrentlyLoading.emplace_back( pEntityToAdd );
        }

        m_entitiesToLoad.clear();

        //-------------------------------------------------------------------------

        struct EntityLoadingTask : public ITaskSet
        {
            EntityLoadingTask( LoadingContext const& loadingContext, InitializationContext& initializationContext, TVector<Entity*>& entitiesToLoad )
                : m_loadingContext( loadingContext )
                , m_initializationContext( initializationContext )
                , m_entitiesToLoad( entitiesToLoad )
            {
                m_SetSize = (uint32_t) m_entitiesToLoad.size();
            }

            virtual void ExecuteRange( TaskSetPartition range, uint32_t threadnum ) override final
            {
                EE_PROFILE_SCOPE_ENTITY( "Load and Initialize Entities" );
                for ( uint32_t i = range.start; i < range.end; ++i )
                {
                    auto pEntity = m_entitiesToLoad[i];
                    if ( pEntity->UpdateEntityState( m_loadingContext, m_initializationContext ) )
                    {
                        #if EE_DEVELOPMENT_TOOLS
                        for ( auto pComponent : pEntity->GetComponents() )
                        {
                            EE_ASSERT( pComponent->IsInitialized() || pComponent->HasLoadingFailed() );
                        }
                        #endif

                        // Initialize any entities that loaded successfully
                        if ( pEntity->IsLoaded() )
                        {
                            // Prevent us from initializing entities whose parents are not yet initialized, this ensures that our attachment chains have a consistent initialized state
                            if ( pEntity->HasSpatialParent() )
                            {
                                // We need to recheck the initialization state of this entity since while waiting for the lock, it could have been initialized by the parent
                                Threading::RecursiveScopeLock parentLock( pEntity->GetSpatialParent()->m_internalStateMutex );
                                if ( !pEntity->IsInitialized() && pEntity->GetSpatialParent()->IsInitialized() )
                                {
                                    pEntity->Initialize( m_initializationContext );
                                }
                            }
                            else
                            {
                                pEntity->Initialize( m_initializationContext );
                            }
                        }
                    }
                    else // Entity is still loading
                    {
                        bool result = m_stillLoadingEntities.enqueue( pEntity );
                        EE_ASSERT( result );
                    }
                }
            }

        public:

            Threading::LockFreeQueue<Entity*>       m_stillLoadingEntities;

        private:

            LoadingContext const&                   m_loadingContext;
            InitializationContext&                  m_initializationContext;
            TVector<Entity*>&                       m_entitiesToLoad;
        };

        //-------------------------------------------------------------------------

        if ( !m_entitiesCurrentlyLoading.empty() )
        {
            EntityLoadingTask loadingTask( loadingContext, initializationContext, m_entitiesCurrentlyLoading );
            loadingContext.m_pTaskSystem->ScheduleTask( &loadingTask );
            loadingContext.m_pTaskSystem->WaitForTask( &loadingTask );

            //-------------------------------------------------------------------------

            // Track the number of entities that still need loading
            size_t const numEntitiesStillLoading = loadingTask.m_stillLoadingEntities.size_approx();
            m_entitiesCurrentlyLoading.resize( numEntitiesStillLoading );
            size_t numDequeued = loadingTask.m_stillLoadingEntities.try_dequeue_bulk( m_entitiesCurrentlyLoading.data(), numEntitiesStillLoading );
            EE_ASSERT( numEntitiesStillLoading == numDequeued );
        }
    }

    //-------------------------------------------------------------------------

    void EntityMap::ProcessEntityRegistrationRequests( InitializationContext& initializationContext )
    {
        EE_ASSERT( Threading::IsMainThread() );

        //-------------------------------------------------------------------------
        // Entity Registrations
        //-------------------------------------------------------------------------

        {
            EE_PROFILE_SCOPE_ENTITY( "Entity Update Registration" );

            Entity* pEntity = nullptr;
            while ( initializationContext.m_unregisterForEntityUpdate.try_dequeue( pEntity ) )
            {
                EE_ASSERT( pEntity != nullptr && pEntity->m_updateRegistrationStatus == Entity::UpdateRegistrationStatus::QueuedForUnregister );
                initializationContext.m_entityUpdateList.erase_first_unsorted( pEntity );
                pEntity->m_updateRegistrationStatus = Entity::UpdateRegistrationStatus::Unregistered;
            }

            //-------------------------------------------------------------------------

            while ( initializationContext.m_registerForEntityUpdate.try_dequeue( pEntity ) )
            {
                EE_ASSERT( pEntity != nullptr );
                EE_ASSERT( pEntity->IsInitialized() );
                EE_ASSERT( pEntity->m_updateRegistrationStatus == Entity::UpdateRegistrationStatus::QueuedForRegister );
                EE_ASSERT( !pEntity->HasSpatialParent() ); // Attached entities are not allowed to be directly updated
                initializationContext.m_entityUpdateList.push_back( pEntity );
                pEntity->m_updateRegistrationStatus = Entity::UpdateRegistrationStatus::Registered;
            }
        }

        //-------------------------------------------------------------------------
        // Component Registrations
        //-------------------------------------------------------------------------

        // Create a task that splits per-system registration across multiple threads
        struct ComponentRegistrationTask : public ITaskSet
        {
            ComponentRegistrationTask( TVector<IEntityWorldSystem*> const& worldSystems, TVector<EntityModel::EntityComponentPair> const& componentsToRegister, TVector<EntityModel::EntityComponentPair> const& componentsToUnregister )
                : m_worldSystems( worldSystems )
                , m_componentsToRegister( componentsToRegister )
                , m_componentsToUnregister( componentsToUnregister )
            {
                m_SetSize = (uint32_t) worldSystems.size();
            }

            virtual void ExecuteRange( TaskSetPartition range, uint32_t threadnum ) override final
            {
                EE_PROFILE_SCOPE_ENTITY( "Component World Registration Task" );

                // Register/Unregister component with World Systems
                //-------------------------------------------------------------------------

                for ( uint64_t i = range.start; i < range.end; ++i )
                {
                    auto pSystem = m_worldSystems[i];

                    //-------------------------------------------------------------------------

                    size_t const numComponentsToUnregister = m_componentsToUnregister.size();
                    for ( auto c = 0u; c < numComponentsToUnregister; c++ )
                    {
                        auto pEntity = m_componentsToUnregister[c].m_pEntity;
                        auto pComponent = m_componentsToUnregister[c].m_pComponent;

                        EE_ASSERT( pEntity != nullptr );
                        EE_ASSERT( pComponent != nullptr && pComponent->IsInitialized() && pComponent->m_isRegisteredWithWorld );
                        pSystem->UnregisterComponent( pEntity, pComponent );
                    }

                    //-------------------------------------------------------------------------

                    size_t const numComponentsToRegister = m_componentsToRegister.size();
                    for ( auto c = 0u; c < numComponentsToRegister; c++ )
                    {
                        auto pEntity = m_componentsToRegister[c].m_pEntity;
                        auto pComponent = m_componentsToRegister[c].m_pComponent;

                        EE_ASSERT( pEntity != nullptr && pEntity->IsInitialized() && !pComponent->m_isRegisteredWithWorld );
                        EE_ASSERT( pComponent != nullptr && pComponent->IsInitialized() );
                        pSystem->RegisterComponent( pEntity, pComponent );
                    }
                }
            }

        private:

            TVector<IEntityWorldSystem*> const&                         m_worldSystems;
            TVector<EntityModel::EntityComponentPair> const&            m_componentsToRegister;
            TVector<EntityModel::EntityComponentPair> const&            m_componentsToUnregister;
        };

        //-------------------------------------------------------------------------

        {
            EE_PROFILE_SCOPE_ENTITY( "Component World Registration" );

            // Get Components to register/unregister
            //-------------------------------------------------------------------------

            TVector<EntityModel::EntityComponentPair> componentsToUnregister;
            size_t const numComponentsToUnregister = initializationContext.m_componentsToUnregister.size_approx();
            componentsToUnregister.resize( numComponentsToUnregister );

            size_t numDequeued = initializationContext.m_componentsToUnregister.try_dequeue_bulk( componentsToUnregister.data(), numComponentsToUnregister );
            EE_ASSERT( numComponentsToUnregister == numDequeued );

            //-------------------------------------------------------------------------

            TVector<EntityModel::EntityComponentPair> componentsToRegister;
            size_t const numComponentsToRegister = initializationContext.m_componentsToRegister.size_approx();
            componentsToRegister.resize( numComponentsToRegister );

            numDequeued = initializationContext.m_componentsToRegister.try_dequeue_bulk( componentsToRegister.data(), numComponentsToRegister );
            EE_ASSERT( numComponentsToRegister == numDequeued );

            // Run registration task
            //-------------------------------------------------------------------------

            if ( ( numComponentsToUnregister + numComponentsToRegister ) > 0 )
            {
                ComponentRegistrationTask componentRegistrationTask( initializationContext.m_worldSystems, componentsToRegister, componentsToUnregister );
                initializationContext.m_pTaskSystem->ScheduleTask( &componentRegistrationTask );
                initializationContext.m_pTaskSystem->WaitForTask( &componentRegistrationTask );
            }

            // Finalize component registration
            //-------------------------------------------------------------------------

            // Remove from type tracking table
            for ( auto const& pair : componentsToUnregister )
            {
                pair.m_pComponent->m_isRegisteredWithWorld = false;

                #if EE_DEVELOPMENT_TOOLS
                EntityComponentTypeMap& componentTypeMap = *initializationContext.m_pComponentTypeMap;
                auto const castableTypeIDs = initializationContext.m_pTypeRegistry->GetAllCastableTypes( pair.m_pComponent );
                for ( auto castableTypeID : castableTypeIDs )
                {
                    componentTypeMap[castableTypeID].erase_first_unsorted( pair.m_pComponent );
                }
                #endif
            }

            // Add to type tracking table
            for ( auto const& pair : componentsToRegister )
            {
                pair.m_pComponent->m_isRegisteredWithWorld = true;

                #if EE_DEVELOPMENT_TOOLS
                EntityComponentTypeMap& componentTypeMap = *initializationContext.m_pComponentTypeMap;
                auto const castableTypeIDs = initializationContext.m_pTypeRegistry->GetAllCastableTypes( pair.m_pComponent );
                for ( auto castableTypeID : castableTypeIDs )
                {
                    componentTypeMap[castableTypeID].emplace_back( pair.m_pComponent );
                }
                #endif
            }
        }
    }

    //-------------------------------------------------------------------------

    bool EntityMap::UpdateLoadingAndStateChanges( LoadingContext const& loadingContext, InitializationContext& initializationContext )
    {
        EE_PROFILE_SCOPE_ENTITY( "Map State Update" );
        EE_ASSERT( Threading::IsMainThread() && loadingContext.IsValid() && initializationContext.IsValid() );

        #if EE_DEVELOPMENT_TOOLS
        EE_ASSERT( m_entitiesToHotReload.empty() );
        EE_ASSERT( m_editedEntities.empty() ); // You are missing a EndComponentEdit call somewhere!
        #endif

        //-------------------------------------------------------------------------

        Threading::RecursiveScopeLock lock( m_mutex );

        //-------------------------------------------------------------------------

        switch ( m_status )
        {
            case Status::Unloading:
            {
                ProcessMapUnloading( loadingContext, initializationContext );
            }
            break;

            case Status::Loading:
            {
                ProcessMapLoading( loadingContext );
            }
            break;

            default:
            break;
        }

        // Process entity removal requests
        //-------------------------------------------------------------------------

        ProcessEntityShutdownRequests( initializationContext );
        ProcessEntityRegistrationRequests( initializationContext );
        ProcessEntityRemovalRequests( loadingContext );

        // Update entity load states
        //-------------------------------------------------------------------------

        ProcessEntityLoadingAndInitialization( loadingContext, initializationContext );
        ProcessEntityRegistrationRequests( initializationContext );

        // Return status
        //-------------------------------------------------------------------------

        if ( m_status == Status::Loading || !m_entitiesCurrentlyLoading.empty() )
        {
            return false;
        }

        return true;
    }

    //-------------------------------------------------------------------------
    // Tools
    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void EntityMap::RenameEntity( Entity* pEntity, StringID newNameID )
    {
        EE_ASSERT( pEntity != nullptr && pEntity->m_mapID == m_ID );

        // Lock the map
        Threading::RecursiveScopeLock lock( m_mutex );

        // Remove from lookup map
        auto nameLookupIter = m_entityNameLookupMap.find( pEntity->m_name );
        EE_ASSERT( nameLookupIter != m_entityNameLookupMap.end() );
        m_entityNameLookupMap.erase( nameLookupIter );

        // Rename
        pEntity->m_name = GenerateUniqueEntityNameID( newNameID );

        // Add to lookup map
        m_entityNameLookupMap.insert( TPair<StringID, Entity*>( pEntity->m_name, pEntity ) );
    }

    StringID EntityMap::GenerateUniqueEntityNameID( StringID desiredNameID ) const
    {
        auto GenerateUniqueName = [] ( InlineString const& baseName, int32_t counterValue )
        {
            InlineString finalName;

            if ( baseName.length() > 3 )
            {
                // Check if the last three characters are a numeric set, if so then increment the value and replace them
                if ( isdigit( baseName[baseName.length() - 1] ) && isdigit( baseName[baseName.length() - 2] ) && isdigit( baseName[baseName.length() - 3] ) )
                {
                    finalName.sprintf( "%s%03u", baseName.substr( 0, baseName.length() - 3 ).c_str(), counterValue );
                    return finalName;
                }
            }

            finalName.sprintf( "%s %03u", baseName.c_str(), counterValue );
            return finalName;
        };

        //-------------------------------------------------------------------------

        EE_ASSERT( desiredNameID.IsValid() );

        InlineString desiredName = desiredNameID.c_str();
        InlineString finalName = desiredName;
        StringID finalNameID( finalName.c_str() );

        uint32_t counter = 0;
        bool isUniqueName = false;
        while ( !isUniqueName )
        {
            // Check the lookup map
            isUniqueName = m_entityNameLookupMap.find( finalNameID ) == m_entityNameLookupMap.end();

            // If we found a name clash, generate a new name and try again
            if ( !isUniqueName )
            {
                finalName = GenerateUniqueName( desiredName, counter );
                finalNameID = StringID( finalName.c_str() );
                counter++;
            }
        }

        //-------------------------------------------------------------------------

        return finalNameID;
    }

    void EntityMap::BeginComponentEdit( LoadingContext const& loadingContext, InitializationContext& initializationContext, EntityID const& entityID )
    {
        EE_ASSERT( Threading::IsMainThread() );

        auto pEntity = FindEntity( entityID );
        EE_ASSERT( pEntity != nullptr );
        EE_ASSERT( !VectorContains( m_editedEntities, pEntity ) ); // Starting multiple edits for the same entity?!

        if ( pEntity->IsInitialized() )
        {
            pEntity->Shutdown( initializationContext );
            ProcessEntityRegistrationRequests( initializationContext );
        }

        pEntity->UnloadComponents( loadingContext );
        m_entitiesCurrentlyLoading.erase_first_unsorted( pEntity );
        m_editedEntities.emplace_back( pEntity );
    }

    void EntityMap::EndComponentEdit( LoadingContext const& loadingContext, InitializationContext& initializationContext, EntityID const& entityID )
    {
        EE_ASSERT( Threading::IsMainThread() );

        auto pEntity = FindEntity( entityID );

        EE_ASSERT( pEntity != nullptr );
        EE_ASSERT( !VectorContains( m_entitiesCurrentlyLoading, pEntity ) );
        EE_ASSERT( VectorContains( m_editedEntities, pEntity ) ); // Cant end an edit that was never started!

        pEntity->LoadComponents( loadingContext );
        m_entitiesCurrentlyLoading.emplace_back( pEntity );
        m_editedEntities.erase_first_unsorted( pEntity );
    }

    //-------------------------------------------------------------------------

    void EntityMap::BeginHotReload( LoadingContext const& loadingContext, InitializationContext& initializationContext, TVector<Resource::ResourceRequesterID> const& usersToReload )
    {
        EE_ASSERT( Threading::IsMainThread() );
        EE_ASSERT( !usersToReload.empty() );
        EE_ASSERT( m_entitiesToHotReload.empty() );

        Threading::RecursiveScopeLock lock( m_mutex );

        // Run map loading code to ensure that any destroy entity request (that might be unloading resources are executed!)
        UpdateLoadingAndStateChanges( loadingContext, initializationContext );

        // There should never be any queued registration request at this stage!
        EE_ASSERT( initializationContext.m_registerForEntityUpdate.size_approx() == 0 && initializationContext.m_unregisterForEntityUpdate.size_approx() == 0 );
        EE_ASSERT( initializationContext.m_componentsToRegister.size_approx() == 0 && initializationContext.m_componentsToUnregister.size_approx() == 0 );

        // Generate list of entities to be reloaded
        for ( auto const& requesterID : usersToReload )
        {
            // See if the entity that needs a reload is in this map
            Entity* pFoundEntity = FindEntity( EntityID( requesterID.GetID() ) );
            if ( pFoundEntity != nullptr )
            {
                m_entitiesToHotReload.emplace_back( pFoundEntity );
            }
        }

        // Request shutdown for any entities that are initialized
        bool someEntitiesRequireShutdown = false;
        for ( auto pEntityToHotReload : m_entitiesToHotReload )
        {
            if ( pEntityToHotReload->IsInitialized() )
            {
                pEntityToHotReload->Shutdown( initializationContext );
                someEntitiesRequireShutdown = true;
            }
        }

        // Process unregistration requests
        if ( someEntitiesRequireShutdown )
        {
            ProcessEntityRegistrationRequests( initializationContext );
        }

        // Unload entities
        for ( auto pEntityToHotReload : m_entitiesToHotReload )
        {
            EE_ASSERT( !pEntityToHotReload->IsInitialized() );

            // We might still be loading this entity so remove it from the loading requests
            m_entitiesCurrentlyLoading.erase_first_unsorted( pEntityToHotReload );

            // Request unload of the components (client system needs to ensure that all resource requests are processed)
            pEntityToHotReload->UnloadComponents( loadingContext );
        }
    }

    void EntityMap::EndHotReload( LoadingContext const& loadingContext )
    {
        EE_ASSERT( Threading::IsMainThread() );

        Threading::RecursiveScopeLock lock( m_mutex );

        for ( auto pEntityToHotReload : m_entitiesToHotReload )
        {
            EE_ASSERT( pEntityToHotReload->IsUnloaded() );
            pEntityToHotReload->LoadComponents( loadingContext );
            m_entitiesCurrentlyLoading.emplace_back( pEntityToHotReload );
        }

        m_entitiesToHotReload.clear();
    }
    #endif
}