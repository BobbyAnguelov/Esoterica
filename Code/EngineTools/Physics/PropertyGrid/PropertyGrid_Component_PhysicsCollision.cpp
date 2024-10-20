#include "EngineTools/PropertyGrid/PropertyGridTypeEditingRules.h"
#include "Engine/Physics/Components/Component_PhysicsCollisionMesh.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class CollisionComponentEditingRules : public PG::TTypeEditingRules<CollisionMeshComponent>
    {
        using PG::TTypeEditingRules<CollisionMeshComponent>::TTypeEditingRules;

        virtual HiddenState IsHidden( StringID const& propertyID ) override
        {
            StringID const collisionSettingsPropertyID( "m_collisionSettings" );
            if ( propertyID == collisionSettingsPropertyID )
            {
                if ( !m_pTypeInstance->IsOverridingCollisionSettings() )
                {
                    return HiddenState::Hidden;
                }
            }

            return HiddenState::Unhandled;
        }
    };

    EE_PROPERTY_GRID_EDITING_RULES( BodySettingsHelperFactory, CollisionMeshComponent, CollisionComponentEditingRules );
}