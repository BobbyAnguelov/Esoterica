#include "Component_PhysicsCapsule.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
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
            EE_LOG_ERROR( "Physics", "Invalid radius or half height on Physics Capsule Component: %s (%u). Negative or zero values are not allowed!", GetName().c_str(), GetID() );
            return false;
        }

        if ( !m_materialID.IsValid() )
        {
            EE_LOG_ERROR( "Physics", "Invalid physical material setup on Physics Component: %s (%u)", GetName().c_str() );
            return false;
        }

        return true;
    }
}