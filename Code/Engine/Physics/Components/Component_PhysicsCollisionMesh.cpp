#include "Component_PhysicsCollisionMesh.h"
#include "Engine/Entity/EntityLog.h"
#include "Engine/Physics/PhysicsWorld.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    OBB CollisionMeshComponent::CalculateLocalBounds() const
    {
        OBB meshBounds( Vector::Zero, Vector::Half );

        if ( HasValidPhysicsSetup() )
        {
            meshBounds = OBB( m_collisionMesh->GetBounds() );
            meshBounds.m_center *= GetWorldNonUniformScale();
            meshBounds.m_extents *= GetWorldNonUniformScale();
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

    void CollisionMeshComponent::CreatePhysicsShape()
    {
        EE_ASSERT( B3_IS_NON_NULL( m_physicsBodyID ) );
        EE_ASSERT( B3_IS_NULL( m_physicsShapeID ) );

        EE_ASSERT( m_collisionMesh->IsValid() );
        if ( m_collisionMesh->GetPhysicsMaterials().empty() )
        {
            EE_LOG_ENTITY_ERROR( this, "Physics", "No physics materials set for collision mesh (%s) on component %s (%u). No shapes will be created!", m_collisionMesh.GetResourceID().c_str(), this->GetNameID().c_str(), this->GetID() );
            return;
        }

        TInlineVector<b3SurfaceMaterial, 10> materials;
        for ( auto const& materialID : m_collisionMesh->GetPhysicsMaterials() )
        {
            materials.emplace_back( m_pPhysicsWorld->GetMaterial( materialID ) );
        }

        //-------------------------------------------------------------------------

        b3ShapeDef shapeDef = b3DefaultShapeDef();
        shapeDef.density = m_defaultDensity;
        shapeDef.userData = &m_userData;
        shapeDef.filter = ToBox3D( GetCollisionSettings() );
        shapeDef.materials = materials.data();
        shapeDef.materialCount = (int32_t) materials.size();

        Transform const& worldTransform = GetWorldTransform();
        float const scale = worldTransform.GetScale();
        Vector const finalScale = GetWorldNonUniformScale() * scale;

        // Triangle Mesh
        if ( m_collisionMesh->IsTriangleMesh() )
        {
            m_physicsShapeID = b3CreateMeshShape( m_physicsBodyID, &shapeDef, m_collisionMesh->GetMeshData(), ToBox3D( finalScale ) );
        }
        else // Convex Hull
        {
            m_physicsShapeID = b3CreateTransformedHullShape( m_physicsBodyID, &shapeDef, m_collisionMesh->GetHullData(), b3Transform_identity, ToBox3D( finalScale ) );
        }
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

    void CollisionMeshComponent::OnNonUniformScaleChanged()
    {
        GetRebuildBodySignal()->Send( this );
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void CollisionMeshComponent::PostPropertyEdit( TypeSystem::PropertyInfo const* pPropertyEdited )
    {
        if ( Math::IsNearZero( m_nonUniformScale.m_x ) ) m_nonUniformScale.m_x = 0.001f;
        if ( Math::IsNearZero( m_nonUniformScale.m_y ) ) m_nonUniformScale.m_y = 0.001f;
        if ( Math::IsNearZero( m_nonUniformScale.m_z ) ) m_nonUniformScale.m_z = 0.001f;

        PhysicsShapeComponent::PostPropertyEdit( pPropertyEdited );
    }
    #endif
}