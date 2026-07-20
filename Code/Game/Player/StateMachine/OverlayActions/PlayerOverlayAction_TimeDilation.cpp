#include "PlayerOverlayAction_TimeDilation.h"
#include "Game/Player/Components/Component_PlayerCamera.h"

//-------------------------------------------------------------------------

namespace EE
{
    bool PlayerOverlayAction_TimeDilation::TryStartInternal( PlayerActionContext const& ctx )
    {
        if ( ctx.m_pPlayerState->GetEnergy() <= 0.0f )
        {
            return false;
        }

        return ctx.m_pInput->m_timeDilation.IsHeld();
    }

    PlayerAction::Status PlayerOverlayAction_TimeDilation::UpdateInternal( PlayerActionContext const& ctx, bool isFirstUpdate )
    {
        if ( !ctx.m_pInput->m_timeDilation.IsHeld() || ctx.m_pPlayerState->GetEnergy() == 0.0f )
        {
            return Status::Completed;
        }

        ctx.m_pEntityWorldUpdateContext->SetTimeScale( 0.35f );
        ctx.m_pPlayerState->ConsumeEnergy( ctx.GetRawDeltaTime() );
        ctx.m_pCamera->SetOrbitDistance( 3.0f );
        ctx.m_pPlayerState->m_isInTimeDilation = true;

        return Status::Interruptible;
    }

    void PlayerOverlayAction_TimeDilation::StopInternal( PlayerActionContext const& ctx, StopReason reason )
    {
        ctx.m_pEntityWorldUpdateContext->SetTimeScale( 1.0f );
        ctx.m_pCamera->ResetOrbitTargetOffset();
        ctx.m_pPlayerState->m_isInTimeDilation = false;
    }
}