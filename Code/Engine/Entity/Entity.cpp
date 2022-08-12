#include "Entity.h"
#include "EntitySystem.h"
#include "EntityWorldUpdateContext.h"
#include "EntityActivationContext.h"
#include "EntityLoadingContext.h"
#include "EntityDescriptors.h"
#include "EntityLog.h"
#include "System/Resource/ResourceRequesterID.h"
#include "System/TypeSystem/TypeRegistry.h"
#include <eastl/sort.h>

//-------------------------------------------------------------------------

namespace EE
{
    TEvent<Entity*> Entity::s_entityUpdatedEvent;
    TEvent<Entity*> Entity::s_entityInternalStateUpdatedEvent;
    TEvent<Entity*> Entity::s_entitySpatialAttachmentStateUpdatedEvent;

    //-------------------------------------------------------------------------

    Entity* Entity::CreateFromDescriptor( TypeSystem::TypeRegistry const& typeRegistry, EntityModel::EntityDescriptor const& entityDesc )
    {
        EE_ASSERT( entityDesc.IsValid() );

        auto pEntityTypeInfo = Entity::s_pTypeInfo;
        EE_ASSERT( pEntityTypeInfo != nullptr );

        // Create new entity
        //-------------------------------------------------------------------------

        auto pEntity = reinterpret_cast<Entity*>( pEntityTypeInfo->CreateType() );
        pEntity->m_name = entityDesc.m_name;

        // Create entity components
        //-------------------------------------------------------------------------
        // Component descriptors are sorted during compilation, spatial components are first, followed by regular components

        for ( EntityModel::ComponentDescriptor const& componentDesc : entityDesc.m_components )
        {
            auto pEntityComponent = componentDesc.CreateTypeInstance<EntityComponent>( typeRegistry );
            EE_ASSERT( pEntityComponent != nullptr );

            TypeSystem::TypeInfo const* pTypeInfo = pEntityComponent->GetTypeInfo();
            EE_ASSERT( pTypeInfo != nullptr );

            // Set IDs and add to component lists
            pEntityComponent->m_name = componentDesc.m_name;
            pEntityComponent->m_entityID = pEntity->m_ID;
            pEntity->m_components.push_back( pEntityComponent );

            //-------------------------------------------------------------------------

            if ( componentDesc.IsSpatialComponent() )
            {
                // Set parent socket ID
                auto pSpatialEntityComponent = static_cast<SpatialEntityComponent*>( pEntityComponent );
                pSpatialEntityComponent->m_parentAttachmentSocketID = componentDesc.m_attachmentSocketID;

                // Set as root component
                if ( componentDesc.IsRootComponent() )
                {
                    EE_ASSERT( pEntity->m_pRootSpatialComponent == nullptr );
                    pEntity->m_pRootSpatialComponent = pSpatialEntityComponent;
                }
            }
        }

        // Create component spatial hierarchy
        //-------------------------------------------------------------------------

        int32_t const numComponents = (int32_t) pEntity->m_components.size();
        for ( int32_t spatialComponentIdx = 0; spatialComponentIdx < entityDesc.m_numSpatialComponents; spatialComponentIdx++ )
        {
            EntityModel::ComponentDescriptor const& spatialComponentDesc = entityDesc.m_components[spatialComponentIdx];
            EE_ASSERT( spatialComponentDesc.IsSpatialComponent() );

            // Skip the root component
            if ( spatialComponentDesc.IsRootComponent() )
            {
                EE_ASSERT( pEntity->GetRootSpatialComponent()->GetName() == spatialComponentDesc.m_name );
                continue;
            }

            // Todo: profile this lookup and if it becomes too costly, pre-compute the parent indices and serialize them
            int32_t const parentComponentIdx = entityDesc.FindComponentIndex( spatialComponentDesc.m_spatialParentName );
            EE_ASSERT( parentComponentIdx != InvalidIndex );

            auto pParentSpatialComponent = static_cast<SpatialEntityComponent*>( pEntity->m_components[parentComponentIdx] );
            if ( spatialComponentDesc.m_spatialParentName == pParentSpatialComponent->GetName() )
            {
                auto pSpatialComponent = static_cast<SpatialEntityComponent*>( pEntity->m_components[spatialComponentIdx] );
                pSpatialComponent->m_pSpatialParent = pParentSpatialComponent;

                pParentSpatialComponent->m_spatialChildren.emplace_back( pSpatialComponent );
            }
        }

        // Create entity systems
        //-------------------------------------------------------------------------

        for ( auto const& systemDesc : entityDesc.m_systems )
        {
            TypeSystem::TypeInfo const* pTypeInfo = typeRegistry.GetTypeInfo( systemDesc.m_typeID );
            auto pEntitySystem = reinterpret_cast<EntitySystem*>( pTypeInfo->CreateType() );
            EE_ASSERT( pEntitySystem != nullptr );

            pEntity->m_systems.push_back( pEntitySystem );
        }

        // Add to collection
        //-------------------------------------------------------------------------

        return pEntity;
    }

    //-------------------------------------------------------------------------

    Entity::~Entity()
    {
        EE_ASSERT( m_status == Status::Unloaded );
        EE_ASSERT( !m_isSpatialAttachmentCreated );
        EE_ASSERT( m_updateRegistrationStatus == RegistrationStatus::Unregistered );

        // Break all inter-entity links
        if ( IsSpatialEntity() )
        {
            // Clear any child attachments
            for ( auto pAttachedEntity : m_attachedEntities )
            {
                EE_ASSERT( pAttachedEntity->m_pParentSpatialEntity == this );
                pAttachedEntity->ClearSpatialParent();
            }

            // Clear any parent attachment we might have
            if ( m_pParentSpatialEntity != nullptr )
            {
                ClearSpatialParent();
            }
        }

        // Clean up deferred actions
        for ( auto& action : m_deferredActions )
        {
            // Add actions took ownership of the component to add, so it needs to be deleted
            // All other actions can be ignored
            if ( action.m_type == EntityInternalStateAction::Type::AddComponent )
            {
                auto pComponent = reinterpret_cast<EntityComponent const*>( action.m_ptr );
                EE::Delete( pComponent );
            }
        }
        m_deferredActions.clear();

        // Destroy Systems
        for ( auto& pSystem : m_systems )
        {
            EE::Delete( pSystem );
        }
        
        m_systems.clear();

        // Destroy components
        for ( auto& pComponent : m_components )
        {
            EE::Delete( pComponent );
        }

        m_components.clear();
    }

    #if EE_DEVELOPMENT_TOOLS
    OBB Entity::GetCombinedWorldBounds() const
    {
        EE_ASSERT( IsSpatialEntity() );

        TInlineVector<Vector, 64> points;

        for ( auto pComponent : m_components )
        {
            if ( auto pSC = TryCast<SpatialEntityComponent>( pComponent ) )
            {
                Vector corners[8];
                pSC->GetWorldBounds().GetCorners(corners);
                for ( auto i = 0; i < 8; i++ )
                {
                    points.emplace_back( corners[i] );
                }
            }
        }

        OBB const worldBounds( points.data(), (uint32_t) points.size() );
        return worldBounds;
    }
    #endif

    bool Entity::CreateDescriptor( TypeSystem::TypeRegistry const& typeRegistry, EntityModel::EntityDescriptor& outDesc ) const
    {
        EE_ASSERT( !outDesc.IsValid() );
        outDesc.m_name = m_name;

        // Get spatial parent
        if ( HasSpatialParent() )
        {
            outDesc.m_spatialParentName = m_pParentSpatialEntity->GetName();
            outDesc.m_attachmentSocketID = GetAttachmentSocketID();
        }

        // Components
        //-------------------------------------------------------------------------

        TVector<StringID> entityComponentList;

        for ( auto pComponent : m_components )
        {
            EntityModel::ComponentDescriptor componentDesc;
            componentDesc.m_name = pComponent->GetName();

            // Check for unique names
            if ( VectorContains( entityComponentList, componentDesc.m_name ) )
            {
                // Duplicate name detected!!
                EE_LOG_ENTITY_ERROR( this, "Entity", "Failed to create entity descriptor, duplicate component name detected: %s on entity %s", pComponent->GetName().c_str(), m_name.c_str() );
                return false;
            }
            else
            {
                entityComponentList.emplace_back( componentDesc.m_name );
            }

            // Spatial info
            auto pSpatialEntityComponent = TryCast<SpatialEntityComponent>( pComponent );
            if ( pSpatialEntityComponent != nullptr )
            {
                if ( pSpatialEntityComponent->HasSpatialParent() )
                {
                    EntityComponent const* pSpatialParentComponent = FindComponent( pSpatialEntityComponent->GetSpatialParentID() );
                    componentDesc.m_spatialParentName = pSpatialParentComponent->GetName();
                    componentDesc.m_attachmentSocketID = pSpatialEntityComponent->GetAttachmentSocketID();
                }

                componentDesc.m_isSpatialComponent = true;
            }

            // Type descriptor - Properties
            componentDesc.DescribeTypeInstance( typeRegistry, pComponent, true );

            // Add component
            outDesc.m_components.emplace_back( componentDesc );
            if ( componentDesc.m_isSpatialComponent )
            {
                outDesc.m_numSpatialComponents++;
            }
        }

        // Systems
        //-------------------------------------------------------------------------

        for ( auto pSystem : m_systems )
        {
            EntityModel::SystemDescriptor systemDesc;
            systemDesc.m_typeID = pSystem->GetTypeID();
            outDesc.m_systems.emplace_back( systemDesc );
        }

        return true;
    }

    //-------------------------------------------------------------------------

    void Entity::LoadComponents( EntityModel::EntityLoadingContext const& loadingContext )
    {
        EE_ASSERT( m_status == Status::Unloaded );

        for ( auto pComponent : m_components )
        {
            EE_ASSERT( pComponent->IsUnloaded() );
            pComponent->Load( loadingContext, Resource::ResourceRequesterID( m_ID.ToUint64() ) );
        }

        m_status = Status::Loaded;
    }

    void Entity::UnloadComponents( EntityModel::EntityLoadingContext const& loadingContext )
    {
        EE_ASSERT( m_status == Status::Loaded );

        for ( auto pComponent : m_components )
        {
            if ( pComponent->IsUnloaded() )
            {
                continue;
            }

            //-------------------------------------------------------------------------

            if ( pComponent->IsInitialized() )
            {
                pComponent->Shutdown();
                EE_ASSERT( !pComponent->IsInitialized() ); // Did you forget to call the parent class shutdown?
            }

            pComponent->Unload( loadingContext, Resource::ResourceRequesterID( m_ID.ToUint64() ) );
        }

        m_status = Status::Unloaded;
    }
    
    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void Entity::ComponentEditingDeactivate( EntityModel::ActivationContext& activationContext, ComponentID const& componentID )
    {
        auto pComponent = FindComponent( componentID );
        EE_ASSERT( pComponent != nullptr );

        if ( pComponent->m_isRegisteredWithWorld )
        {
            activationContext.m_componentsToUnregister.enqueue( TPair<Entity*, EntityComponent*>( this, pComponent ) );
        }

        if ( pComponent->m_isRegisteredWithEntity )
        {
            UnregisterComponentFromLocalSystems( pComponent );
        }
    }

    void Entity::ComponentEditingUnload( EntityModel::EntityLoadingContext const& loadingContext, ComponentID const& componentID )
    {
        auto pComponent = FindComponent( componentID );
        EE_ASSERT( pComponent != nullptr );

        if ( !pComponent->IsUnloaded() )
        {
            if ( pComponent->IsInitialized() )
            {
                pComponent->Shutdown();
                EE_ASSERT( !pComponent->IsInitialized() ); // Did you forget to call the parent class shutdown?
            }

            pComponent->Unload( loadingContext, Resource::ResourceRequesterID( m_ID.ToUint64() ) );
        }
    }

    void Entity::EndComponentEditing( EntityModel::EntityLoadingContext const& loadingContext )
    {
        for ( auto pComponent : m_components )
        {
            if ( pComponent->IsUnloaded() )
            {
                pComponent->Load( loadingContext, Resource::ResourceRequesterID( m_ID.ToUint64() ) );
            }
        }
    }

    StringID Entity::GenerateUniqueComponentName( EntityComponent* pComponent, StringID desiredNameID ) const
    {
        if ( !desiredNameID.IsValid() )
        {
            desiredNameID = pComponent->GetTypeID().ToStringID();
        }

        //-------------------------------------------------------------------------

        InlineString desiredName = desiredNameID.c_str();
        InlineString finalName = desiredName;

        uint32_t counter = 0;
        bool isUniqueName = false;
        while ( !isUniqueName )
        {
            isUniqueName = true;

            for ( auto pExistingComponent : m_components )
            {
                if ( pExistingComponent == pComponent )
                {
                    continue;
                }

                if ( finalName.comparei( pExistingComponent->GetName().c_str() ) == 0 )
                {
                    isUniqueName = false;
                    break;
                }
            }

            if ( !isUniqueName )
            {
                // Check if the last three characters are a numeric set, if so then increment the value and replace them
                if ( desiredName.length() > 3 && isdigit( desiredName[desiredName.length() - 1] ) && isdigit( desiredName[desiredName.length() - 2] ) && isdigit( desiredName[desiredName.length() - 3] ) )
                {
                    finalName.sprintf( "%s%03u", desiredName.substr( 0, desiredName.length() - 3 ).c_str(), counter );
                }
                else // Default name generation
                {
                    finalName.sprintf( "%s %03u", desiredName.c_str(), counter );
                }

                counter++;
            }
        }

        //-------------------------------------------------------------------------

        return StringID( finalName.c_str() );
    }

    void Entity::RenameComponent( EntityComponent* pComponent, StringID newNameID )
    {
        EE_ASSERT( pComponent != nullptr && pComponent->m_entityID == m_ID );
        pComponent->m_name = GenerateUniqueComponentName( pComponent, newNameID );
    }
    #endif

    //-------------------------------------------------------------------------

    void Entity::Activate( EntityModel::ActivationContext& activationContext )
    {
        EE_ASSERT( m_status == Status::Loaded );
        EE_ASSERT( m_updateRegistrationStatus == RegistrationStatus::Unregistered );

        // If we are attached to another entity we CANNOT be activated if our parent is not. This ensures that attachment chains have consistent activation state
        if ( HasSpatialParent() )
        {
            EE_ASSERT( m_pParentSpatialEntity->IsActivated() );
        }

        // Update internal status
        //-------------------------------------------------------------------------

        m_status = Status::Activated;

        // Initialize spatial hierarchy
        //-------------------------------------------------------------------------
        // Transforms are set at serialization time so we have all information available to update the world transforms

        if ( IsSpatialEntity() )
        {
            // Calculate the initial world transform but do not trigger the callback to the components
            m_pRootSpatialComponent->CalculateWorldTransform( false );
        }

        // Systems and Components
        //-------------------------------------------------------------------------

        for ( auto pSystem : m_systems )
        {
            pSystem->Initialize();
        }

        for ( auto pComponent : m_components )
        {
            if ( pComponent->IsInitialized() )
            {
                RegisterComponentWithLocalSystems( pComponent );
                activationContext.m_componentsToRegister.enqueue( TPair<Entity*, EntityComponent*>( this, pComponent ) );
            }
        }

        for ( auto pSystem : m_systems )
        {
            pSystem->Activate();
        }

        // Register for system updates
        if ( !m_systems.empty() )
        {
            GenerateSystemUpdateList();
            activationContext.m_registerForEntityUpdate.enqueue( this );
            m_updateRegistrationStatus = RegistrationStatus::QueuedForRegister;
        }

        // Spatial Attachments
        //-------------------------------------------------------------------------
        
        if ( IsSpatialEntity() )
        {
            if ( m_pParentSpatialEntity != nullptr )
            {
                CreateSpatialAttachment();
            }

            //-------------------------------------------------------------------------

            RefreshChildSpatialAttachments();
        }

        // Activate attached entities
        //-------------------------------------------------------------------------

        for ( auto pAttachedEntity : m_attachedEntities )
        {
            EE_ASSERT( !pAttachedEntity->IsActivated() );
            if ( pAttachedEntity->IsLoaded() )
            {
                pAttachedEntity->Activate( activationContext );
            }
        }
    }

    void Entity::Deactivate( EntityModel::ActivationContext& activationContext )
    {
        EE_ASSERT( m_status == Status::Activated );

        // If we are attached to another entity, that entity must also have been activated, else we have a problem in our attachment chains
        if ( HasSpatialParent() )
        {
            EE_ASSERT( m_pParentSpatialEntity->IsActivated() );
        }

        // Deactivate attached entities
        //-------------------------------------------------------------------------

        for ( auto pAttachedEntity : m_attachedEntities )
        {
            if ( pAttachedEntity->IsActivated() )
            {
                pAttachedEntity->Deactivate( activationContext );
            }
        }

        // Spatial Attachments
        //-------------------------------------------------------------------------

        if ( m_isSpatialAttachmentCreated )
        {
            DestroySpatialAttachment();
        }

        // Systems and Components
        //-------------------------------------------------------------------------

        // Unregister from system updates
        if ( !m_systems.empty() )
        {
            if ( m_updateRegistrationStatus == RegistrationStatus::Registered )
            {
                activationContext.m_unregisterForEntityUpdate.enqueue( this );
                m_updateRegistrationStatus = RegistrationStatus::QueuedForUnregister;
            }

            for ( int8_t i = 0; i < (int8_t) UpdateStage::NumStages; i++ )
            {
                m_systemUpdateLists[i].clear();
            }
        }

        for ( auto pSystem : m_systems )
        {
            pSystem->Deactivate();
        }

        for ( auto pComponent : m_components )
        {
            if ( pComponent->m_isRegisteredWithWorld )
            {
                activationContext.m_componentsToUnregister.enqueue( TPair<Entity*, EntityComponent*>( this, pComponent ) );
            }

            if ( pComponent->m_isRegisteredWithEntity )
            {
                UnregisterComponentFromLocalSystems( pComponent );
            }
        }

        for ( auto pSystem : m_systems )
        {
            pSystem->Shutdown();
        }

        //-------------------------------------------------------------------------

        m_status = Status::Loaded;
    }

    //-------------------------------------------------------------------------

    void Entity::UpdateSystems( EntityWorldUpdateContext const& context )
    {
        int8_t const updateStageIdx = (int8_t) context.GetUpdateStage();
        for( auto pSystem : m_systemUpdateLists[updateStageIdx] )
        {
            EE_ASSERT( pSystem->GetRequiredUpdatePriorities().IsStageEnabled( (UpdateStage) updateStageIdx ) );
            pSystem->Update( context );
        }
    }

    void Entity::RegisterComponentWithLocalSystems( EntityComponent* pComponent )
    {
        EE_ASSERT( pComponent != nullptr && pComponent->IsInitialized() && pComponent->m_entityID == m_ID && !pComponent->m_isRegisteredWithEntity );

        for ( auto pSystem : m_systems )
        {
            pSystem->RegisterComponent( pComponent );
            pComponent->m_isRegisteredWithEntity = true;
        }
    }

    void Entity::UnregisterComponentFromLocalSystems( EntityComponent* pComponent )
    {
        EE_ASSERT( pComponent != nullptr && pComponent->IsInitialized() && pComponent->m_entityID == m_ID && pComponent->m_isRegisteredWithEntity );

        for ( auto pSystem : m_systems )
        {
            pSystem->UnregisterComponent( pComponent );
            pComponent->m_isRegisteredWithEntity = false;
        }
    }

    //-------------------------------------------------------------------------

    SpatialEntityComponent* Entity::FindSocketAttachmentComponent( SpatialEntityComponent* pComponentToSearch, StringID socketID ) const
    {
        EE_ASSERT( pComponentToSearch != nullptr );

        if ( pComponentToSearch->HasSocket( socketID ) )
        {
            return pComponentToSearch;
        }

        for ( auto pChildComponent : pComponentToSearch->m_spatialChildren )
        {
            if ( auto pFoundComponent = FindSocketAttachmentComponent( pChildComponent, socketID ) )
            {
                return pFoundComponent;
            }
        }

        return nullptr;
    }

    void Entity::SetSpatialParent( Entity* pParentEntity, StringID attachmentSocketID )
    {
        EE_ASSERT( IsSpatialEntity() );
        EE_ASSERT( m_pParentSpatialEntity == nullptr && !m_isSpatialAttachmentCreated);
        EE_ASSERT( pParentEntity != nullptr && pParentEntity->IsSpatialEntity() );

        // Check for circular dependencies
        #if EE_DEVELOPMENT_TOOLS
        Entity const* pAncestorEntity = pParentEntity->GetSpatialParent();
        while ( pAncestorEntity != nullptr )
        {
            EE_ASSERT( pAncestorEntity != this );
            pAncestorEntity = pAncestorEntity->GetSpatialParent();
        }
        #endif

        // Set attachment data
        m_pParentSpatialEntity = pParentEntity;
        m_parentAttachmentSocketID = attachmentSocketID;

        // Add myself to the parent's list of attached entities
        {
            Threading::ScopeLock lock( pParentEntity->m_internalStateMutex );
            pParentEntity->m_attachedEntities.emplace_back( this );
        }

        s_entitySpatialAttachmentStateUpdatedEvent.Execute( this );
    }

    void Entity::ClearSpatialParent()
    {
        EE_ASSERT( IsSpatialEntity() );
        EE_ASSERT( HasSpatialParent() && !m_isSpatialAttachmentCreated );

        // Remove myself from parent's attached entity list
        auto iter = eastl::find( m_pParentSpatialEntity->m_attachedEntities.begin(), m_pParentSpatialEntity->m_attachedEntities.end(), this );
        EE_ASSERT( iter != m_pParentSpatialEntity->m_attachedEntities.end() );
        m_pParentSpatialEntity->m_attachedEntities.erase_unsorted( iter );

        // Clear attachment data
        m_parentAttachmentSocketID = StringID();
        m_pParentSpatialEntity = nullptr;
        s_entitySpatialAttachmentStateUpdatedEvent.Execute( this );
    }

    void Entity::CreateSpatialAttachment()
    {
        EE_ASSERT( IsSpatialEntity() );
        EE_ASSERT( m_pParentSpatialEntity != nullptr && !m_isSpatialAttachmentCreated );
        EE_ASSERT( IsActivated() && m_pParentSpatialEntity->IsActivated() );

        // Find component to attach to
        //-------------------------------------------------------------------------

        auto pParentEntity = m_pParentSpatialEntity;
        SpatialEntityComponent* pParentRootComponent = pParentEntity->m_pRootSpatialComponent;
        if ( m_parentAttachmentSocketID.IsValid() )
        {
            if ( auto pFoundComponent = pParentEntity->FindSocketAttachmentComponent( m_parentAttachmentSocketID ) )
            {
                pParentRootComponent = pFoundComponent;
            }
            else
            {
                EE_LOG_ENTITY_WARNING( this, "Entity", "Could not find attachment socket '%s' on entity '%s' (%u)", m_parentAttachmentSocketID.c_str(), pParentEntity->GetName().c_str(), pParentEntity->GetID() );
            }
        }

        // Perform attachment
        //-------------------------------------------------------------------------

        EE_ASSERT( pParentRootComponent != nullptr );

        // Set component hierarchy values
        m_pRootSpatialComponent->m_pSpatialParent = pParentRootComponent;
        m_pRootSpatialComponent->m_parentAttachmentSocketID = m_parentAttachmentSocketID;
        m_pRootSpatialComponent->CalculateWorldTransform();

        // Add to the list of child components on the component to attach to
        {
            Threading::ScopeLock lock( pParentEntity->m_internalStateMutex );
            pParentRootComponent->m_spatialChildren.emplace_back( m_pRootSpatialComponent );
        }

        //-------------------------------------------------------------------------

        m_isSpatialAttachmentCreated = true;
        s_entitySpatialAttachmentStateUpdatedEvent.Execute( this );
    }

    void Entity::DestroySpatialAttachment()
    {
        EE_ASSERT( IsSpatialEntity() );
        EE_ASSERT( IsActivated() );
        EE_ASSERT( m_pParentSpatialEntity != nullptr && m_isSpatialAttachmentCreated );

        // Remove from parent component child list
        //-------------------------------------------------------------------------

        auto pParentComponent = m_pRootSpatialComponent->m_pSpatialParent;
        auto foundIter = VectorFind( pParentComponent->m_spatialChildren, m_pRootSpatialComponent );
        EE_ASSERT( foundIter != pParentComponent->m_spatialChildren.end() );

        {
            Threading::ScopeLock lock( m_pParentSpatialEntity->m_internalStateMutex );
            pParentComponent->m_spatialChildren.erase_unsorted( foundIter );
        }

        // Remove component hierarchy values
        m_pRootSpatialComponent->m_pSpatialParent = nullptr;
        m_pRootSpatialComponent->m_parentAttachmentSocketID = StringID();

        //-------------------------------------------------------------------------

        m_isSpatialAttachmentCreated = false;
        s_entitySpatialAttachmentStateUpdatedEvent.Execute( this );
    }

    void Entity::RefreshChildSpatialAttachments()
    {
        EE_ASSERT( IsSpatialEntity() );
        for ( auto pAttachedEntity: m_attachedEntities )
        {
            // Only refresh active attachments
            if ( pAttachedEntity->m_isSpatialAttachmentCreated )
            {
                pAttachedEntity->DestroySpatialAttachment();
                pAttachedEntity->CreateSpatialAttachment();
            }
        }
    }

    void Entity::RemoveComponentFromSpatialHierarchy( SpatialEntityComponent* pSpatialComponent )
    {
        EE_ASSERT( pSpatialComponent != nullptr );

        if ( pSpatialComponent == m_pRootSpatialComponent )
        {
            int32_t const numChildrenForRoot = (int32_t) m_pRootSpatialComponent->m_spatialChildren.size();

            // Ensure that we break any spatial attachments before we mess with the root component
            bool const recreateSpatialAttachment = m_isSpatialAttachmentCreated;
            if ( HasSpatialParent() && m_isSpatialAttachmentCreated )
            {
                DestroySpatialAttachment();
            }

            //-------------------------------------------------------------------------

            // Removal of the root component if it has more than one child is not supported!
            if ( numChildrenForRoot > 1 )
            {
                EE_HALT();
            }
            else if ( numChildrenForRoot == 1 )
            {
                // Replace the root component of this entity with the child of the current root component that we are removing
                pSpatialComponent->m_spatialChildren[0]->m_pSpatialParent = nullptr;
                m_pRootSpatialComponent = pSpatialComponent->m_spatialChildren[0];
                pSpatialComponent->m_spatialChildren.clear();
            }
            else // No children, so completely remove the root component
            {
                m_pRootSpatialComponent = nullptr;

                // We are no longer a spatial entity so break any attachments
                if ( HasSpatialParent() )
                {
                    ClearSpatialParent();
                }
            }

            //-------------------------------------------------------------------------

            // Should we recreate the spatial attachment
            if ( HasSpatialParent() && recreateSpatialAttachment )
            {
                CreateSpatialAttachment();
            }
        }
        else // 'Fix' spatial hierarchy parenting
        {
            auto pParentComponent = pSpatialComponent->m_pSpatialParent;
            if ( pParentComponent != nullptr ) // Component may already have been removed from the hierarchy
            {
                // Remove from parent
                pParentComponent->m_spatialChildren.erase_first_unsorted( pSpatialComponent );

                // Reparent all children to parent component
                for ( auto pChildComponent : pSpatialComponent->m_spatialChildren )
                {
                    pChildComponent->m_pSpatialParent = pParentComponent;
                    pParentComponent->m_spatialChildren.emplace_back( pChildComponent );
                }
            }
        }
    }

    //-------------------------------------------------------------------------

    void Entity::GenerateSystemUpdateList()
    {
        for ( int8_t i = 0; i < (int8_t) UpdateStage::NumStages; i++ )
        {
            m_systemUpdateLists[i].clear();

            for ( auto& pSystem : m_systems )
            {
                if ( pSystem->GetRequiredUpdatePriorities().IsStageEnabled( (UpdateStage) i ) )
                {
                    m_systemUpdateLists[i].push_back( pSystem );
                }
            }

            // Sort update list
            auto comparator = [i] ( EntitySystem* const& pSystemA, EntitySystem* const& pSystemB )
            {
                uint8_t const A = pSystemA->GetRequiredUpdatePriorities().GetPriorityForStage( (UpdateStage) i );
                uint8_t const B = pSystemB->GetRequiredUpdatePriorities().GetPriorityForStage( (UpdateStage) i );
                return A > B;
            };

            eastl::sort( m_systemUpdateLists[i].begin(), m_systemUpdateLists[i].end(), comparator );
        }
    }

    void Entity::CreateSystemImmediate( TypeSystem::TypeInfo const* pSystemTypeInfo )
    {
        EE_ASSERT( pSystemTypeInfo != nullptr && pSystemTypeInfo->IsDerivedFrom<EntitySystem>() );

        #if EE_DEVELOPMENT_TOOLS
        // Ensure that we only allow a single system of a specific family
        for ( auto pExistingSystem : m_systems )
        {
            auto const pExistingSystemTypeInfo = pExistingSystem->GetTypeInfo();
            if ( pSystemTypeInfo->IsDerivedFrom( pExistingSystemTypeInfo->m_ID ) || pExistingSystemTypeInfo->IsDerivedFrom( pSystemTypeInfo->m_ID ) )
            {
                EE_HALT();
            }
        }
        #endif

        // Create the new system and add it
        auto pSystem = (EntitySystem*) pSystemTypeInfo->CreateType();
        m_systems.emplace_back( pSystem );

        if ( IsActivated() )
        {
            pSystem->Activate();
        }

        // Register all components with the new system
        for ( auto pComponent : m_components )
        {
            if ( pComponent->m_isRegisteredWithEntity )
            {
                pSystem->RegisterComponent( pComponent );
            }
        }
    }

    void Entity::CreateSystemDeferred( EntityModel::EntityLoadingContext const& loadingContext, TypeSystem::TypeInfo const* pSystemTypeInfo )
    {
        CreateSystemImmediate( pSystemTypeInfo );
        GenerateSystemUpdateList();
    }

    void Entity::CreateSystem( TypeSystem::TypeInfo const* pSystemTypeInfo )
    {
        EE_ASSERT( pSystemTypeInfo != nullptr );
        EE_ASSERT( pSystemTypeInfo->IsDerivedFrom( EntitySystem::GetStaticTypeID() ) );

        if ( IsUnloaded() )
        {
            CreateSystemImmediate( pSystemTypeInfo );
            s_entityUpdatedEvent.Execute( this );
        }
        else
        {
            Threading::ScopeLock lock( m_internalStateMutex );
            auto& action = m_deferredActions.emplace_back( EntityInternalStateAction() );
            action.m_type = EntityInternalStateAction::Type::CreateSystem;
            action.m_ptr = pSystemTypeInfo;
            s_entityInternalStateUpdatedEvent.Execute( this );
        }
    }

    void Entity::DestroySystem( TypeSystem::TypeInfo const* pSystemTypeInfo )
    {
        EE_ASSERT( pSystemTypeInfo != nullptr );
        EE_ASSERT( pSystemTypeInfo->IsDerivedFrom( EntitySystem::GetStaticTypeID() ) );

        if ( IsUnloaded() )
        {
            DestroySystemImmediate( pSystemTypeInfo );
            s_entityUpdatedEvent.Execute( this );
        }
        else
        {
            Threading::ScopeLock lock( m_internalStateMutex );
            auto& action = m_deferredActions.emplace_back( EntityInternalStateAction() );
            action.m_type = EntityInternalStateAction::Type::DestroySystem;
            action.m_ptr = pSystemTypeInfo;
            s_entityInternalStateUpdatedEvent.Execute( this );
        }
    }

    void Entity::DestroySystem( TypeSystem::TypeID systemTypeID )
    {
        TypeSystem::TypeInfo const* pSystemTypeInfo = nullptr;
        for ( auto pSystem : m_systems )
        {
            if ( pSystem->GetTypeID() == systemTypeID )
            {
                pSystemTypeInfo = pSystem->GetTypeInfo();
                break;
            }
        }

        DestroySystem( pSystemTypeInfo );
    }

    void Entity::DestroySystemImmediate( TypeSystem::TypeInfo const* pSystemTypeInfo )
    {
        EE_ASSERT( pSystemTypeInfo != nullptr && pSystemTypeInfo->IsDerivedFrom<EntitySystem>() );

        int32_t const systemIdx = VectorFindIndex( m_systems, pSystemTypeInfo->m_ID, [] ( EntitySystem* pSystem, TypeSystem::TypeID systemTypeID ) { return pSystem->GetTypeInfo()->m_ID == systemTypeID; } );
        EE_ASSERT( systemIdx != InvalidIndex );
        auto pSystem = m_systems[systemIdx];

        // Unregister all components from the system to remove
        for ( auto pComponent : m_components )
        {
            if ( pComponent->m_isRegisteredWithEntity )
            {
                pSystem->UnregisterComponent( pComponent );
            }
        }

        if ( IsActivated() )
        {
            pSystem->Deactivate();
        }

        // Destroy the system
        EE::Delete( pSystem );
        m_systems.erase_unsorted( m_systems.begin() + systemIdx );
    }

    void Entity::DestroySystemDeferred( EntityModel::EntityLoadingContext const& loadingContext, TypeSystem::TypeInfo const* pSystemTypeInfo )
    {
        DestroySystemImmediate( pSystemTypeInfo );
        GenerateSystemUpdateList();
    }
    
    //-------------------------------------------------------------------------

    void Entity::CreateComponent( TypeSystem::TypeInfo const* pComponentTypeInfo, ComponentID const& parentSpatialComponentID )
    {
        EE_ASSERT( pComponentTypeInfo != nullptr && pComponentTypeInfo->IsDerivedFrom<EntityComponent>() );
        EntityComponent* pComponent = Cast<EntityComponent>( pComponentTypeInfo->CreateType() );

        #if EE_DEVELOPMENT_TOOLS
        pComponent->m_name = StringID( pComponentTypeInfo->GetFriendlyTypeName() );
        #else
        pComponent->m_name = StringID( pComponentTypeInfo->GetTypeName() );
        #endif

        AddComponent( pComponent, parentSpatialComponentID );
    }

    void Entity::AddComponent( EntityComponent* pComponent, ComponentID const& parentSpatialComponentID )
    {
        EE_ASSERT( pComponent != nullptr && pComponent->GetID().IsValid() );
        EE_ASSERT( !pComponent->m_entityID.IsValid() && pComponent->IsUnloaded() );
        EE_ASSERT( !VectorContains( m_components, (EntityComponent*) pComponent ) );

        //-------------------------------------------------------------------------

        SpatialEntityComponent* pSpatialComponent = TryCast<SpatialEntityComponent>( pComponent );
        
        // Parent ID can only be set when adding a spatial component
        if( pSpatialComponent == nullptr )
        {
            EE_ASSERT( !parentSpatialComponentID.IsValid() );
        }

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        if ( pComponent->IsSingletonComponent() )
        {
            // Ensure we have no components that are derived from or are a parent of the component
            for ( auto pExistingComponent : m_components )
            {
                EE_ASSERT( !pComponent->GetTypeInfo()->IsDerivedFrom( pExistingComponent->GetTypeID() ) );
                EE_ASSERT( !pExistingComponent->GetTypeInfo()->IsDerivedFrom( pComponent->GetTypeID() ) );
            }
        }

        // Ensure unique name
        //-------------------------------------------------------------------------

        pComponent->m_name = GenerateUniqueComponentName( pComponent, pComponent->m_name );

        #endif

        //-------------------------------------------------------------------------

        if ( IsUnloaded() )
        {
            SpatialEntityComponent* pParentComponent = nullptr;
            if ( parentSpatialComponentID.IsValid() )
            {
                EE_ASSERT( pSpatialComponent != nullptr );
                int32_t const componentIdx = VectorFindIndex( m_components, parentSpatialComponentID, [] ( EntityComponent* pComponent, ComponentID const& componentID ) { return pComponent->GetID() == componentID; } );
                EE_ASSERT( componentIdx != InvalidIndex );

                pParentComponent = TryCast<SpatialEntityComponent>( m_components[componentIdx] );
                EE_ASSERT( pParentComponent != nullptr );
            }

            AddComponentImmediate( pComponent, pParentComponent );
            s_entityUpdatedEvent.Execute( this );
        }
        else // Defer the operation to the next loading phase
        {
            Threading::ScopeLock lock( m_internalStateMutex );

            EntityInternalStateAction& action = m_deferredActions.emplace_back( EntityInternalStateAction() );
            action.m_type = EntityInternalStateAction::Type::AddComponent;
            action.m_ptr = pComponent;
            action.m_parentComponentID = parentSpatialComponentID;
            s_entityInternalStateUpdatedEvent.Execute( this );
        }
    }

    void Entity::DestroyComponent( ComponentID const& componentID )
    {
        int32_t const componentIdx = VectorFindIndex( m_components, componentID, [] ( EntityComponent* pComponent, ComponentID const& componentID ) { return pComponent->GetID() == componentID; } );

        // If you hit this assert either the component doesnt exist or this entity has still to process deferred actions and the component might be in that list
        // We dont support adding and destroying a component in the same frame, please avoid doing stupid things
        EE_ASSERT( componentIdx != InvalidIndex );
        auto pComponent = m_components[componentIdx];

        //-------------------------------------------------------------------------

        if ( IsUnloaded() )
        {
            // Root removal validation
            if ( pComponent == m_pRootSpatialComponent )
            {
                // You are only allowed to remove the root component if it has a single child or no children. Otherwise we cant fix the spatial hierarchy for this entity.
                EE_ASSERT( m_pRootSpatialComponent->m_spatialChildren.size() <= 1 );
            }

            DestroyComponentImmediate( pComponent );
            s_entityUpdatedEvent.Execute( this );
        }
        else // Defer the operation to the next loading phase
        {
            Threading::ScopeLock lock( m_internalStateMutex );

            auto& action = m_deferredActions.emplace_back( EntityInternalStateAction() );
            action.m_type = EntityInternalStateAction::Type::DestroyComponent;
            action.m_ptr = pComponent;
            EE_ASSERT( action.m_ptr != nullptr );
            s_entityInternalStateUpdatedEvent.Execute( this );
        }
    }

    void Entity::AddComponentImmediate( EntityComponent* pComponent, SpatialEntityComponent* pParentSpatialComponent )
    {
        EE_ASSERT( pComponent != nullptr && pComponent->GetID().IsValid() && !pComponent->m_entityID.IsValid() );
        EE_ASSERT( pComponent->IsUnloaded() && !pComponent->m_isRegisteredWithWorld && !pComponent->m_isRegisteredWithEntity );

        // Update spatial hierarchy
        //-------------------------------------------------------------------------

        SpatialEntityComponent* pSpatialEntityComponent = TryCast<SpatialEntityComponent>( pComponent );
        if ( pSpatialEntityComponent != nullptr )
        {
            // If the parent component is null, attach it to the root by default
            if ( pParentSpatialComponent == nullptr )
            {
                pParentSpatialComponent = m_pRootSpatialComponent;
            }

            // Update hierarchy and transform
            if ( pParentSpatialComponent != nullptr )
            {
                pSpatialEntityComponent->m_pSpatialParent = pParentSpatialComponent;
                pParentSpatialComponent->m_spatialChildren.emplace_back( pSpatialEntityComponent );
                pSpatialEntityComponent->CalculateWorldTransform( false );
            }
            else // Make this component the root component
            {
                m_pRootSpatialComponent = pSpatialEntityComponent;
                EE_ASSERT( pSpatialEntityComponent->m_pSpatialParent == nullptr );
            }
        }

        // Add component
        //-------------------------------------------------------------------------

        m_components.emplace_back( pComponent );
        pComponent->m_entityID = m_ID;
    }

    void Entity::AddComponentDeferred( EntityModel::EntityLoadingContext const& loadingContext, EntityComponent* pComponent, SpatialEntityComponent* pParentSpatialComponent )
    {
        EE_ASSERT( loadingContext.IsValid() );
        AddComponentImmediate( pComponent, pParentSpatialComponent );
        pComponent->Load( loadingContext, Resource::ResourceRequesterID( m_ID.ToUint64() ) );
    }

    void Entity::DestroyComponentImmediate( EntityComponent* pComponent )
    {
        EE_ASSERT( pComponent != nullptr );
        EE_ASSERT( pComponent->IsUnloaded() && !pComponent->m_isRegisteredWithWorld && !pComponent->m_isRegisteredWithEntity );

        int32_t const componentIdx = VectorFindIndex( m_components, pComponent );
        EE_ASSERT( componentIdx != InvalidIndex );

        // Update spatial hierarchy
        //-------------------------------------------------------------------------

        SpatialEntityComponent* pSpatialComponent = TryCast<SpatialEntityComponent>( pComponent );
        if ( pSpatialComponent != nullptr )
        {
            RemoveComponentFromSpatialHierarchy( pSpatialComponent );
        }

        // Remove component
        //-------------------------------------------------------------------------

        m_components.erase_unsorted( m_components.begin() + componentIdx );
        EE::Delete( pComponent );
    }

    void Entity::DestroyComponentDeferred( EntityModel::EntityLoadingContext const& loadingContext, EntityComponent* pComponent )
    {
        EE_ASSERT( loadingContext.IsValid() );

        // Shutdown and unload component
        //-------------------------------------------------------------------------

        if ( pComponent->IsInitialized() )
        {
            pComponent->Shutdown();
        }

        pComponent->Unload( loadingContext, Resource::ResourceRequesterID( m_ID.ToUint64() ) );

        // Destroy the component
        //-------------------------------------------------------------------------

        DestroyComponentImmediate( pComponent );
    }

    //-------------------------------------------------------------------------

    bool Entity::UpdateEntityState( EntityModel::EntityLoadingContext const& loadingContext, EntityModel::ActivationContext& activationContext )
    {
        Threading::ScopeLock lock( m_internalStateMutex );

        //-------------------------------------------------------------------------
        // Deferred Actions
        //-------------------------------------------------------------------------
        // Note: ORDER OF OPERATIONS MATTERS!

        bool const entityStateChanged = !m_deferredActions.empty();
        for ( int32_t i = 0; i < (int32_t) m_deferredActions.size(); i++ )
        {
            EntityInternalStateAction& action = m_deferredActions[i];

            switch ( action.m_type )
            {
                case EntityInternalStateAction::Type::CreateSystem:
                {
                    CreateSystemDeferred( loadingContext, (TypeSystem::TypeInfo const*) action.m_ptr );
                    m_deferredActions.erase( m_deferredActions.begin() + i );
                    i--;
                }
                break;

                case EntityInternalStateAction::Type::DestroySystem:
                {
                    DestroySystemDeferred( loadingContext, (TypeSystem::TypeInfo const*) action.m_ptr );
                    m_deferredActions.erase( m_deferredActions.begin() + i );
                    i--;
                }
                break;

                case EntityInternalStateAction::Type::AddComponent:
                {
                    SpatialEntityComponent* pParentComponent = nullptr;

                    // Try to resolve the ID
                    if ( action.m_parentComponentID.IsValid() )
                    {
                        int32_t const componentIdx = VectorFindIndex( m_components, action.m_parentComponentID, [] ( EntityComponent* pComponent, ComponentID const& componentID ) { return pComponent->GetID() == componentID; } );
                        EE_ASSERT( componentIdx != InvalidIndex );

                        pParentComponent = TryCast<SpatialEntityComponent>( m_components[componentIdx] );
                        EE_ASSERT( pParentComponent != nullptr );
                    }

                    AddComponentDeferred( loadingContext, (EntityComponent*) action.m_ptr, pParentComponent );
                    m_deferredActions.erase( m_deferredActions.begin() + i );
                    i--;
                }
                break;

                case EntityInternalStateAction::Type::DestroyComponent:
                {
                    auto pComponentToDestroy = (EntityComponent*) action.m_ptr;

                    // Remove spatial components from the hierarchy immediately
                    SpatialEntityComponent* pSpatialComponent = TryCast<SpatialEntityComponent>( pComponentToDestroy );
                    if ( pSpatialComponent != nullptr )
                    {
                        RemoveComponentFromSpatialHierarchy( pSpatialComponent );
                    }

                    // Unregister from local systems
                    if ( pComponentToDestroy->m_isRegisteredWithEntity )
                    {
                        UnregisterComponentFromLocalSystems( pComponentToDestroy );
                    }

                    // Request unregister from global systems
                    if ( pComponentToDestroy->m_isRegisteredWithWorld )
                    {
                        activationContext.m_componentsToUnregister.enqueue( TPair<Entity*, EntityComponent*>( this, pComponentToDestroy ) );
                        action.m_type = EntityInternalStateAction::Type::WaitForComponentUnregistration;
                    }
                }
                // No Break: Intentional Fall-through here

                case EntityInternalStateAction::Type::WaitForComponentUnregistration:
                {
                    auto pComponentToDestroy = (EntityComponent*) action.m_ptr;

                    // We can destroy the component safely
                    if ( !pComponentToDestroy->m_isRegisteredWithEntity && !pComponentToDestroy->m_isRegisteredWithWorld )
                    {
                        DestroyComponentDeferred( loadingContext, (EntityComponent*) action.m_ptr );
                        m_deferredActions.erase( m_deferredActions.begin() + i );
                        i--;
                    }
                }
                break;
            }
        }

        if ( entityStateChanged )
        {
            s_entityUpdatedEvent.Execute( this );
        }

        //-------------------------------------------------------------------------
        // Component Loading
        //-------------------------------------------------------------------------

        for ( auto pComponent : m_components )
        {
            if ( pComponent->IsLoading() )
            {
                pComponent->UpdateLoading();

                // If we are still loading, return
                if ( pComponent->IsLoading() )
                {
                    return false;
                }
            }

            // Once we finish loading a component immediately initialize it
            if ( pComponent->IsLoaded() )
            {
                pComponent->Initialize();
                EE_ASSERT( pComponent->IsInitialized() ); // Did you forget to call the parent class initialize?

                if ( auto pSpatialComponent = TryCast<SpatialEntityComponent>( pComponent ) )
                {
                    pSpatialComponent->CalculateWorldTransform( false );
                }

                // If we are already activated, then register with entity systems
                if ( IsActivated() )
                {
                    RegisterComponentWithLocalSystems( pComponent );
                    activationContext.m_componentsToRegister.enqueue( TPair<Entity*, EntityComponent*>( this, pComponent ) );
                }
            }
        }

        //-------------------------------------------------------------------------
        // Entity update registration
        //-------------------------------------------------------------------------

        EE_ASSERT( m_updateRegistrationStatus != RegistrationStatus::QueuedForRegister && m_updateRegistrationStatus != RegistrationStatus::QueuedForUnregister );

        bool const shouldBeRegisteredForUpdates = IsActivated() && !m_systems.empty() && !HasSpatialParent();

        if ( shouldBeRegisteredForUpdates )
        {
            if ( m_updateRegistrationStatus != RegistrationStatus::Registered )
            {
                activationContext.m_registerForEntityUpdate.enqueue( this );
                m_updateRegistrationStatus = RegistrationStatus::QueuedForRegister;
            }
        }
        else // Doesnt require an update
        {
            if ( m_updateRegistrationStatus == RegistrationStatus::Registered )
            {
                activationContext.m_unregisterForEntityUpdate.enqueue( this );
                m_updateRegistrationStatus = RegistrationStatus::QueuedForUnregister;
            }
        }

        //-------------------------------------------------------------------------

        return m_deferredActions.empty();
    }
}