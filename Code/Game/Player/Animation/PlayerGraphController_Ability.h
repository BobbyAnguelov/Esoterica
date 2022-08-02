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

    private:

        ControlParameter<StringID>     m_abilityID = ControlParameter<StringID>( "Ability_ID" );
    };
}