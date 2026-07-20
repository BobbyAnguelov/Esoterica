#include "BehaviorSelector.h"

#include "Behaviors/Behavior_Death.h"
#include "Behaviors/Behavior_Wander.h"
#include "Behaviors/Behavior_HackMove.h"
#include "Game/Damage/Components/Component_Health.h"

//-------------------------------------------------------------------------

namespace EE
{
    BehaviorSelector::BehaviorSelector( BehaviorContext const& context )
        : m_context( context )
    {
        m_behaviors.emplace_back( EE::New<DeathBehavior>() );
        m_behaviors.emplace_back( EE::New<HackPrototypeBehavior>() );
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
        EE_ASSERT( m_context.IsValid() );

        //-------------------------------------------------------------------------

        // HACK HACK HACK
        if ( m_pActiveBehavior == nullptr )
        {
            m_pActiveBehavior = m_behaviors.back();
            m_pActiveBehavior->Start( m_context );
        }

        if ( m_pActiveBehavior != nullptr )
        {
            auto pHealthComponent = m_context.GetComponentByType<HealthComponent>();
            if ( pHealthComponent != nullptr && pHealthComponent->GetHP() == 0 && m_pActiveBehavior != m_behaviors.front() )
            {
                m_pActiveBehavior->Stop( m_context, Behavior::StopReason::Interrupted );
                m_pActiveBehavior = m_behaviors.front();
                m_pActiveBehavior->Start( m_context );
            }
        }
        // HACK HACK HACK

        //-------------------------------------------------------------------------

        if ( m_pActiveBehavior != nullptr )
        {
            m_pActiveBehavior->Update( m_context );
        }
    }
}