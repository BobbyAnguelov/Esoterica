#include "AIBehaviorSelector.h"
#include "System/Log.h"
#include "Game/AI/Behaviors/AIBehavior_Wander.h"
#include "Game/AI/Behaviors/AIBehavior_CombatPositioning.h"

//-------------------------------------------------------------------------

namespace EE::AI
{
    BehaviorSelector::BehaviorSelector( BehaviorContext const& context )
        : m_actionContext( context )
    {
        m_behaviors.emplace_back( EE::New<WanderBehavior>() );
        m_behaviors.emplace_back( EE::New<CombatPositionBehavior>() );
    }

    BehaviorSelector::~BehaviorSelector()
    {
        for ( auto& pBehavior : m_behaviors )
        {
            EE::Delete( pBehavior );
        }
        m_behaviors.clear();
    }

    //-------------------------------------------------------------------------

    void BehaviorSelector::Update()
    {
        EE_ASSERT( m_actionContext.IsValid() );

        //-------------------------------------------------------------------------

        // HACK HACK HACK
        if ( m_pActiveBehavior == nullptr )
        {
            m_pActiveBehavior = m_behaviors.back();
            m_pActiveBehavior->Start( m_actionContext );
        }
        // HACK HACK HACK

        //-------------------------------------------------------------------------

        if ( m_pActiveBehavior != nullptr )
        {
            m_pActiveBehavior->Update( m_actionContext );
        }
    }
}