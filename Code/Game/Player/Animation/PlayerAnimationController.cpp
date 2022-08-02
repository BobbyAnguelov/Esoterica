#include "PlayerAnimationController.h"
#include "Game/Player/Animation/PlayerGraphController_Locomotion.h"
#include "Game/Player/Animation/PlayerGraphController_Ability.h"

//-------------------------------------------------------------------------

namespace EE::Player
{
    AnimationController::AnimationController( Animation::AnimationGraphComponent* pGraphComponent, Render::SkeletalMeshComponent* pMeshComponent )
        : Animation::GraphController( pGraphComponent, pMeshComponent )
    {
        CreateSubGraphController<LocomotionGraphController>();
        CreateSubGraphController<AbilityGraphController>();
        m_characterStateParam.TryBind( this );
    }

    void AnimationController::SetCharacterState( CharacterAnimationState state )
    {
        static StringID const characterStates[(uint8_t) CharacterAnimationState::NumStates] =
        {
            StringID( "Locomotion" ),
            StringID( "Falling" ),
            StringID( "Ability" ),
            StringID( "Interaction" ),

            StringID( "DebugMode" ),
        };

        EE_ASSERT( state < CharacterAnimationState::NumStates );
        m_characterStateParam.Set( this, characterStates[(uint8_t) state] );
    }
}