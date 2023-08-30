#pragma once
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Controller.h"

//-------------------------------------------------------------------------

namespace EE::AI
{
    class LocomotionGraphController final : public Animation::SubGraphController
    {
    public:

        EE_ANIMATION_SUBGRAPH_CONTROLLER_ID( LocomotionGraphController );

    public:

        LocomotionGraphController( Animation::GraphInstance* pGraphInstance, Render::SkeletalMeshComponent* pMeshComponent );

        // Request the character go into an idle state
        void SetIdle();

        // Set movement parameters - facing direction will be converted to 2D
        void SetLocomotionDesires( Seconds const deltaTime, Vector const& movementVelocityWS, Vector const& facingDirectionWS );

    private:

        ControlParameter<float>     m_speedParam = "Locomotion_Speed";
        ControlParameter<Vector>    m_movementVelocityParam = "Locomotion_MovementVelocity";
        ControlParameter<Vector>    m_facingParam = "Locomotion_Facing";
    };
}