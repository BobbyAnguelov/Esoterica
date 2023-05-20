#include "Component_PhysicsBox.h"
#include "Engine/Entity/EntityLog.h"

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
}