#pragma once

#include "Game/Player/Animation/PlayerGraphController_Locomotion.h"

//-------------------------------------------------------------------------

namespace EE::AI
{
    enum class CharacterAnimationState : uint8_t
    {
        Locomotion = 0,
        Falling,
        Ability,

        DebugMode,
        NumStates
    };

    //-------------------------------------------------------------------------

    class AnimationController final : public Animation::GraphController
    {
    public:

        AnimationController( Animation::GraphComponent* pGraphComponent, Render::SkeletalMeshComponent* pMeshComponent );

        void SetCharacterState( CharacterAnimationState state );

        #if EE_DEVELOPMENT_TOOLS
        virtual char const* GetName() const { return "AI Graph Controller"; }
        #endif

    private:

        ControlParameter<StringID>     m_characterStateParam = "CharacterState";
    };
}