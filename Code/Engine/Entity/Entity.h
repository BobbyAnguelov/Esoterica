#pragma once

#include "EntitySpatialComponent.h"
#include "Engine/UpdateStage.h"
#include "System/Threading/Threading.h"
#include "System/Types/Event.h"

//-------------------------------------------------------------------------

namespace EE
{
    class SystemRegistry;
    class EntitySystem;
    class EntityWorldUpdateContext;

    namespace EntityModel
    {
        struct ActivationContext;
        struct SerializedEntityDescriptor;
        class SerializedEntityCollection;
        struct Serializer;
    }

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
    //-------------------------------------------------------------------------

    class EE_ENGINE_API Entity : public IRegisteredType
    {
        EE_REGISTER_TYPE( Entity );

        friend class EntityWorld;
        friend EntityModel::Serializer;
        friend EntityModel::EntityMap;
        friend EntityModel::EntityMapEditor;
        template<typename T> friend struct TEntityAccessor;

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

        // Event that's fired whenever the spatial attachment state changes
        static TEvent<Entity*>                  s_entitySpatialAttachmentStateUpdatedEvent;

        // Registration state
        enum class RegistrationStatus : uint8_t
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
            Activated,
        };

        // Event that's fired whenever a component/system is actually added or removed
        static TEventHandle<Entity*> OnEntityUpdated() { return s_entityUpdatedEvent; }

        // Event that's fired whenever an entities internal state changes and it requires an state update
        static TEventHandle<Entity*> OnEntityInternalStateUpdated() { return s_entityInternalStateUpdatedEvent; }

        // Event that's fired whenever the spatial attachment state changes
        static TEventHandle<Entity*> OnEntitySpatialAttachmentStateUpdated() { return s_entitySpatialAttachmentStateUpdatedEvent; }

    public:

        Entity() = default;
        Entity( StringID name ) : m_name( name ) {}
        ~Entity();

        // Entity Info
        //-------------------------------------------------------------------------

        inline EntityID const& GetID() const { return m_ID; }
        inline StringID GetName() const { return m_name; }
        inline EntityMapID const& GetMapID() const { return m_mapID; }
        inline uint32_t GetNumComponents() const { return (uint32_t) m_components.size(); }
        inline uint32_t GetNumSystems() const { return (uint32_t) m_systems.size(); }

        // Spatial Info
        //-------------------------------------------------------------------------

        // Does this entity have any spatial components
        inline bool IsSpatialEntity() const { return m_pRootSpatialComponent != nullptr; }
        
        inline SpatialEntityComponent const* GetRootSpatialComponent() const { return m_pRootSpatialComponent; }
        inline SpatialEntityComponent* GetRootSpatialComponent() { return m_pRootSpatialComponent; }
        inline ComponentID const& GetRootSpatialComponentID() const { return m_pRootSpatialComponent->GetID(); }
        inline OBB const& GetRootSpatialComponentWorldBounds() const { EE_ASSERT( IsSpatialEntity() ); return m_pRootSpatialComponent->GetWorldBounds(); }

        // Get the world transform for this entity i.e. the transform of the root spatial component
        inline Transform const& GetWorldTransform() const { EE_ASSERT( IsSpatialEntity() ); return m_pRootSpatialComponent->GetLocalTransform(); }
        
        // Set the world transform for this entity i.e. the transform of the root spatial component
        inline void SetWorldTransform( Transform const& worldTransform ) const { EE_ASSERT( IsSpatialEntity() ); return m_pRootSpatialComponent->SetLocalTransform( worldTransform ); }
        
        inline bool HasSpatialParent() const { return m_pParentSpatialEntity != nullptr; }
        inline Entity* GetSpatialParent() const { return m_pParentSpatialEntity; }
        inline EntityID const& GetSpatialParentID() const { EE_ASSERT( HasSpatialParent() ); return m_pParentSpatialEntity->GetID(); }
        
        inline StringID const& GetAttachmentSocketID() const { EE_ASSERT( HasSpatialParent() ); return m_parentAttachmentSocketID; }
        inline bool HasAttachedEntities() const { return !m_attachedEntities.empty(); }
        inline Transform GetAttachmentSocketTransform( StringID socketID ) const { EE_ASSERT( IsSpatialEntity() ); return m_pRootSpatialComponent->GetAttachmentSocketTransform( socketID ); }

        #if EE_DEVELOPMENT_TOOLS
        // Get the world bounds for this entity i.e. the combined bounds of all spatial components
        // Warning!!! This is incredibly expensive
        OBB GetCombinedWorldBounds() const;
        #endif

        // Status
        //-------------------------------------------------------------------------

        inline bool IsAddedToMap() const { return m_mapID.IsValid(); }
        inline bool IsActivated() const { return m_status == Status::Activated; }
        inline bool IsRegisteredForUpdates() const { return m_updateRegistrationStatus == RegistrationStatus::Registered; }
        inline bool HasRequestedComponentLoad() const { return m_status != Status::Unloaded; }
        inline bool IsLoaded() const { return m_status == Status::Loaded; }
        inline bool IsUnloaded() const { return m_status == Status::Unloaded; }
        inline bool HasStateChangeActionsPending() const { return !m_deferredActions.empty(); }

        // Components
        //-------------------------------------------------------------------------
        // NB!!! Add and remove operations execute immediately for unloaded entities BUT will be deferred to the next loading phase for loaded entities

        inline TVector<EntityComponent*> const& GetComponents() const { return m_components; }

        inline EntityComponent* FindComponent( ComponentID const& componentID )
        {
            auto foundIter = eastl::find( m_components.begin(), m_components.end(), componentID, [] ( EntityComponent* pComponent, ComponentID const& ID ) { return pComponent->GetID() == ID; } );
            return ( foundIter != m_components.end() ) ? *foundIter : nullptr;
        }

        inline EntityComponent const* FindComponent( ComponentID const& componentID ) const
        {
            auto foundIter = eastl::find( m_components.begin(), m_components.end(), componentID, [] ( EntityComponent* pComponent, ComponentID const& ID ) { return pComponent->GetID() == ID; } );
            return ( foundIter != m_components.end() ) ? *foundIter : nullptr;
        }

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

        inline TVector<EntitySystem*> const& GetSystems() const { return m_systems; }

        template<typename T>
        T* GetSystem()
        {
            static_assert( std::is_base_of<EntitySystem, T>::value, "T has to derive from IEntitySystem" );
            for ( auto pSystem : m_systems )
            {
                if ( pSystem->GetTypeInfo()->m_ID == T::GetStaticTypeID() )
                {
                    return static_cast<T*>( pSystem );
                }
            }

            return nullptr;
        }

        void CreateSystem( TypeSystem::TypeInfo const* pSystemTypeInfo );

        template<typename T> 
        inline void CreateSystem()
        {
            static_assert( std::is_base_of<EE::EntitySystem, T>::value, "Invalid system type detected" );
            EE_ASSERT( !VectorContains( m_systems, T::s_pTypeInfo->m_ID, [] ( EntitySystem* pSystem, TypeSystem::TypeID systemTypeID ) { return pSystem->GetTypeInfo()->m_ID == systemTypeID; } ) );
            CreateSystem( T::s_pTypeInfo );
        }

        void DestroySystem( TypeSystem::TypeID systemTypeID );
        void DestroySystem( TypeSystem::TypeInfo const* pSystemTypeInfo );

        template<typename T>
        inline void DestroySystem()
        {
            static_assert( std::is_base_of<EE::EntitySystem, T>::value, "Invalid system type detected" );
            EE_ASSERT( VectorContains( m_systems, T::s_pTypeInfo->m_ID, [] ( EntitySystem* pSystem, TypeSystem::TypeID systemTypeID ) { return pSystem->GetTypeInfo()->m_ID == systemTypeID; } ) );
            DestroySystem( T::s_pTypeInfo );
        }

    private:

        // This function will search through the spatial hierarchy of this entity and return the first component it finds that contains a socket with the specified socket ID
        SpatialEntityComponent* FindSocketAttachmentComponent( SpatialEntityComponent* pComponentToSearch, StringID socketID ) const;
        inline SpatialEntityComponent* FindSocketAttachmentComponent( StringID socketID ) const { EE_ASSERT( IsSpatialEntity() ); return FindSocketAttachmentComponent( m_pRootSpatialComponent, socketID ); }

        // Set attachment info - This will set the ptr to the parent entity and add this entity to the parent entity's attached entity list
        void SetSpatialParent( Entity* pParentEntity, StringID socketID );

        // Clear attachment info - Assumes spatial attachment has not been created
        void ClearSpatialParent();

        // Create the component-to-component attachment between this entity and the parent entity
        void CreateSpatialAttachment();

        // Destroy the component-to-component attachment between this entity and the parent entity
        void DestroySpatialAttachment();

        // Update the attachment hierarchy, required when we have made changes to this entity's spatial components or the spatial component hierarchy
        void RefreshChildSpatialAttachments();

        // Removes an internal component from the current hierarchy while awaiting destruction
        void RemoveComponentFromSpatialHierarchy( SpatialEntityComponent* pComponent );

        //-------------------------------------------------------------------------

        // Generate the per-stage update lists for this entity
        void GenerateSystemUpdateList();

        // Run Entity Systems
        void UpdateSystems( EntityWorldUpdateContext const& context );

        // Registers a component with all the local entity systems
        void RegisterComponentWithLocalSystems( EntityComponent* pComponent );

        // Unregister a component from all the local entity systems
        void UnregisterComponentFromLocalSystems( EntityComponent* pComponent );

        //-------------------------------------------------------------------------

        // Update internal entity state and execute all deferred actions
        bool UpdateEntityState( EntityModel::EntityLoadingContext const& loadingContext, EntityModel::ActivationContext& activationContext );

        // Request initial load of all components
        void LoadComponents( EntityModel::EntityLoadingContext const& loadingContext );

        // Request final unload of all components
        void UnloadComponents( EntityModel::EntityLoadingContext const& loadingContext );

        // Called when an entity finishes Loading successfully - Registers components with system, creates spatial attachments. Will attempt to activate all attached entities
        void Activate( EntityModel::ActivationContext& activationContext );

        // Called just before an entity fully unloads - Unregisters components from systems, breaks spatial attachments. Will attempt to deactivate all attached entities
        void Deactivate( EntityModel::ActivationContext& activationContext );

        // Immediate functions can be executed immediately for unloaded entities allowing us to skip the deferral of the operation
        void CreateSystemImmediate( TypeSystem::TypeInfo const* pSystemTypeInfo );
        void DestroySystemImmediate( TypeSystem::TypeInfo const* pSystemTypeInfo );
        void AddComponentImmediate( EntityComponent* pComponent, SpatialEntityComponent* pParentSpatialComponent );
        void DestroyComponentImmediate( EntityComponent* pComponent );

        // Deferred functions are execute as part of the internal state update and will execute the immediate version as well as perform additional operations ( e.g. load/unload/etc. )
        void CreateSystemDeferred( EntityModel::EntityLoadingContext const& loadingContext, TypeSystem::TypeInfo const* pSystemTypeInfo );
        void DestroySystemDeferred( EntityModel::EntityLoadingContext const& loadingContext, TypeSystem::TypeInfo const* pSystemTypeInfo );
        void AddComponentDeferred( EntityModel::EntityLoadingContext const& loadingContext, EntityComponent* pComponent, SpatialEntityComponent* pParentSpatialComponent );
        void DestroyComponentDeferred( EntityModel::EntityLoadingContext const& loadingContext, EntityComponent* pComponent );

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        // This will deactivate the specified component to allow for its property state to be edited safely. Can be called multiple times (once per component edited)
        void ComponentEditingDeactivate( EntityModel::ActivationContext& activationContext, ComponentID const& componentID );

        // This will unload the specified component to allow for its property state to be edited safely. Can be called multiple times (once per component edited)
        void ComponentEditingUnload( EntityModel::EntityLoadingContext const& loadingContext, ComponentID const& componentID );

        // This will end any component editing for this frame and load all unloaded components, should only be called once per frame when needed!
        void EndComponentEditing( EntityModel::EntityLoadingContext const& loadingContext );

        // Generates a unique component name based on the desired name
        StringID GenerateUniqueComponentName( EntityComponent* pComponent, StringID desiredNameID ) const;

        // Rename an existing component - this ensures that component names remain unique
        void RenameComponent( EntityComponent* pComponent, StringID newNameID );
        #endif

    protected:

        EntityID                                        m_ID = EntityID( this );                                            // The unique ID of this entity ( globally unique and generated at runtime )
        EntityMapID                                     m_mapID;                                                            // The ID of the map that owns this entity
        EE_REGISTER StringID                           m_name;                                                             // The name of the entity, only unique within the context of a map
        Status                                          m_status = Status::Unloaded;
        RegistrationStatus                              m_updateRegistrationStatus = RegistrationStatus::Unregistered;      // Is this entity registered for frame updates

        TVector<EntitySystem*>                          m_systems;
        TVector<EntityComponent*>                       m_components;
        SystemUpdateList                                m_systemUpdateLists[(int8_t) UpdateStage::NumStages];

        SpatialEntityComponent*                         m_pRootSpatialComponent = nullptr;                                  // This spatial component defines our world position
        TVector<Entity*>                                m_attachedEntities;                                                 // The list of entities that are attached to this entity
        Entity*                                         m_pParentSpatialEntity = nullptr;                                   // The parent entity we are attached to
        EE_EXPOSE StringID                             m_parentAttachmentSocketID;                                         // The socket that we are attached to on the parent
        bool                                            m_isSpatialAttachmentCreated = false;                               // Has the actual component-to-component attachment been created

        TVector<EntityInternalStateAction>              m_deferredActions;                                                  // The set of internal entity state changes that need to be executed
        Threading::Mutex                                m_internalStateMutex;                                               // A mutex that needs to be lock due to internal state changes
    };
 }