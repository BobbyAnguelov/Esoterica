#include "PlayerOverlayAction_Aim.h"
#include "Engine/Physics/Components/Component_PhysicsCharacter.h"
#include "Game/Player/Camera/PlayerCameraController.h"
#include "Base/Input/InputSystem.h"
#include "../../Animation/PlayerAnimationController.h"

//-------------------------------------------------------------------------

namespace EE::Player
{
    bool AimOverlayAction::TryStartInternal( ActionContext const& ctx )
    {
        if ( ctx.m_pInputState->GetControllerState()->GetLeftTriggerValue() >= 0.2f )
        {
            return true;
        }

        return false;
    }

    Action::Status AimOverlayAction::UpdateInternal( ActionContext const& ctx )
    {
        ctx.m_pAnimationController->SetWeaponAim( true );

        if ( ctx.m_pInputState->GetControllerState()->GetLeftTriggerValue() < 0.2f )
        {
            return Status::Completed;
        }

        return Status::Interruptible;
    }

    void AimOverlayAction::StopInternal( ActionContext const& ctx, StopReason reason )
    {
        ctx.m_pAnimationController->SetWeaponAim( false );
    }
}