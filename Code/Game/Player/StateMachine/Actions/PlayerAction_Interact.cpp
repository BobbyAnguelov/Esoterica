#include "PlayerAction_Interact.h"
#include "Game/Player/Components/Component_MainPlayer.h"
#include "Game/Player/Animation/PlayerAnimationController.h"
#include "System/Input/InputSystem.h"

//-------------------------------------------------------------------------

namespace EE::Player
{
    class ExternalController : public Animation::ExternalGraphController
    {
    public:

        using Animation::ExternalGraphController::ExternalGraphController;

        virtual char const* GetName() const override
        {
            return "Temp";
        }

        virtual void PostGraphUpdate( Seconds deltaTime ) override
        {
            Animation::ExternalGraphController::PostGraphUpdate( deltaTime );

            auto const& sampledEvents = GetSampledEvents();
            for ( Animation::SampledEvent const& sampledEvent : sampledEvents )
            {
                static StringID const completionEventID( "InteractionComplete" );
                if ( sampledEvent.IsStateEvent() && sampledEvent.GetStateEventID() == completionEventID )
                {
                    m_isComplete = true;
                    break;
                }
            }
        }

    public:

        bool m_isComplete = false;
    };

    //-------------------------------------------------------------------------

    bool InteractAction::TryStartInternal( ActionContext const& ctx )
    {
        m_canInteract = false;

        if ( ctx.m_pPlayerComponent->m_pAvailableInteraction == nullptr )
        {
            return false;
        }

        // If we have an valid interactible, show the prompt
        // show prompt
        m_canInteract = true;

        // Check for input
        if( ctx.m_pInputState->GetControllerState()->WasReleased( Input::ControllerButton::FaceButtonUp ) )
        {
            // Create external controller
            m_pController = ctx.m_pAnimationController->TryCreateExternalGraphController<ExternalController>( StringID( "Interaction" ), ctx.m_pPlayerComponent->m_pAvailableInteraction, true );
            if ( m_pController != nullptr )
            {
                ctx.m_pAnimationController->SetCharacterState( Player::CharacterAnimationState::Interaction );
                return true;
            }
        }

        return false;
    }

    Action::Status InteractAction::UpdateInternal( ActionContext const& ctx )
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

    void InteractAction::StopInternal( ActionContext const& ctx, StopReason reason )
    {
        //ctx.m_pAnimationController->DestroyExternalGraphController( StringID( "Interaction" ) );
        m_pController = nullptr;
    }
}