#pragma once
#include "Base/Math/Quaternion.h"
#include "Base/Time/Time.h"

//-------------------------------------------------------------------------

namespace EE { class EntityWorldUpdateContext; }

namespace EE::Physics
{
    class CharacterComponent;
    class PhysicsWorld;
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

        bool TryMoveCapsule( EntityWorldUpdateContext const& ctx, Physics::PhysicsWorld* pPhysicsWorld, Vector const& deltaTranslation, Quaternion const& deltaRotation );

    public:

        Physics::CharacterComponent*        m_pCharacterComponent = nullptr;
    };
}