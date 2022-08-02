#include "AIAnimationController.h"
#include "Game/AI/Animation/AIGraphController_Locomotion.h"

//-------------------------------------------------------------------------

namespace EE::AI
{
    AnimationController::AnimationController( Animation::AnimationGraphComponent* pGraphComponent, Render::SkeletalMeshComponent* pMeshComponent )
        : Animation::GraphController( pGraphComponent, pMeshComponent )
    {
        CreateSubGraphController<LocomotionGraphController>();
        m_characterStateParam.TryBind( this );
    }

    void AnimationController::SetCharacterState( CharacterAnimationState state )
    {
        static StringID const characterStates[(uint8_t) CharacterAnimationState::NumStates] =
        {
            StringID( "Locomotion" ),
            StringID( "Falling" ),
            StringID( "Ability" ),
            StringID( "DebugMode" ),
        };

        EE_ASSERT( state < CharacterAnimationState::NumStates );
        m_characterStateParam.Set( this, characterStates[(uint8_t) state] );
    }
}