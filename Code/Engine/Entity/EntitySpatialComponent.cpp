#include "EntitySpatialComponent.h"
#include "EntityLog.h"

//-------------------------------------------------------------------------

namespace EE
{
    int32_t SpatialEntityComponent::GetSpatialHierarchyDepth( bool limitToCurrentEntity ) const
    {
        int32_t hierarchyDepth = 0;

        if ( limitToCurrentEntity && IsRootComponent() )
        {
            return hierarchyDepth;
        }

        //-------------------------------------------------------------------------

        SpatialEntityComponent const* pComponent = this;
        while ( pComponent->HasSpatialParent() )
        {
            if ( limitToCurrentEntity && pComponent->GetEntityID() != GetEntityID() )
            {
                break;
            }

            //-------------------------------------------------------------------------

            hierarchyDepth++;
            pComponent = pComponent->m_pSpatialParent;
        }

        return hierarchyDepth;
    }

    bool SpatialEntityComponent::IsSpatialChildOf( SpatialEntityComponent const* pPotentialParent ) const
    {
        EE_ASSERT( pPotentialParent != nullptr );
        EE_ASSERT( pPotentialParent->m_entityID == m_entityID );

        auto pActualParent = m_pSpatialParent;
        while ( pActualParent != nullptr )
        {
            if ( pActualParent == pPotentialParent )
            {
                return true;
            }

            pActualParent = pActualParent->m_pSpatialParent;
        }

        return false;
    }

    Transform SpatialEntityComponent::GetAttachmentSocketTransform( StringID socketID ) const
    {
        Transform socketTransform;

        // If the socket ID is invalid, just return the current transform
        if ( !socketID.IsValid() )
        {
            socketTransform = m_worldTransform;
            return socketTransform;
        }

        //-------------------------------------------------------------------------

        // Try to find the attachment socket transform and if it succeeds return it
        if ( TryFindAttachmentSocketTransform( socketID, socketTransform ) )
        {
            return socketTransform;
        }

        //-------------------------------------------------------------------------

        // Search all children
        for ( auto pChildSpatialComponent : m_spatialChildren )
        {
            auto foundTransform = pChildSpatialComponent->TryGetAttachmentSocketTransform( socketID, socketTransform );
            if ( foundTransform )
            {
                return socketTransform;
            }
        }

        // Log warning only when we are loaded/initialized
        if ( m_status == EntityComponent::Status::Loaded || m_status == EntityComponent::Status::Initialized )
        {
            EE_LOG_ENTITY_WARNING( this, "Entity", "Failed to find socket %s on component %s (%u)", socketID.c_str(), GetNameID().c_str(), GetID() );
        }

        // Fallback to the world transform
        socketTransform = m_worldTransform;
        return socketTransform;
    }

    bool SpatialEntityComponent::TryGetAttachmentSocketTransform( StringID socketID, Transform& outSocketWorldTransform ) const
    {
        // Try to find the attachment socket transform and if it succeeds return it
        if ( TryFindAttachmentSocketTransform( socketID, outSocketWorldTransform ) )
        {
            return true;
        }

        // Search all children
        for ( auto pChildSpatialComponent : m_spatialChildren )
        {
            auto foundTransform = pChildSpatialComponent->TryGetAttachmentSocketTransform( socketID, outSocketWorldTransform );
            if ( foundTransform )
            {
                return true;
            }
        }

        return false;
    }

    bool SpatialEntityComponent::TryFindAttachmentSocketTransform( StringID socketID, Transform& outSocketWorldTransform ) const
    {
        outSocketWorldTransform = m_worldTransform;
        return false;
    }

    void SpatialEntityComponent::NotifySocketsUpdated()
    {
        for ( auto& pChildComponent : m_spatialChildren )
        {
            pChildComponent->CalculateWorldTransform();
        }
    }

    void SpatialEntityComponent::Initialize()
    {
        EntityComponent::Initialize();
        UpdateBounds();
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void SpatialEntityComponent::PostPropertyEdit( TypeSystem::PropertyInfo const* pPropertyEdited )
    {
        EntityComponent::PostPropertyEdit( pPropertyEdited );

        // Property edits always refresh the transform since properties could have an effect on bounds/transform
        CalculateWorldTransform();
    }
    #endif
}