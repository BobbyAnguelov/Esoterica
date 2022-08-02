#pragma once
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Controller.h"

//-------------------------------------------------------------------------

namespace EE::Player
{
    enum class CharacterAnimationState : uint8_t
    {
        Locomotion = 0,
        Falling,
        Ability,
        Interaction,

        DebugMode,
        NumStates
    };

    //-------------------------------------------------------------------------

    class AnimationController final : public Animation::GraphController
    {

    public:

        AnimationController( Animation::AnimationGraphComponent* pGraphComponent, Render::SkeletalMeshComponent* pMeshComponent );

        void SetCharacterState( CharacterAnimationState state );

        #if EE_DEVELOPMENT_TOOLS
        virtual char const* GetName() const { return "Player Graph Controller"; }
        #endif

    private:

        ControlParameter<StringID>     m_characterStateParam = "CharacterState";
    };
}