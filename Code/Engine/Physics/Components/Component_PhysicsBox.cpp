#include "Component_PhysicsBox.h"
#include "Engine/Entity/EntityLog.h"
#include "Engine/Physics/PhysicsWorld.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    OBB BoxComponent::CalculateLocalBounds() const
    {
        return OBB( Vector::Origin, m_boxHalfExtents );
    }

    bool BoxComponent::HasValidPhysicsSetup() const
    {
        if ( m_boxHalfExtents.m_x <= 0.0f || m_boxHalfExtents.m_y <= 0.0f || m_boxHalfExtents.m_z <= 0.0f )
        {
            EE_LOG_ENTITY_ERROR( this, "Physics", "Invalid box extents on Physics Box Component: %s (%u). Negative or zero values are not allowed!", GetNameID().c_str(), GetID() );
            return false;
        }

        return true;
    }

    void BoxComponent::CreatePhysicsShape()
    {
        EE_ASSERT( B3_IS_NON_NULL( m_physicsBodyID ) );
        EE_ASSERT( B3_IS_NULL( m_physicsShapeID ) );

        Transform const& worldTransform = GetWorldTransform();
        float const scale = worldTransform.GetScale();

        b3SurfaceMaterial material = m_pPhysicsWorld->GetMaterial( m_materialID );

        b3ShapeDef shapeDef = b3DefaultShapeDef();
        shapeDef.density = m_defaultDensity;
        shapeDef.userData = &m_userData;
        shapeDef.filter = ToBox3D( GetCollisionSettings() );
        shapeDef.materials = &material;
        shapeDef.materialCount = 1;

        Vector const scaledExtents = m_boxHalfExtents * scale;
        b3BoxHull box = b3MakeBoxHull( scaledExtents.GetX(), scaledExtents.GetY(), scaledExtents.GetZ() );
        m_physicsShapeID = b3CreateHullShape( m_physicsBodyID, &shapeDef, &box.base );
    }
}