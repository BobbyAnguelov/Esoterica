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

    bool SpatialEntityComponent::TryGetAttachmentSocketTransform( StringID socketID, Transform& outTransform ) const
    {
        // If the socket ID is invalid, just return the current transform
        if ( !socketID.IsValid() )
        {
            outTransform = m_worldTransform;
            return false;
        }

        //-------------------------------------------------------------------------

        // Try to find the attachment socket transform and if it succeeds return it
        if ( GetAttachmentSocketTransformInternal( socketID, outTransform ) )
        {
            return true;
        }

        //-------------------------------------------------------------------------

        // Search all children
        for ( auto pChildSpatialComponent : m_spatialChildren )
        {
            auto foundTransform = pChildSpatialComponent->TryGetAttachmentSocketTransform( socketID, outTransform );
            if ( foundTransform )
            {
                return true;
            }
        }

        // Log warning only when we are loaded/initialized
        if ( m_status == EntityComponent::Status::Loaded || m_status == EntityComponent::Status::Initialized )
        {
            EE_LOG_ENTITY_WARNING( this, "Entity", "Failed to find socket %s on component %s (%u)", socketID.c_str(), GetNameID().c_str(), GetID() );
        }

        // Fallback to the world transform
        outTransform = m_worldTransform;
        return false;
    }

    void SpatialEntityComponent::ApplyOffsetToAllChildren( Vector const& offset )
    {
        for ( auto pChildSpatialComponent : m_spatialChildren )
        {
            Transform adjustedLocalTransform = pChildSpatialComponent->GetLocalTransform();
            adjustedLocalTransform.AddTranslation( offset );
            pChildSpatialComponent->SetLocalTransform( adjustedLocalTransform );
        }
    }

    bool SpatialEntityComponent::GetAttachmentSocketTransformInternal( StringID socketID, Transform& outSocketWorldTransform ) const
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

        if ( SupportsNonUniformScale() )
        {
            OnNonUniformScaleChanged();
        }
    }
    #endif
}