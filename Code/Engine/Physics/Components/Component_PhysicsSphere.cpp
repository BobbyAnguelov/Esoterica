#include "Component_PhysicsSphere.h"
#include "Engine/Entity/EntityLog.h"
#include "Engine/Physics/PhysicsWorld.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    OBB SphereComponent::CalculateLocalBounds() const
    {
        return OBB( Vector::Origin, Vector( m_radius ) );
    }

    bool SphereComponent::HasValidPhysicsSetup() const
    {
        if ( m_radius <= 0.0f )
        {
            EE_LOG_ENTITY_ERROR( this, "Physics", "Invalid radius on Physics Sphere Component: %s (%u)", GetNameID().c_str(), GetID() );
            return false;
        }

        return true;
    }

    void SphereComponent::CreatePhysicsShape()
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

        b3Sphere const sphere = { b3Vec3_zero, m_radius * scale };
        m_physicsShapeID = b3CreateSphereShape( m_physicsBodyID, &shapeDef, &sphere );
    }
}