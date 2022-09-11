#pragma once

#include "Engine/_Module/API.h"
#include "EntityDescriptors.h"
#include "System/Types/Event.h"
#include "System/Threading/Threading.h"
#include "System/Resource/ResourcePtr.h"
#include "System/Math/Transform.h"

//-------------------------------------------------------------------------

namespace EE
{
    class Entity;
    class EntityWorld;

    //-------------------------------------------------------------------------

    namespace EntityModel
    {
        struct EntityLoadingContext;
        struct ActivationContext;
        class SerializedEntityCollection;

        //-------------------------------------------------------------------------
        // Entity Map
        //-------------------------------------------------------------------------
        // This is a logical grouping of entities whose lifetime is linked
        // e.g. the game level, a cell in a open world game, etc.
        //
        // * Maps manage lifetime, loading and activation of entities
        // * All map operations are threadsafe using a standard mutex
        //
        // There are some quirks with the addition/removal of entities:
        // * When adding an entity, it may take a frame to make it to the actual entities list but is immediately added to the lookup maps
        // * Removal functions in the opposite manner, entities are only removed from the maps once they are removed entirely from the map
        //
        //-------------------------------------------------------------------------

        class EE_ENGINE_API EntityMap
        {
            friend EntityWorld;
            friend struct Serializer;

            enum class Status
            {
                LoadFailed = -1,
                Unloaded = 0,
                MapDescriptorLoading,
                MapEntitiesLoading,
                Loaded,
                Activated,
            };

            struct RemovalRequest
            {
                RemovalRequest( Entity* pEntity, bool shouldDestroy ) : m_pEntity( pEntity ), m_shouldDestroy( shouldDestroy ) {}

                Entity*     m_pEntity = nullptr;
                bool        m_shouldDestroy = false;
            };

        public:

            EntityMap(); // Default constructor creates a transient map
            EntityMap( ResourceID mapResourceID );
            EntityMap( EntityMap const& map );
            EntityMap( EntityMap&& map );
            ~EntityMap();

            EntityMap& operator=( EntityMap const& map );
            EntityMap& operator=( EntityMap&& map );

            // Map Info
            //-------------------------------------------------------------------------

            inline ResourceID const& GetMapResourceID() const { return m_pMapDesc.GetResourceID(); }
            inline EntityMapID GetID() const { return m_ID; }
            inline bool IsTransientMap() const { return m_isTransientMap; }

            // Loading and Activation
            //-------------------------------------------------------------------------

            void Load( EntityLoadingContext const& loadingContext );
            void Unload( EntityLoadingContext const& loadingContext );

            void Activate( EntityModel::ActivationContext& activationContext );
            void Deactivate( EntityModel::ActivationContext& activationContext );

            // Map State
            //-------------------------------------------------------------------------

            // Updates map loading and entity state, returns true if all loading/state changes are complete, false otherwise
            bool UpdateState( EntityLoadingContext const& loadingContext, EntityModel::ActivationContext& activationContext );

            // Do we have any pending entity addition or removal requests?
            inline bool HasPendingAddOrRemoveRequests() const { return ( m_entitiesToLoad.size() + m_entitiesToRemove.size() ) > 0; }

            bool IsLoading() const { return m_status == Status::MapDescriptorLoading || m_status == Status::MapEntitiesLoading; }
            inline bool IsLoaded() const { return m_status == Status::Loaded; }
            inline bool IsUnloaded() const { return m_status == Status::Unloaded; }
            inline bool HasLoadingFailed() const { return m_status == Status::LoadFailed; }
            inline bool IsActivated() const { return m_status == Status::Activated; }

            //-------------------------------------------------------------------------
            // Entity API
            //-------------------------------------------------------------------------

            // Get the number of entities in this map
            inline int32_t GetNumEntities() const { return (int32_t) m_entities.size(); }

            // Get all entities in the map. This includes all entities that are still loading/unloading, etc...
            inline TVector<Entity*> const& GetEntities() const { return m_entities; }

            // Finds an entity via its ID. This uses a lookup map so is relatively performant
            inline Entity* FindEntity( EntityID entityID ) const
            {
                auto iter = m_entityIDLookupMap.find( entityID );
                return ( iter != m_entityIDLookupMap.end() ) ? iter->second : nullptr;
            }

            // Does this map contain this entity?
            // If you are working with an entity and a map, prefer to check the map ID vs the entity's map ID!
            inline bool ContainsEntity( EntityID entityID ) const { return FindEntity( entityID ) != nullptr; }

            // Adds a set of entities to this map - Transfers ownership of the entities to the map
            // Additionally allows you to offset all the entities via the supplied offset transform
            // Takes 1 frame to be fully added
            void AddEntities( TVector<Entity*> const& entities, Transform const& offsetTransform = Transform::Identity );

            // Instantiates and adds an entity collection to the map
            // Additionally allows you to offset all the entities via the supplied offset transform
            // Takes 1 frame to be fully added
            void AddEntityCollection( TaskSystem* pTaskSystem, TypeSystem::TypeRegistry const& typeRegistry, SerializedEntityCollection const& entityCollectionDesc, Transform const& offsetTransform = Transform::Identity );

            // Add a newly created entity to the map - Transfers ownership of the entity to the map
            void AddEntity( Entity* pEntity );

            // Unload and remove an entity from the map - Transfer ownership of the entity to the calling code
            // May take multiple frames to be fully destroyed, as the deactivation/unload occurs during the loading update
            // Entity is free to be deleted by the user ONLY once it is in an unloaded state
            Entity* RemoveEntity( EntityID entityID );

            // Unload, remove and destroy entity in this map
            // May take multiple frames to be fully destroyed, as the removal occurs during the loading update
            void DestroyEntity( EntityID entityID );

            #if EE_DEVELOPMENT_TOOLS
            // Gets a unique entity name for this map given a specified desired name
            StringID GenerateUniqueEntityNameID( StringID desiredNameID ) const;

            // Gets a unique entity name for this map given a specified desired name
            inline StringID GenerateUniqueEntityNameID( char const* pName ) const { return GenerateUniqueEntityNameID( StringID( pName ) ); }

            // Rename an existing entity - allow renaming of existing entities, will ensure that the new name is unique
            void RenameEntity( Entity* pEntity, StringID newNameID );

            // Find an entity by name
            inline Entity* FindEntityByName( StringID nameID ) const
            {
                auto iter = m_entityNameLookupMap.find( nameID );
                return ( iter != m_entityNameLookupMap.end() ) ? iter->second : nullptr;
            }
            #endif

        private:

            //-------------------------------------------------------------------------
            // Editing / Hot-reloading
            //-------------------------------------------------------------------------
            // Editing and hot-reloading are blocking processes that run in stages

            #if EE_DEVELOPMENT_TOOLS
            // This function will deactivate and unload the specified component, allowing its properties to be edited safely!
            void ComponentEditingDeactivate( EntityModel::ActivationContext& activationContext, EntityID const& entityID, ComponentID const& componentID );

            // This function will deactivate and unload the specified component, allowing its properties to be edited safely!
            void ComponentEditingUnload( EntityLoadingContext const& loadingContext, EntityID const& entityID, ComponentID const& componentID );

            // Hot reloading is a blocking process that runs in stages
            //-------------------------------------------------------------------------

            // 1st Stage: Fills the hot-reload list and request deactivation for any entities that require a hot-reload
            void HotReloadDeactivateEntities( EntityModel::ActivationContext& activationContext, TVector<Resource::ResourceRequesterID> const& usersToReload );

            // 2nd Stage: Requests unload of all entities required hot-reload
            void HotReloadUnloadEntities( EntityLoadingContext const& loadingContext );

            // 3rd Stage: Requests load of all entities required hot-reload
            void HotReloadLoadEntities( EntityLoadingContext const& loadingContext );
            #endif

            //-------------------------------------------------------------------------

            // Called whenever the internal state of an entity changes, schedules the entity for loading
            void OnEntityStateUpdated( Entity* pEntity );

            bool ProcessMapUnloadRequest( EntityLoadingContext const& loadingContext, EntityModel::ActivationContext& activationContext );
            bool ProcessMapLoading( EntityLoadingContext const& loadingContext, EntityModel::ActivationContext& activationContext );
            void ProcessEntityAdditionAndRemoval( EntityLoadingContext const& loadingContext, EntityModel::ActivationContext& activationContext );
            bool ProcessEntityLoadingAndActivation( EntityLoadingContext const& loadingContext, EntityModel::ActivationContext& activationContext );

            // Remove entity
            Entity* RemoveEntityInternal( EntityID entityID, bool destroyEntityOnceRemoved );

            // Destroy all created entity instances
            void DestroyAllEntities();

        private:

            EntityMapID                                 m_ID = UUID::GenerateID(); // ID is always regenerated at creation time, do not rely on the ID being the same for a map on different runs
            Threading::RecursiveMutex                   m_mutex;
            TResourcePtr<SerializedEntityMap>           m_pMapDesc;
            TVector<Entity*>                            m_entities;
            THashMap<EntityID, Entity*>                 m_entityIDLookupMap; // All activated entities in the map
            TVector<Entity*>                            m_entitiesCurrentlyLoading;
            TInlineVector<Entity*, 5>                   m_entitiesToLoad;
            TInlineVector<RemovalRequest, 5>            m_entitiesToRemove;
            EventBindingID                              m_entityUpdateEventBindingID;
            Status                                      m_status = Status::Unloaded;
            bool                                        m_isUnloadRequested = false;
            bool                                        m_isMapInstantiated = false;
            bool const                                  m_isTransientMap = false; // If this is set, then this is a transient map i.e.created and managed at runtime and not loaded from disk

            #if EE_DEVELOPMENT_TOOLS
            THashMap<StringID, Entity*>                 m_entityNameLookupMap; // All entities that have attempted to load
            TVector<Entity*>                            m_entitiesToHotReload;
            TVector<Entity*>                            m_editedEntities;
            #endif
        };
    }
}