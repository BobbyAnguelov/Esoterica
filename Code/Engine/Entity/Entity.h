#pragma once

#include "EntitySpatialComponent.h"
#include "Engine/UpdateStage.h"
#include "Base/Threading/Threading.h"
#include "Base/Types/Event.h"

//-------------------------------------------------------------------------
// Entity
//-------------------------------------------------------------------------
// This is container for a set of components and systems
//
//  * Owns and is responsible for its systems and components and their memory
//
//  * Any runtime changes to an entity's internal state (components/systems) 
//  * will be deferred to the next entity world load phase to ensure that we
//  * dont interfere with any current operations for this frame
//
//  Note:   Initialized means that the entity is registered with all systems 
//          and is registered for updates
//
//  Spatial attachments imply some level of ownership in terms of initialization
//  Attached entities are never directly initialized or shutdown but instead
//  the parent entity is responsible for init/shutdown of attached children
//
//-------------------------------------------------------------------------

namespace EE
{
    class SystemRegistry;
    class EntitySystem;
    class EntityWorldUpdateContext;

    namespace EntityModel
    {
        struct InitializationContext;
        struct SerializedEntityDescriptor;
        class SerializedEntityCollection;
        struct Serializer;

        #if EE_DEVELOPMENT_TOOLS
        class EntityEditorWorkspace;
        #endif
    }

    //-------------------------------------------------------------------------

    class EE_ENGINE_API Entity : public IReflectedType
    {
        EE_REFLECT_TYPE( Entity );

        friend EntityModel::Serializer;
        friend EntityModel::EntityMap;

        #if EE_DEVELOPMENT_TOOLS
        friend EntityModel::EntityEditorWorkspace;
        #endif

        using SystemUpdateList = TVector<EntitySystem*>;

        // Entity internal state change actions
        struct EntityInternalStateAction
        {
            enum class Type
            {
                Unknown,
                CreateSystem,
                DestroySystem,
                AddComponent,
                DestroyComponent,
                WaitForComponentUnregistration,
            };

            void const*                         m_ptr = nullptr;            // Can either be ptr to a system or a component
            ComponentID                         m_parentComponentID;        // Contains the ID of the component
            Type                                m_type = Type::Unknown;     // Type of action
        };

        // Event that's fired whenever a component/system is actually added or removed
        static TEvent<Entity*>                  s_entityUpdatedEvent;

        // Event that's fired whenever a component/system is added or removed
        static TEvent<Entity*>                  s_entityInternalStateUpdatedEvent;

        // Registration state
        enum class UpdateRegistrationStatus : uint8_t
        {
            Unregistered = 0,
            QueuedForRegister,
            QueuedForUnregister,
            Registered,
        };

    public:

        enum class Status : uint8_t
        {
            Unloaded = 0,
            Loaded,
            Initialized,
        };

        enum class SpatialAttachmentRule
        {
            KeepWorldTransform,
            KeepLocalTranform
        };

        // Event that's fired whenever a component/system is actually added or removed
        static TEventHandle<Entity*> OnEntityUpdated() { return s_entityUpdatedEvent; }

        // Event that's fired whenever an entities internal state changes and it requires an state update
        static TEventHandle<Entity*> OnEntityInternalStateUpdated() { return s_entityInternalStateUpdatedEvent; }

    public:

        Entity() = default;
        Entity( StringID name ) : m_name( name ) {}
        ~Entity();

        // Entity Info
        //-------------------------------------------------------------------------

        // Get the entity ID, this is a globally unique transient ID (it is generated at runtime)
        inline EntityID const& GetID() const { return m_ID; }

        // Get the serialized name ID for this entity, this is only unique within the context of a map
        inline StringID GetNameID() const { return m_name; }

        // Get the ID of the map this entity belongs to
        inline EntityMapID const& GetMapID() const { return m_mapID; }

        // Get a list of all resource referenced by this entity
        void GetReferencedResources( TVector<ResourceID>& outReferencedResources ) const;

        // Spatial Info
        //-------------------------------------------------------------------------

        // Does this entity have any spatial components
        inline bool IsSpatialEntity() const { return m_pRootSpatialComponent != nullptr; }
        
        inline SpatialEntityComponent const* GetRootSpatialComponent() const { return m_pRootSpatialComponent; }
        inline SpatialEntityComponent* GetRootSpatialComponent() { return m_pRootSpatialComponent; }
        inline ComponentID const& GetRootSpatialComponentID() const { return m_pRootSpatialComponent->GetID(); }
        inline OBB const& GetRootSpatialComponentWorldBounds() const { EE_ASSERT( IsSpatialEntity() ); return m_pRootSpatialComponent->GetWorldBounds(); }

        // Get the world AABB for this entity i.e. the combined bounds of all spatial components
        AABB GetCombinedWorldBounds() const;

        // Get the local transform for this entity i.e. the local transform of the root spatial component
        inline Transform const& GetLocalTransform() const { EE_ASSERT( IsSpatialEntity() ); return m_pRootSpatialComponent->GetLocalTransform(); }

        // Get the world transform for this entity i.e. the world transform of the root spatial component
        inline Transform const& GetWorldTransform() const { EE_ASSERT( IsSpatialEntity() ); return m_pRootSpatialComponent->GetWorldTransform(); }
        
        // Set the world transform for this entity i.e. the transform of the root spatial component
        inline void SetWorldTransform( Transform const& worldTransform ) const { EE_ASSERT( IsSpatialEntity() ); return m_pRootSpatialComponent->SetWorldTransform( worldTransform ); }
        
        // Do we have a spatial parent entity
        inline bool HasSpatialParent() const { return m_pParentSpatialEntity != nullptr; }

        // Get our spatial parent entity
        inline Entity* GetSpatialParent() const { return m_pParentSpatialEntity; }

        // Get the ID for our spatial parent entity - Warning: Do not call without checking if we actually have a parent
        inline EntityID const& GetSpatialParentID() const { EE_ASSERT( HasSpatialParent() ); return m_pParentSpatialEntity->GetID(); }

        // Are we under the spatial hierarchy of the supplied entity?
        bool IsSpatialChildOf( Entity const* pPotentialParent ) const;

        // Set the spatial parent
        // This will set the ptr to the parent entity and add this entity to the parent entity's attached entity list
        // This will also update any spatial attachments between components
        // Note: this will lock a bunch of mutexes so be careful when you call this
        void SetSpatialParent( Entity* pParentEntity, StringID socketID = StringID(), SpatialAttachmentRule attachmentRule = SpatialAttachmentRule::KeepWorldTransform);

        // Clears the spatial parent for this entity
        // Note: this will lock a bunch of mutexes so be careful when you call this
        void ClearSpatialParent( SpatialAttachmentRule attachmentRule = SpatialAttachmentRule::KeepWorldTransform );

        // Get the socketID that we are attached to
        inline StringID const& GetAttachmentSocketID() const { EE_ASSERT( HasSpatialParent() ); return m_parentAttachmentSocketID; }
        
        // Get the actual world space socket transform that we are attached to
        inline Transform GetAttachmentSocketTransform( StringID socketID ) const { EE_ASSERT( IsSpatialEntity() ); return m_pRootSpatialComponent->GetAttachmentSocketTransform( socketID ); }

        // Do we have any entities attached to us
        inline bool HasAttachedEntities() const { return !m_attachedEntities.empty(); }

        // Get all attached entities
        TVector<Entity*> const& GetAttachedEntities() const { return m_attachedEntities; }

        // Status
        //-------------------------------------------------------------------------

        inline bool IsAddedToMap() const { return m_mapID.IsValid(); }
        inline bool IsInitialized() const { return m_status == Status::Initialized; }
        inline bool IsRegisteredForUpdates() const { return m_updateRegistrationStatus == UpdateRegistrationStatus::Registered; }
        inline bool HasRequestedComponentLoad() const { return m_status != Status::Unloaded; }
        inline bool IsLoaded() const { return m_status == Status::Loaded; }
        inline bool IsUnloaded() const { return m_status == Status::Unloaded; }
        inline bool HasStateChangeActionsPending() const { return !m_deferredActions.empty(); }

        // Components
        //-------------------------------------------------------------------------
        // NB!!! Add and remove operations execute immediately for unloaded entities BUT will be deferred to the next loading phase for loaded entities

        // Get the number of components this entity owns
        inline uint32_t GetNumComponents() const { return (uint32_t) m_components.size(); }

        inline TVector<EntityComponent*> const& GetComponents() const { return m_components; }

        inline EntityComponent const* FindComponent( ComponentID const& componentID ) const
        {
            auto foundIter = eastl::find( m_components.begin(), m_components.end(), componentID, [] ( EntityComponent* pComponent, ComponentID const& ID ) { return pComponent->GetID() == ID; } );
            return ( foundIter != m_components.end() ) ? *foundIter : nullptr;
        }
        
        inline EntityComponent* FindComponent( ComponentID const& componentID ) { return const_cast<EntityComponent*>( const_cast<Entity const*>( this )->FindComponent( componentID ) ); }

        // Create a new component of the specified type
        void CreateComponent( TypeSystem::TypeInfo const* pComponentTypeInfo, ComponentID const& parentSpatialComponentID = ComponentID() );

        // Add a new component. For spatial component, you can optionally specify a component to attach to. 
        // If this is unset, the component will be attached to the root component (or will become the root component if one doesnt exist)
        void AddComponent( EntityComponent* pComponent, ComponentID const& parentSpatialComponentID = ComponentID() );

        // Destroys a component on this entity
        void DestroyComponent( ComponentID const& componentID );

        // Systems
        //-------------------------------------------------------------------------
        // NB!!! Add and remove operations execute immediately for unloaded entities BUT will be deferred to the next loading phase for loaded entities

        // Get the number of systems this entity owns
        inline uint32_t GetNumSystems() const { return (uint32_t) m_systems.size(); }

        // Get all systems
        inline TVector<EntitySystem*> const& GetSystems() const { return m_systems; }

        // Run Entity Systems
        void UpdateSystems( EntityWorldUpdateContext const& context );

        // Get a specific system
        template<typename T>
        T* GetSystem()
        {
            static_assert( std::is_base_of<EntitySystem, T>::value, "T has to derive from IEntitySystem" );
            for ( auto pSystem : m_systems )
            {
                if ( pSystem->GetTypeInfo()->m_ID == T::GetStaticTypeID() )
                {
                    return reinterpret_cast<T*>( pSystem );
                }
            }

            return nullptr;
        }

        // Request creation of a new system
        void CreateSystem( TypeSystem::TypeInfo const* pSystemTypeInfo );

        // Request creation of a new system
        template<typename T> 
        inline void CreateSystem()
        {
            static_assert( std::is_base_of<EE::EntitySystem, T>::value, "Invalid system type detected" );
            EE_ASSERT( !VectorContains( m_systems, T::s_pTypeInfo->m_ID, [] ( EntitySystem* pSystem, TypeSystem::TypeID systemTypeID ) { return pSystem->GetTypeInfo()->m_ID == systemTypeID; } ) );
            CreateSystem( T::s_pTypeInfo );
        }

        // Destroy an existing system
        void DestroySystem( TypeSystem::TypeID systemTypeID );

        // Destroy an existing system
        void DestroySystem( TypeSystem::TypeInfo const* pSystemTypeInfo );

        // Destroy an existing system
        template<typename T>
        inline void DestroySystem()
        {
            static_assert( std::is_base_of<EE::EntitySystem, T>::value, "Invalid system type detected" );
            EE_ASSERT( VectorContains( m_systems, T::s_pTypeInfo->m_ID, [] ( EntitySystem* pSystem, TypeSystem::TypeID systemTypeID ) { return pSystem->GetTypeInfo()->m_ID == systemTypeID; } ) );
            DestroySystem( T::s_pTypeInfo );
        }

        // Tools Helpers
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        // Finds a component by it's name ID
        inline EntityComponent const* FindComponentByName( StringID const& componentID ) const
        {
            auto foundIter = eastl::find( m_components.begin(), m_components.end(), componentID, [] ( EntityComponent* pComponent, StringID const& ID ) { return pComponent->GetNameID() == ID; } );
            return ( foundIter != m_components.end() ) ? *foundIter : nullptr;
        }

        // Finds a component by it's name ID
        inline EntityComponent* FindComponentByName( StringID const& componentID ) { return const_cast<EntityComponent*>( const_cast<Entity const*>( this )->FindComponentByName( componentID ) ); }

        // Generates a unique component name based on the desired name
        StringID GenerateUniqueComponentNameID( EntityComponent* pComponent, StringID desiredNameID ) const;

        // Rename an existing component - this ensures that component names remain unique
        void RenameComponent( EntityComponent* pComponent, StringID newNameID );
        #endif

    private:

        // This function will search through the spatial hierarchy of this entity and return the first component it finds that contains a socket with the specified socket ID
        SpatialEntityComponent* FindSocketAttachmentComponent( SpatialEntityComponent* pComponentToSearch, StringID socketID ) const;

        // This function will search through the spatial hierarchy of this entity and return the first component it finds that contains a socket with the specified socket ID
        inline SpatialEntityComponent* FindSocketAttachmentComponent( StringID socketID ) const { EE_ASSERT( IsSpatialEntity() ); return FindSocketAttachmentComponent( m_pRootSpatialComponent, socketID ); }

        // Create the component-to-component attachment between this entity and the parent entity
        void CreateSpatialAttachment();

        // Destroy the component-to-component attachment between this entity and the parent entity
        void DestroySpatialAttachment( SpatialAttachmentRule attachmentRule );

        // Update the attachment hierarchy, required when we have made changes to this entity's spatial components or the spatial component hierarchy
        void RefreshChildSpatialAttachments();

        // Removes an internal component from the current hierarchy while awaiting destruction
        void RemoveComponentFromSpatialHierarchy( SpatialEntityComponent* pComponent );

        //-------------------------------------------------------------------------

        // Generate the per-stage update lists for this entity
        void GenerateSystemUpdateList();

        // Registers a component with all the local entity systems
        void RegisterComponentWithLocalSystems( EntityComponent* pComponent );

        // Unregister a component from all the local entity systems
        void UnregisterComponentFromLocalSystems( EntityComponent* pComponent );

        //-------------------------------------------------------------------------

        // Update internal entity state and execute all deferred actions
        bool UpdateEntityState( EntityModel::LoadingContext const& loadingContext, EntityModel::InitializationContext& initializationContext );

        // Request initial load of all components
        void LoadComponents( EntityModel::LoadingContext const& loadingContext );

        // Request final unload of all components
        void UnloadComponents( EntityModel::LoadingContext const& loadingContext );

        // Called when an entity finishes Loading successfully - Registers components with system, creates spatial attachments.
        void Initialize( EntityModel::InitializationContext& initializationContext );

        // Called just before an entity fully unloads - Unregisters components from systems, breaks spatial attachments.
        void Shutdown( EntityModel::InitializationContext& initializationContext );

        // Immediate functions can be executed immediately for unloaded entities allowing us to skip the deferral of the operation
        void CreateSystemImmediate( TypeSystem::TypeInfo const* pSystemTypeInfo );
        void DestroySystemImmediate( TypeSystem::TypeInfo const* pSystemTypeInfo );
        void AddComponentImmediate( EntityComponent* pComponent, SpatialEntityComponent* pParentSpatialComponent );
        void DestroyComponentImmediate( EntityComponent* pComponent );

    protected:

        EntityID                                            m_ID = EntityID::Generate();                                            // The unique ID of this entity ( globally unique and generated at runtime )
        EntityMapID                                         m_mapID;                                                                // The ID of the map that owns this entity
        EE_REFLECT( "IsToolsReadOnly" : true ) StringID     m_name;                                                                 // The name of the entity, only unique within the context of a map
        Status                                              m_status = Status::Unloaded;
        UpdateRegistrationStatus                            m_updateRegistrationStatus = UpdateRegistrationStatus::Unregistered;    // Is this entity registered for frame updates

        TVector<EntitySystem*>                              m_systems;
        TVector<EntityComponent*>                           m_components;
        SystemUpdateList                                    m_systemUpdateLists[(int8_t) UpdateStage::NumStages];

        SpatialEntityComponent*                             m_pRootSpatialComponent = nullptr;                                      // This spatial component defines our world position
        TVector<Entity*>                                    m_attachedEntities;                                                     // The list of entities that are attached to this entity
        Entity*                                             m_pParentSpatialEntity = nullptr;                                       // The parent entity we are attached to
        EE_REFLECT() StringID                               m_parentAttachmentSocketID;                                             // The socket that we are attached to on the parent
        bool                                                m_isSpatialAttachmentCreated = false;                                   // Has the actual component-to-component attachment been created

        TVector<EntityInternalStateAction>                  m_deferredActions;                                                      // The set of internal entity state changes that need to be executed
        Threading::RecursiveMutex                           m_internalStateMutex;                                                   // A mutex that needs to be lock due to internal state changes
    };
 }