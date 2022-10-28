#include "Component_PhysicsCapsule.h"
#include "Engine/Entity/EntityLog.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    OBB CapsuleComponent::CalculateLocalBounds() const
    {
        Vector const boundsExtents( m_cylinderPortionHalfHeight + m_radius, m_radius, m_radius );
        return OBB( Vector::Origin, boundsExtents );
    }

    // This constructor only exists to lazy initialize the static default material ID
    CapsuleComponent::CapsuleComponent()
    {
        static StringID const defaultMaterialID( PhysicsMaterial::DefaultID );
        m_materialID = defaultMaterialID;

        static Transform const rotatedUpright( Quaternion( Vector::UnitY, Radians( Math::PiDivTwo ) ) );
        SetLocalTransform( rotatedUpright );
    }

    bool CapsuleComponent::HasValidPhysicsSetup() const
    {
        if ( m_radius <= 0 || m_cylinderPortionHalfHeight <= 0 )
        {
            EE_LOG_ENTITY_ERROR( this, "Physics", "Invalid radius or half height on Physics Capsule Component: %s (%u). Negative or zero values are not allowed!", GetNameID().c_str(), GetID() );
            return false;
        }

        if ( !m_materialID.IsValid() )
        {
            EE_LOG_ENTITY_ERROR( this, "Physics", "Invalid physical material setup on Physics Component: %s (%u)", GetNameID().c_str() );
            return false;
        }

        return true;
    }
}