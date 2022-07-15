#include "PlayerAnimationController.h"
#include "GraphControllers/PlayerGraphController_Locomotion.h"
#include "GraphControllers/PlayerGraphController_Ability.h"

//-------------------------------------------------------------------------

namespace EE::Player
{
    AnimationController::AnimationController( Animation::AnimationGraphComponent* pGraphComponent, Render::SkeletalMeshComponent* pMeshComponent )
        : Animation::GraphController( pGraphComponent, pMeshComponent )
    {
        m_subGraphControllers.emplace_back( EE::New<LocomotionGraphController>( pGraphComponent, pMeshComponent ) );
        m_subGraphControllers.emplace_back( EE::New<AbilityGraphController>( pGraphComponent, pMeshComponent ) );
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