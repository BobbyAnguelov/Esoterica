#include "PlayerAction_Interact.h"
#include "Game/Player/Animation/PlayerAnimationController.h"
#include "Base/Input/InputSystem.h"

//-------------------------------------------------------------------------

namespace EE
{
    class ExternalController : public Animation::ExternalGraphController
    {
    public:

        using Animation::ExternalGraphController::ExternalGraphController;

        #if EE_DEVELOPMENT_TOOLS
        virtual char const* GetName() const override
        {
            return "Temp";
        }
        #endif

        virtual void PostGraphUpdate( Seconds deltaTime ) override
        {
            Animation::ExternalGraphController::PostGraphUpdate( deltaTime );

            static StringID const completionEventID( "InteractionComplete" );
            m_isComplete = GetSampledEvents().ContainsGraphEvent( completionEventID );
        }

    public:

        bool m_isComplete = false;
    };

    //-------------------------------------------------------------------------

    bool PlayerAction_Interact::TryStartInternal( PlayerActionContext const& ctx )
    {
        m_canInteract = false;

        /*if ( ctx.m_pCharacter->m_pAvailableInteraction == nullptr )
        {
            return false;
        }*/

        //// If we have an valid interactible, show the prompt
        //// show prompt
        //m_canInteract = true;

        //// Check for input
        //if( ctx.m_pInput->m_interact.WasPressed() )
        //{
        //    // Create external controller
        //    m_pController = ctx.m_pAnimationController->TryCreateExternalGraphController<ExternalController>( StringID( "Interaction" ), ctx.m_pPlayerComponent->m_pAvailableInteraction, true );
        //    if ( m_pController != nullptr )
        //    {
        //        ctx.m_pAnimationController->SetCharacterState( PlayerAnimationController::CharacterState::Interaction );
        //        return true;
        //    }
        //}

        return false;
    }

    PlayerAction::Status PlayerAction_Interact::UpdateInternal( PlayerActionContext const& ctx, bool isFirstUpdate )
    {
        EE_ASSERT( m_pController != nullptr );

        // Wait for complete signal
        if ( m_pController->m_isComplete )
        {
            return Status::Completed;
        }
        else
        {
            return Status::Interruptible;
        }
    }

    void PlayerAction_Interact::StopInternal( PlayerActionContext const& ctx, StopReason reason )
    {
        //ctx.m_pAnimationController->DestroyExternalGraphController( StringID( "Interaction" ) );
        m_pController = nullptr;
    }
}