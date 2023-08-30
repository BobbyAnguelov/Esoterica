#pragma once
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Controller.h"

//-------------------------------------------------------------------------

namespace EE::Player
{
    class AbilityGraphController final : public Animation::SubGraphController
    {
    public:

        EE_ANIMATION_SUBGRAPH_CONTROLLER_ID( AbilityGraphController );

    public:

        AbilityGraphController( Animation::GraphInstance* pGraphInstance, Render::SkeletalMeshComponent* pMeshComponent );

        void StartJump();
        void StartDash();
        void StartSlide();

        void SetDesiredMovement( Seconds const deltaTime, Vector const& movementVelocityWS, Vector const& facingDirectionWS );

    private:

        ControlParameter<StringID>      m_abilityID = "Ability_ID";
        ControlParameter<float>         m_speedParam = "Ability_Speed";
        ControlParameter<Vector>        m_movementVelocityParam =  "Ability_MovementVelocity";
        ControlParameter<Vector>        m_facingParam = "Ability_Facing";
    };
}