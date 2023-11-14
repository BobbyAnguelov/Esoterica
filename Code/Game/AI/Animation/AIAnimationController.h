#pragma once

#include "Engine/Animation/Graph/Animation_RuntimeGraph_Controller.h"

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

        // Locomotion
        //-------------------------------------------------------------------------

        // Request the character go into an idle state
        void SetIdle();

        // Set movement parameters - facing direction will be converted to 2D
        void SetLocomotionDesires( Seconds const deltaTime, Vector const& movementVelocityWS, Vector const& facingDirectionWS );

    private:

        ControlParameter<StringID>     m_characterStateParam = "CharacterState";

        // Locomotion
        //-------------------------------------------------------------------------

        ControlParameter<float>     m_speedParam = "Locomotion_Speed";
        ControlParameter<Vector>    m_movementVelocityParam = "Locomotion_MovementVelocity";
        ControlParameter<Vector>    m_facingParam = "Locomotion_Facing";
    };
}