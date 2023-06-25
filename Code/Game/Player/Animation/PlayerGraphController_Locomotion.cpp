#include "PlayerGraphController_Locomotion.h"
#include "Engine/Animation/Events/AnimationEvent_Transition.h"

//-------------------------------------------------------------------------

namespace EE::Player
{
    StringID const LocomotionGraphController::s_locomotionStateIDs[] =
    {
        StringID( "Idle" ),
        StringID( "TurnOnSpot" ),
        StringID( "Start" ),
        StringID( "Move" ),
        StringID( "PlantedTurn" ),
        StringID( "Stop" ),
    };

    StringID const LocomotionGraphController::s_graphStateIDs[] =
    {
        StringID( "Locomotion_Idle" ),
        StringID( "Locomotion_TurnOnSpot" ),
        StringID( "Locomotion_Start" ),
        StringID( "Locomotion_Move" ),
        StringID( "Locomotion_PlantedTurn" ),
        StringID( "Locomotion_Stop" ),
    };

    //-------------------------------------------------------------------------

    LocomotionGraphController::LocomotionGraphController( Animation::GraphInstance* pGraphInstance, Render::SkeletalMeshComponent* pMeshComponent )
        : Animation::SubGraphController( pGraphInstance, pMeshComponent )
    {
        m_stateParam.TryBind( this );
        m_speedParam.TryBind( this );
        m_movementVelocityParam.TryBind( this );
        m_facingParam.TryBind( this );
        m_isCrouchParam.TryBind( this );
        m_isSlidingParam.TryBind( this );
    }

    void LocomotionGraphController::RequestIdle()
    {
        m_stateParam.Set( s_locomotionStateIDs[States::Idle] );
        m_movementVelocityParam.Set( Vector::Zero );
        m_facingParam.Set( Vector::WorldForward );
        m_speedParam.Set( 0.0f );
    }

    void LocomotionGraphController::RequestTurnOnSpot( Vector const& directionWS )
    {
        m_stateParam.Set( s_locomotionStateIDs[States::TurnOnSpot] );
        m_movementVelocityParam.Set( Vector::Zero );
        m_facingParam.Set( ConvertWorldSpaceVectorToCharacterSpace( directionWS ) );
        m_speedParam.Set( 0.0f );
    }

    void LocomotionGraphController::RequestStart( Vector const& movementVelocityWS )
    {
        Vector const movementVelocityCS = ConvertWorldSpaceVectorToCharacterSpace( movementVelocityWS );
        float const speed = movementVelocityCS.GetLength3();

        //-------------------------------------------------------------------------

        m_stateParam.Set( s_locomotionStateIDs[States::Start] );
        m_movementVelocityParam.Set( movementVelocityCS );
        m_facingParam.Set( Vector::WorldForward );
        m_speedParam.Set( speed );
    }

    void LocomotionGraphController::RequestMove( Seconds const deltaTime, Vector const& movementVelocityWS, Vector const& facingDirectionWS )
    {
        Vector const movementVelocityCS = ConvertWorldSpaceVectorToCharacterSpace( movementVelocityWS );
        float const speed = movementVelocityCS.GetLength3();

        //-------------------------------------------------------------------------

        Vector facingCS = Vector::WorldForward;
        if ( !facingDirectionWS.IsZero3() )
        {
            EE_ASSERT( facingDirectionWS.IsNormalized3() );
            facingCS = ConvertWorldSpaceVectorToCharacterSpace( facingDirectionWS ).GetNormalized2();
        }

        //-------------------------------------------------------------------------

        m_stateParam.Set( s_locomotionStateIDs[States::Move] );
        m_movementVelocityParam.Set( movementVelocityCS );
        m_facingParam.Set( facingCS );
        m_speedParam.Set( speed );
    }

    void LocomotionGraphController::RequestPlantedTurn( Vector const& directionWS )
    {
        m_stateParam.Set( s_locomotionStateIDs[States::PlantedTurn] );
    }

    void LocomotionGraphController::RequestStop( Transform const& target )
    {
        m_stateParam.Set( s_locomotionStateIDs[States::Stop] );
        m_movementVelocityParam.Set( Vector::Zero );
        m_facingParam.Set( Vector::WorldForward );
        m_speedParam.Set( 0.0f );
    }

    //-------------------------------------------------------------------------

    void LocomotionGraphController::SetCrouch( bool isCrouch )
    {
        m_isCrouchParam.Set( isCrouch );
    }

    void LocomotionGraphController::SetSliding( bool isSliding )
    {
        m_isSlidingParam.Set( isSliding );
    }

    //-------------------------------------------------------------------------

    void LocomotionGraphController::PostGraphUpdate( Seconds deltaTime )
    {
        m_graphState = States::Unknown;

        //-------------------------------------------------------------------------

        for ( auto const& sampledEvent : GetSampledEvents() )
        {
            if ( sampledEvent.IsIgnored() )
            {
                continue;
            }

            //-------------------------------------------------------------------------

            if ( sampledEvent.IsStateEvent() )
            {
                if ( sampledEvent.IsEntryEvent() || sampledEvent.IsFullyInStateEvent() )
                {
                    StringID const stateID = sampledEvent.GetStateEventID();
                    if ( stateID == s_graphStateIDs[States::Idle] )
                    {
                        m_graphState = States::Idle;
                    }
                    else if ( stateID == s_graphStateIDs[States::TurnOnSpot] )
                    {
                        m_graphState = States::TurnOnSpot;
                    }
                    else if ( stateID == s_graphStateIDs[States::Start] )
                    {
                        m_graphState = States::Start;
                    }
                    else if ( stateID == s_graphStateIDs[States::Move] )
                    {
                        m_graphState = States::Move;
                    }
                    else if ( stateID == s_graphStateIDs[States::PlantedTurn] )
                    {
                        m_graphState = States::PlantedTurn;
                    }
                    else if ( stateID == s_graphStateIDs[States::Stop] )
                    {
                        m_graphState = States::Stop;
                    }
                }
            }
        }
    }
}