#include "PlayerActionContext.h"
#include "Base/Drawing/DebugDrawing.h"

//-------------------------------------------------------------------------

namespace EE
{
    PlayerActionContext::~PlayerActionContext()
    {
        EE_ASSERT( m_pEntityWorldUpdateContext == nullptr && m_pPhysicsWorld == nullptr );
        EE_ASSERT( m_pPlayer == nullptr && m_pAnimationController == nullptr && m_pCamera == nullptr );
    }

    bool PlayerActionContext::IsValid() const
    {
        if ( m_pPlayer == nullptr || !m_pPlayer->IsRootComponent() )
        {
            return false;
        }

        if ( m_pPlayerState == nullptr )
        {
            return false;
        }

        if ( m_pAnimationController == nullptr )
        {
            return false;
        }

        return m_pEntityWorldUpdateContext != nullptr && m_pCamera != nullptr && m_pPhysicsWorld != nullptr && m_pInput != nullptr;
    }

    #if EE_DEVELOPMENT_TOOLS
    DebugDrawContext PlayerActionContext::GetDebugDrawContext() const
    {
        return m_pEntityWorldUpdateContext->GetDebugDrawContext();
    }
    #endif
}