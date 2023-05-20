#include "Component_PhysicsSphere.h"
#include "Engine/Entity/EntityLog.h"

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
}