#pragma once
#include "System/Math/Quaternion.h"
#include "System/Time/Time.h"

//-------------------------------------------------------------------------

namespace EE { class EntityWorldUpdateContext; }

namespace EE::Physics
{
    class CharacterComponent;
    class Scene;
}

//-------------------------------------------------------------------------

namespace EE::AI
{
    class CharacterPhysicsController final
    {
    public:

        CharacterPhysicsController( Physics::CharacterComponent* pCharacterComponent )
            : m_pCharacterComponent( pCharacterComponent )
        {
            EE_ASSERT( m_pCharacterComponent != nullptr );
        }

        bool TryMoveCapsule( EntityWorldUpdateContext const& ctx, Physics::Scene* pPhysicsScene, Vector const& deltaTranslation, Quaternion const& deltaRotation );

    public:

        Physics::CharacterComponent*        m_pCharacterComponent = nullptr;
    };
}