#include "BehaviorAction_Death.h"

//-------------------------------------------------------------------------

namespace EE
{
    BehaviorAction::Status BehaviorAction_Death::Update( BehaviorContext const& ctx )
    {
        ctx.m_pAnimationController->SetCharacterState( CharacterAnimationState::Death );
        return Status::Running;
    }
}
