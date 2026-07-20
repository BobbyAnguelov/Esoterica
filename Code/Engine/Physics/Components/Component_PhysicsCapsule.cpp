#include "Component_PhysicsCapsule.h"
#include "Engine/Entity/EntityLog.h"
#include "Engine/Physics/PhysicsWorld.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    OBB CapsuleComponent::CalculateLocalBounds() const
    {
        Vector const boundsExtents( m_radius, m_radius, m_radius + m_halfHeight );
        return OBB( Vector::Origin, boundsExtents );
    }

    bool CapsuleComponent::HasValidPhysicsSetup() const
    {
        if ( m_radius <= 0 || m_halfHeight <= 0 )
        {
            EE_LOG_ENTITY_ERROR( this, "Physics", "Invalid radius or half height on Physics Capsule Component: %s (%u). Negative or zero values are not allowed!", GetNameID().c_str(), GetID() );
            return false;
        }

        return true;
    }

    void CapsuleComponent::Initialize()
    {
        m_radius = m_defaultRadius;
        m_halfHeight = m_defaultHalfHeight;

        PhysicsShapeComponent::Initialize();
    }

    void CapsuleComponent::CreatePhysicsShape()
    {
        EE_ASSERT( B3_IS_NON_NULL( m_physicsBodyID ) );
        EE_ASSERT( B3_IS_NULL( m_physicsShapeID ) );

        Transform const& worldTransform = GetWorldTransform();
        float const scale = Math::Abs( worldTransform.GetScale() );

        b3SurfaceMaterial material = m_pPhysicsWorld->GetMaterial( m_materialID );

        b3ShapeDef shapeDef = b3DefaultShapeDef();
        shapeDef.density = m_defaultDensity;
        shapeDef.userData = &m_userData;
        shapeDef.filter = ToBox3D( GetCollisionSettings() );
        shapeDef.materials = &material;
        shapeDef.materialCount = 1;

        float const scaledRadius = m_radius * scale;
        float const scaledHalfHeight = m_halfHeight * scale;
        b3Capsule capsule = { { 0.0f, 0.0f, -scaledHalfHeight }, { 0.0f, 0.0f, scaledHalfHeight }, scaledRadius };
        m_physicsShapeID = b3CreateCapsuleShape( m_physicsBodyID, &shapeDef, &capsule );
    }

    void CapsuleComponent::ResizeCapsule( float newRadius, float newHalfHeight, bool keepFloorPosition )
    {
        EE_ASSERT( newRadius > 0.0f );
        EE_ASSERT( newHalfHeight >= 0.0f );

        Transform const& worldTransform = GetWorldTransform();
        float const scale = Math::Abs( worldTransform.GetScale() );

        float const heightDelta = ( newHalfHeight - m_halfHeight ) * scale;
        m_radius = newRadius;
        m_halfHeight = newHalfHeight;

        //-------------------------------------------------------------------------

        ApplyOffsetToAllChildren( Vector( 0, 0, -heightDelta ) );

        //-------------------------------------------------------------------------

        m_pPhysicsWorld->LockWrite();
        {
            float const scaledRadius = m_radius * scale;
            float const scaledHalfHeight = m_halfHeight * scale;
            b3Capsule const capsule = { { 0.0f, 0.0f, -scaledHalfHeight }, { 0.0f, 0.0f, scaledHalfHeight }, scaledRadius };
            b3Shape_SetCapsule( m_physicsShapeID, &capsule );

            // If we are keeping the floor position, we need to shift the capsule down
            if ( keepFloorPosition )
            {
                Transform shiftedWorldTransform = GetWorldTransform();
                shiftedWorldTransform.AddTranslation( shiftedWorldTransform.RotateVector( Vector( 0, 0, heightDelta ) ) );
                SetWorldTransform( shiftedWorldTransform );
            }
        }
        m_pPhysicsWorld->UnlockWrite();
    }
}