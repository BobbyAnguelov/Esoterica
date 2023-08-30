#include "PlayerOverlayAction_Aim.h"
#include "Engine/Physics/Components/Component_PhysicsCharacter.h"
#include "Game/Player/Animation/PlayerAnimationController.h"
#include "Game/Player/Animation/PlayerGraphController_Weapon.h"
#include "Game/Player/Camera/PlayerCameraController.h"
#include "Base/Input/InputSystem.h"

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
        auto pWeaponController = ctx.GetAnimSubGraphController<WeaponGraphController>();

        if ( !pWeaponController->IsWeaponDrawn() )
        {
            pWeaponController->DrawWeapon();
        }
        else
        {
            Vector const aimTargetWS = ctx.m_pCameraController->GetCameraPosition() + ctx.m_pCameraController->GetCameraRelativeForwardVector() * 10.0f;
            pWeaponController->Aim( aimTargetWS );
        }

        //-------------------------------------------------------------------------

        if ( ctx.m_pInputState->GetControllerState()->GetLeftTriggerValue() < 0.2f )
        {
            return Status::Completed;
        }

        return Status::Interruptible;
    }

    void AimOverlayAction::StopInternal( ActionContext const& ctx, StopReason reason )
    {
        auto pWeaponController = ctx.GetAnimSubGraphController<WeaponGraphController>();
        pWeaponController->HolsterWeapon();
    }
}