#include "PlayerAction.h"
#include "Game/Player/Components/Component_MainPlayer.h"
#include "Game/Player/Animation/PlayerAnimationController.h"
#include "Engine/Physics/Components/Component_PhysicsCharacter.h"
#include "Base/Drawing/DebugDrawing.h"

//-------------------------------------------------------------------------

namespace EE::Player
{
    ActionContext::~ActionContext()
    {
        EE_ASSERT( m_pEntityWorldUpdateContext == nullptr && m_pInputSystem == nullptr && m_pPhysicsWorld == nullptr );
        EE_ASSERT( m_pCharacterComponent == nullptr );
        EE_ASSERT( m_pPlayerComponent == nullptr && m_pAnimationController == nullptr && m_pCameraController == nullptr );
    }

    bool ActionContext::IsValid() const
    {
        if ( m_pPlayerComponent == nullptr )
        {
            return false;
        }

        if ( m_pCharacterComponent == nullptr || !m_pCharacterComponent->IsRootComponent() || !m_pCharacterComponent->IsControllerCreated() )
        {
            return false;
        }


        if ( m_pAnimationController == nullptr )
        {
            return false;
        }

        return m_pEntityWorldUpdateContext != nullptr && m_pCameraController != nullptr && m_pInputSystem != nullptr && m_pPhysicsWorld != nullptr;
    }

    #if EE_DEVELOPMENT_TOOLS
    Drawing::DrawContext ActionContext::GetDrawingContext() const
    {
        return m_pEntityWorldUpdateContext->GetDrawingContext();
    }
    #endif
}