#pragma once
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Controller.h"

//-------------------------------------------------------------------------

namespace EE::Player
{
    class LocomotionGraphController final : public Animation::SubGraphController
    {
        enum States
        {
            Unknown = -1,
            Idle = 0,
            TurnOnSpot,
            Start,
            Move,
            PlantedTurn,
            Stop,

            NumStates,
        };

        static StringID const s_locomotionStateIDs[NumStates];
        static StringID const s_graphStateIDs[NumStates];

    public:

        EE_ANIMATION_SUBGRAPH_CONTROLLER_ID( LocomotionGraphController );

    public:

        LocomotionGraphController( Animation::GraphInstance* pGraphInstance, Render::SkeletalMeshComponent* pMeshComponent );

        void RequestIdle();
        void RequestTurnOnSpot( Vector const& directionWS );
        void RequestStart( Vector const& headingVelocityWS );
        void RequestMove( Seconds const deltaTime, Vector const& headingVelocityWS, Vector const& facingDirectionWS );
        void RequestPlantedTurn( Vector const& directionWS );
        void RequestStop( Transform const& target );

        void SetCrouch( bool isCrouch );
        void SetSliding( bool isSliding );

        // State check
        inline bool IsIdle() const { return m_graphState == States::Idle; }
        inline bool IsTurningOnSpot() const { return m_graphState == States::TurnOnSpot; }
        inline bool IsStarting() const { return m_graphState == States::Start; }
        inline bool IsMoving() const { return m_graphState == States::Move; }
        inline bool IsPlantingAndTurning() const { return m_graphState == States::PlantedTurn; }
        inline bool IsStopping() const { return m_graphState == States::Stop; }
        
    private:

        virtual void PostGraphUpdate( Seconds deltaTime ) override;

    private:

        ControlParameter<StringID>                          m_stateParam = ControlParameter<StringID>( "Locomotion_State" );
        ControlParameter<bool>                              m_isCrouchParam = ControlParameter<bool>( "Locomotion_IsCrouched" );
        ControlParameter<bool>                              m_isSlidingParam = ControlParameter<bool>( "Locomotion_IsSliding" );
        ControlParameter<float>                             m_speedParam = ControlParameter<float>( "Locomotion_Speed" );
        ControlParameter<Vector>                            m_headingParam = ControlParameter<Vector>( "Locomotion_Heading" );
        ControlParameter<Vector>                            m_facingParam = ControlParameter<Vector>( "Locomotion_Facing" );

        States                                              m_graphState = States::Unknown;
    };
}