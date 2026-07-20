#include "PlayerOverlayAction_Weapon.h"
#include "Game/Player/Animation/PlayerAnimationController.h"
#include "Game/Player/Components/Component_PlayerTargeting.h"
#include "Game/Damage/Components/Component_Damage.h"

//-------------------------------------------------------------------------

namespace EE
{
    PlayerOverlayAction_Weapon::PlayerOverlayAction_Weapon()
    {
        char const * const curveStr = "4,0.000000,1.000000,1.000000,0.029443,0,2.500000,0.900000,-0.276809,-0.276809,1,6.000000,0.350778,-0.955276,-0.681616,0,8.000000,0.100000,-0.014375,1.000000,0";
        m_velocityAccuracyCurve.FromString( curveStr );
    }

    bool PlayerOverlayAction_Weapon::TryStartInternal( PlayerActionContext const& ctx )
    {
        auto pTargetingComponent = ctx.GetComponentByType<PlayerTargetingComponent>();
        if ( pTargetingComponent == nullptr )
        {
            return false;
        }

        return true;
    }

    void PlayerOverlayAction_Weapon::StopInternal( PlayerActionContext const& ctx, StopReason reason )
    {
        ctx.m_pAnimationController->HolsterWeapon();
    }

    PlayerAction::Status PlayerOverlayAction_Weapon::UpdateInternal( PlayerActionContext const& ctx, bool isFirstUpdate )
    {
        auto pTargetingComponent = ctx.GetComponentByType<PlayerTargetingComponent>();
        EE_ASSERT( pTargetingComponent != nullptr );

        // Holster
        //-------------------------------------------------------------------------

        bool const isWeaponDrawn = ctx.m_pAnimationController->IsWeaponDrawn();
        if ( !isWeaponDrawn )
        {
            ctx.m_pAnimationController->DrawWeapon();
        }

        // Lock On
        //-------------------------------------------------------------------------

        if ( ctx.m_pInput->m_lockTarget.WasPressed() )
        {
            pTargetingComponent->LockOn();
        }

        if ( pTargetingComponent->IsLockedOn() && !ctx.m_pInput->m_lockTarget.IsHeld() )
        {
            pTargetingComponent->CancelLockOn();
        }

        //-------------------------------------------------------------------------

        if ( !isWeaponDrawn )
        {
            return Status::Interruptible;
        }

        // Accuracy
        //-------------------------------------------------------------------------

        Vector const characterVelocity = ctx.m_pPlayer->GetVelocity();
        float const speed = characterVelocity.GetLength3();
        m_movingAccuracy = Math::Clamp( m_velocityAccuracyCurve.Evaluate( speed ), 0.0f, 1.0f );
        m_shootingAccuracy = Math::Min( m_shootingAccuracy + ( m_shootingInaccuracyRecoveryRate * ctx.GetDeltaTime() ), 1.0f );

        // Aim Target
        //-------------------------------------------------------------------------

        Vector aimTargetWS;

        if ( pTargetingComponent->HasTrackedTarget() )
        {
            aimTargetWS = pTargetingComponent->GetTrackedTargetBounds().GetCenter();
        }
        else
        {
            aimTargetWS = ctx.m_pPlayer->GetPosition() + ( ctx.m_pPlayer->GetForwardVector() * 10.0f );
        }

        ctx.m_pAnimationController->AimWeapon( aimTargetWS );

        // Firing
        //-------------------------------------------------------------------------

        if ( m_isAutomatic )
        {
            if ( ctx.m_pInput->m_shoot.IsHeld() )
            {

            }
        }
        else
        {
            if ( ctx.m_pInput->m_shoot.WasPressed() )
            {
                FireWeapon( ctx );
            }
        }

        //-------------------------------------------------------------------------

        return Status::Interruptible;
    }

    void PlayerOverlayAction_Weapon::FireWeapon( PlayerActionContext const& ctx )
    {
        Vector start = ctx.m_pPlayer->GetAttachmentSocketTransform( StringID( "muzzle" ) ).GetTranslation();
        Vector end;

        auto pTargetingComponent = ctx.GetComponentByType<PlayerTargetingComponent>();
        if ( pTargetingComponent->HasTrackedTarget() )
        {
            end = pTargetingComponent->GetTrackedTargetBounds().GetCenter();

            Vector dir;
            float length;
            ( end - start ).ToDirectionAndLength3( dir, length );
            end = start + ( dir * Math::Max( 250.0f, length ) );
        }
        else
        {
            end = ctx.m_pPlayer->GetPosition() + ( ctx.m_pPlayer->GetForwardVector() * 100.0f );
        }

        auto pDamageComponent = ctx.GetComponentByType<DamageComponent>();
        pDamageComponent->RequestDamage( start, end, { 10 } );

        ctx.m_pAnimationController->FireWeapon();

        m_shootingAccuracy = Math::Max( m_shootingAccuracy - m_inaccuracyPenaltyPerShot, 0.0f ); 

        #if EE_DEVELOPMENT_TOOLS
        auto drawCtx = ctx.m_pEntityWorldUpdateContext->GetDebugDrawContext();
        drawCtx.DrawLine( start, end, Colors::Red, 1, DebugDrawLayer::World, 0.5f );
        #endif
    }
}