#include "BehaviorAction_Idle.h"
#include "Base/Math/MathRandom.h"

//-------------------------------------------------------------------------

namespace EE
{
    void BehaviorAction_Idle::Start( BehaviorContext const& ctx )
    {
        //ctx.m_pAnimationController->SetCharacterState( CharacterAnimationState::Locomotion );
        //ctx.m_pAnimationController->SetIdle();

        ResetIdleBreakerTimer();
    }

    void BehaviorAction_Idle::ResetIdleBreakerTimer()
    {
        m_idleBreakerCooldown.Start( Math::GetRandomFloat( 15.0f, 25.0f ) );
    }

    BehaviorAction::Status BehaviorAction_Idle::Update( BehaviorContext const& ctx )
    {
        if ( m_idleBreakerCooldown.IsRunning() )
        {
            if ( m_idleBreakerCooldown.Update( ctx.GetDeltaTime() ) )
            {
                // TODO: run idle breaker
            }
        }

        return Status::Running;
    }
}
