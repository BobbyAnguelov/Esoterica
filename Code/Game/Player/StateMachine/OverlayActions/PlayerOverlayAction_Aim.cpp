#include "PlayerOverlayAction_Aim.h"
#include "Engine/Physics/Components/Component_PhysicsCharacter.h"
#include "Game/Player/Animation/PlayerAnimationController.h"
#include "Game/Player/Camera/PlayerCameraController.h"
#include "Base/Input/InputSystem.h"

//-------------------------------------------------------------------------

namespace EE::Player
{
    bool AimOverlayAction::TryStartInternal( ActionContext const& ctx )
    {
        if ( ctx.m_pInput->m_aim.IsHeld() )
        {
            return true;
        }
        
        return false;
    }

    Action::Status AimOverlayAction::UpdateInternal( ActionContext const& ctx, bool isFirstUpdate )
    {
        if ( !ctx.m_pAnimationController->IsWeaponDrawn() )
        {
            ctx.m_pAnimationController->DrawWeapon();
        }
        else
        {
            Vector const aimTargetWS = ctx.m_pCameraController->GetCameraPosition() + ctx.m_pCameraController->GetCameraRelativeForwardVector() * 10.0f;
            ctx.m_pAnimationController->AimWeapon( aimTargetWS );
        }

        //-------------------------------------------------------------------------

        if ( ctx.m_pInput->m_aim.WasReleased() )
        {
            return Status::Completed;
        }

        return Status::Interruptible;
    }

    void AimOverlayAction::StopInternal( ActionContext const& ctx, StopReason reason )
    {
        ctx.m_pAnimationController->HolsterWeapon();
    }
}