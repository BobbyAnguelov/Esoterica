#pragma once
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Controller.h"

//-------------------------------------------------------------------------

namespace EE::Player
{
    class InAirGraphController final : public Animation::SubGraphController
    {

    public:

        EE_ANIMATION_SUBGRAPH_CONTROLLER_ID( InAirGraphController );

    public:

        InAirGraphController( Animation::GraphInstance* pGraphInstance, Render::SkeletalMeshComponent* pMeshComponent );

        // Set the air control movement desires
        void SetDesiredMovement( Seconds const deltaTime, Vector const& movementVelocityWS, Vector const& facingDirectionWS );

    private:

        ControlParameter<StringID>                          m_stateParam = "InAir_State";
        ControlParameter<float>                             m_speedParam = "InAir_Speed";
        ControlParameter<Vector>                            m_movementVelocityParam = "InAir_MovementVelocity";
        ControlParameter<Vector>                            m_facingParam = "InAir_Facing";
    };
}