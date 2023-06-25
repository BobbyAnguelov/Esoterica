#include "PlayerGraphController_InAir.h"
#include "Engine/Animation/Events/AnimationEvent_Transition.h"

//-------------------------------------------------------------------------

namespace EE::Player
{
    InAirGraphController::InAirGraphController( Animation::GraphInstance* pGraphInstance, Render::SkeletalMeshComponent* pMeshComponent )
        : Animation::SubGraphController( pGraphInstance, pMeshComponent )
    {
        m_stateParam.TryBind( this );
        m_speedParam.TryBind( this );
        m_movementVelocityParam.TryBind( this );
        m_facingParam.TryBind( this );
    }

    void InAirGraphController::SetDesiredMovement( Seconds const deltaTime, Vector const& movementVelocityWS, Vector const& facingDirectionWS )
    {
        Vector const movementVelocityCS = ConvertWorldSpaceVectorToCharacterSpace( movementVelocityWS );
        float const speed = movementVelocityCS.GetLength3();

        m_movementVelocityParam.Set( movementVelocityCS );
        m_speedParam.Set( speed );

        //-------------------------------------------------------------------------

        if ( facingDirectionWS.IsZero3() )
        {
            m_facingParam.Set( Vector::WorldForward );
        }
        else
        {
            EE_ASSERT( facingDirectionWS.IsNormalized3() );
            Vector const characterSpaceFacing = ConvertWorldSpaceVectorToCharacterSpace( facingDirectionWS ).GetNormalized2();
            m_facingParam.Set( characterSpaceFacing );
        }
    }
}