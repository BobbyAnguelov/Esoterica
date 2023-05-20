#include "Component_PhysicsCollisionMesh.h"
#include "Engine/Entity/EntityLog.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    OBB CollisionMeshComponent::CalculateLocalBounds() const
    {
        OBB meshBounds( Vector::Zero, Vector::Half );

        if ( HasValidPhysicsSetup() )
        {
            meshBounds = m_collisionMesh->GetBounds();
            meshBounds.m_center *= m_localScale;
            meshBounds.m_extents *= m_localScale;
        }

        return meshBounds;
    }

    bool CollisionMeshComponent::HasValidPhysicsSetup() const
    {
        if ( !m_collisionMesh.IsSet() )
        {
            return false;
        }

        if ( !m_collisionMesh.IsLoaded() )
        {
            return false;
        }

        if ( !m_collisionMesh->IsValid() )
        {
            return false;
        }

        return true;
    }

    CollisionSettings const& CollisionMeshComponent::GetCollisionSettings() const
    {
        EE_ASSERT( m_collisionMesh.IsLoaded() );
        return m_overrideCollisionSettings ? m_collisionSettings : m_collisionMesh->GetCollisionSettings();
    }

    void CollisionMeshComponent::SetCollisionSettings( CollisionSettings const& newSettings )
    {
        m_overrideCollisionSettings = true;
        PhysicsShapeComponent::SetCollisionSettings( newSettings );
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void CollisionMeshComponent::PostPropertyEdit( TypeSystem::PropertyInfo const* pPropertyEdited )
    {
        if ( Math::IsNearZero( m_localScale.m_x ) ) m_localScale.m_x = 0.1f;
        if ( Math::IsNearZero( m_localScale.m_y ) ) m_localScale.m_y = 0.1f;
        if ( Math::IsNearZero( m_localScale.m_z ) ) m_localScale.m_z = 0.1f;

        PhysicsShapeComponent::PostPropertyEdit( pPropertyEdited );
    }
    #endif
}