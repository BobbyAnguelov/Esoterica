#include "Component_PhysicsBox.h"
#include "Engine/Entity/EntityLog.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    OBB BoxComponent::CalculateLocalBounds() const
    {
        return OBB( Vector::Origin, m_boxExtents );
    }

    // This constructor only exists to lazy initialize the static default material ID
    BoxComponent::BoxComponent()
    {
        static StringID const defaultMaterialID( PhysicsMaterial::DefaultID );
        m_materialID = defaultMaterialID;
    }

    bool BoxComponent::HasValidPhysicsSetup() const
    {
        if ( m_boxExtents.m_x <= 0.0f || m_boxExtents.m_y <= 0.0f || m_boxExtents.m_z <= 0.0f )
        {
            EE_LOG_ENTITY_ERROR( this, "Physics", "Invalid box extents on Physics Box Component: %s (%u). Negative or zero values are not allowed!", GetNameID().c_str(), GetID() );
            return false;
        }

        if ( !m_materialID.IsValid() )
        {
            EE_LOG_ENTITY_ERROR( this, "Physics", "Invalid physical material setup on Physics Component: %s (%u)", GetNameID().c_str(), GetID() );
            return false;
        }

        return true;
    }
}