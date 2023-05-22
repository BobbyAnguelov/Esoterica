#include "PlayerAnimationController.h"
#include "PlayerGraphController_InAir.h"
#include "PlayerGraphController_Locomotion.h"
#include "PlayerGraphController_Ability.h"

//-------------------------------------------------------------------------

namespace EE::Player
{
    AnimationController::AnimationController( Animation::GraphComponent* pGraphComponent, Render::SkeletalMeshComponent* pMeshComponent )
        : Animation::GraphController( pGraphComponent, pMeshComponent )
    {
        CreateSubGraphController<LocomotionGraphController>();
        CreateSubGraphController<AbilityGraphController>();
        CreateSubGraphController<InAirGraphController>();
        m_characterStateParam.TryBind( this );
    }

    void AnimationController::SetCharacterState( CharacterAnimationState state )
    {
        static StringID const characterStates[(uint8_t) CharacterAnimationState::NumStates] =
        {
            StringID( "Locomotion" ),
            StringID( "InAir" ),
            StringID( "Ability" ),
            StringID( "Interaction" ),
            StringID( "GhostMode" ),
        };

        EE_ASSERT( state < CharacterAnimationState::NumStates );
        m_characterStateParam.Set( this, characterStates[(uint8_t) state] );
    }

    void AnimationController::PostGraphUpdate( Seconds deltaTime )
    {
        Animation::GraphController::PostGraphUpdate( deltaTime );

        //-------------------------------------------------------------------------

        m_transitionMarker = Animation::TransitionMarker::BlockTransition;
        m_hasTransitionMarker = false;

        //-------------------------------------------------------------------------

        for ( auto const& sampledEvent : GetSampledEvents() )
        {
            if ( sampledEvent.IsIgnored() )
            {
                continue;
            }

            //-------------------------------------------------------------------------

            if ( sampledEvent.IsAnimationEvent() )
            {
                if ( sampledEvent.IsFromActiveBranch() )
                {
                    if ( auto pTransitionEvent = sampledEvent.TryGetEvent<Animation::TransitionEvent>() )
                    {
                        if ( pTransitionEvent->GetMarker() < m_transitionMarker )
                        {
                            m_transitionMarker = pTransitionEvent->GetMarker();
                            m_hasTransitionMarker = true;
                        }
                    }
                }
            }
        }
    }
}