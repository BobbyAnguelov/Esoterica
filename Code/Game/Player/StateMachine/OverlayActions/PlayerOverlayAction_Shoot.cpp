#include "PlayerOverlayAction_Shoot.h"
//-------------------------------------------------------------------------
#include "Engine/Physics/Components/Component_PhysicsCharacter.h"
#include "Game/Player/Camera/PlayerCameraController.h"
#include "Game/Weapon/Ammo.h"
#include "Game/Weapon/BaseWeapon.h"
#include "Base/Input/InputSystem.h"
//-------------------------------------------------------------------------

namespace EE::Player
{
    bool ShootOverlayAction::TryStartInternal( ActionContext const& ctx )
    {
        if ( ctx.m_pInputState->GetControllerState()->GetRightTriggerValue() >= 0.2f )
        {
            auto const& camFwd  = ctx.m_pCameraController->GetCameraRelativeForwardVector();
            Vector const origin = ctx.m_pCharacterComponent->GetPosition();
            Vector const target = origin + camFwd*500.f;
            // hard code a weapon here?
            EE::Weapon::BaseWeapon* weaponTest = EE::New<EE::Weapon::BaseWeapon>();
            weaponTest->m_pCurrentAmmo = EE::New<EE::Weapon::Ammo>();

            EE::Weapon::AmmoContext ammoCtx;
            ammoCtx.m_pEntityWorldUpdateContext = ctx.m_pEntityWorldUpdateContext;
            ammoCtx.m_pPhysicsWorld = ctx.m_pPhysicsWorld;
            weaponTest->Shoot(ammoCtx, origin, target);

            EE::Delete(weaponTest->m_pCurrentAmmo);
            EE::Delete(weaponTest);
            return true;
        }

        return false;
    }

    Action::Status ShootOverlayAction::UpdateInternal( ActionContext const& ctx )
    {
        if ( ctx.m_pInputState->GetControllerState()->GetRightTriggerValue() < 0.2f )
        {
            return Status::Completed;
        }

        return Status::Interruptible;
    }

    void ShootOverlayAction::StopInternal( ActionContext const& ctx, StopReason reason )
    {

    }
}