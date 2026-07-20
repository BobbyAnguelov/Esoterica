#include "BehaviorContext.h"

//-------------------------------------------------------------------------

namespace EE
{
    BehaviorContext::~BehaviorContext()
    {
        EE_ASSERT( m_pEntityWorldUpdateContext == nullptr && m_pNavmeshSystem == nullptr && m_pPhysicsWorld == nullptr );
        EE_ASSERT( m_pNPC == nullptr && m_pAnimationController == nullptr );
    }

    bool BehaviorContext::IsValid() const
    {
        if ( m_pNPC == nullptr || !m_pNPC->IsRootComponent() /*|| !m_pCharacter->HasCharacterController()*/ )
        {
            return false;
        }

        if ( m_pNPCState == nullptr )
        {
            return false;
        }

        if ( m_pAnimationController == nullptr )
        {
            return false;
        }

        return m_pEntityWorldUpdateContext != nullptr && m_pNavmeshSystem != nullptr && m_pPhysicsWorld != nullptr;
    }

    #if EE_DEVELOPMENT_TOOLS
    DebugDrawContext BehaviorContext::GetDebugDrawContext() const
    {
        return m_pEntityWorldUpdateContext->GetDebugDrawContext();
    }
    #endif
}