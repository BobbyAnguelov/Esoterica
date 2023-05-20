#include "Component_PhysicsCapsule.h"
#include "Engine/Entity/EntityLog.h"
#include "Engine/Physics/Physics.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    OBB CapsuleComponent::CalculateLocalBounds() const
    {
        Vector const boundsExtents( m_radius, m_radius, m_radius + m_cylinderPortionHalfHeight );
        return OBB( Vector::Origin, boundsExtents );
    }

    bool CapsuleComponent::HasValidPhysicsSetup() const
    {
        if ( m_radius <= 0 || m_cylinderPortionHalfHeight <= 0 )
        {
            EE_LOG_ENTITY_ERROR( this, "Physics", "Invalid radius or half height on Physics Capsule Component: %s (%u). Negative or zero values are not allowed!", GetNameID().c_str(), GetID() );
            return false;
        }

        return true;
    }

    Transform CapsuleComponent::ConvertTransformToPhysics( Transform const& esotericaTransform ) const
    {
        Transform const physicsTransform( PX::Conversion::s_capsuleConversionToPx * esotericaTransform.GetRotation(), esotericaTransform.GetTranslation() );
        return physicsTransform;
    }
}