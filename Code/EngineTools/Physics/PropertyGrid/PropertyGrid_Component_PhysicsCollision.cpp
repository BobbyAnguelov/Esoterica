#include "EngineTools/Core/PropertyGrid/PropertyGridTypeEditingRules.h"
#include "Engine/Physics/Components/Component_PhysicsCollisionMesh.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class CollisionComponentEditingRules : public PG::TTypeEditingRules<CollisionMeshComponent>
    {
        using PG::TTypeEditingRules<CollisionMeshComponent>::TTypeEditingRules;

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

    EE_PROPERTY_GRID_EDITING_RULES( BodySettingsHelperFactory, CollisionMeshComponent, CollisionComponentEditingRules );
}