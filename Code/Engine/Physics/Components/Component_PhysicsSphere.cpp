#include "Component_PhysicsSphere.h"
#include "Engine/Entity/EntityLog.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    // This constructor only exists to lazy initialize the static default material ID
    SphereComponent::SphereComponent()
    {
        static StringID const defaultMaterialID( PhysicsMaterial::DefaultID );
        m_materialID = defaultMaterialID;
    }

    bool SphereComponent::HasValidPhysicsSetup() const
    {
        if ( m_radius <= 0.0f )
        {
            EE_LOG_ENTITY_ERROR( this, "Physics", "Invalid radius on Physics Sphere Component: %s (%u)", GetNameID().c_str(), GetID() );
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