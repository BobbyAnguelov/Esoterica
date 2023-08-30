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
            StringID const collisionSettingsPropertyID( "m_collisionSettings" );
            if ( propertyID == collisionSettingsPropertyID )
            {
                return !m_pTypeInstance->IsOverridingCollisionSettings();
            }

            return false;
        }
    };

    EE_PROPERTY_GRID_EDITING_RULES( BodySettingsHelperFactory, CollisionMeshComponent, CollisionComponentEditingRules );
}