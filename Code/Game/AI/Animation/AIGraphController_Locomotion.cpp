#include "AIGraphController_Locomotion.h"

//-------------------------------------------------------------------------

namespace EE::AI
{
    LocomotionGraphController::LocomotionGraphController( Animation::GraphInstance* pGraphInstance, Render::SkeletalMeshComponent* pMeshComponent )
        : Animation::SubGraphController( pGraphInstance, pMeshComponent )
    {
        m_speedParam.TryBind( this );
        m_movementVelocityParam.TryBind( this );
        m_facingParam.TryBind( this );
    }

    void LocomotionGraphController::SetIdle()
    {
        m_speedParam.Set( 0.0f );
        m_movementVelocityParam.Set( Vector::Zero );
        m_facingParam.Set( Vector::WorldForward );
    }

    void LocomotionGraphController::SetLocomotionDesires( Seconds const deltaTime, Vector const& movementVelocityWS, Vector const& facingDirectionWS )
    {
        EE_ASSERT( Math::IsNearZero( facingDirectionWS.GetZ() ) );

        Vector const movementVelocityCS = ConvertWorldSpaceVectorToCharacterSpace( movementVelocityWS );
        float const speed = movementVelocityCS.GetLength3();

        m_movementVelocityParam.Set( movementVelocityCS );
        m_speedParam.Set( speed );

        //-------------------------------------------------------------------------

        EE_ASSERT( !facingDirectionWS.IsNaN3() );

        if ( facingDirectionWS.IsZero3() )
        {
            m_facingParam.Set( Vector::WorldForward );
        }
        else
        {
            EE_ASSERT( facingDirectionWS.IsNormalized3() );
            Vector const facingDirCS = ConvertWorldSpaceVectorToCharacterSpace( facingDirectionWS ).GetNormalized2();
            m_facingParam.Set( facingDirCS );
        }
    }
}