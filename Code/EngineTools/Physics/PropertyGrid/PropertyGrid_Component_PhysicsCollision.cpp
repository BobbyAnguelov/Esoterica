#include "EngineTools/Core/PropertyGrid/PropertyGridHelper.h"
#include "Engine/Physics/Components/Component_PhysicsCollisionMesh.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class CollisionComponentPropertyHelper : public PG::TPropertyHelper<CollisionMeshComponent>
    {
        using PG::TPropertyHelper<CollisionMeshComponent>::TPropertyHelper;

        virtual bool IsReadOnly( StringID const& propertyID ) override
        {
            return false;
        }

        virtual bool IsHidden( StringID const& propertyID ) override
        {
            if ( propertyID == EE_REFLECT_GET_PROPERTY_ID( CollisionMeshComponent, m_collisionSettings ) )
            {
                return !m_pTypeInstance->IsOverridingCollisionSettings();
            }

            return false;
        }
    };

    EE_PROPERTY_GRID_HELPER( BodySettingsHelperFactory, CollisionMeshComponent, CollisionComponentPropertyHelper );
}