#include "AIGraphController_Locomotion.h"

//-------------------------------------------------------------------------

namespace EE::AI
{
    LocomotionGraphController::LocomotionGraphController( Animation::GraphInstance* pGraphInstance, Render::SkeletalMeshComponent* pMeshComponent )
        : Animation::SubGraphController( pGraphInstance, pMeshComponent )
    {
        m_speedParam.TryBind( this );
        m_headingParam.TryBind( this );
        m_facingParam.TryBind( this );
    }

    void LocomotionGraphController::SetIdle()
    {
        m_speedParam.Set( this, 0.0f );
        m_headingParam.Set( this, Vector::Zero );
        m_facingParam.Set( this, Vector::WorldForward );
    }

    void LocomotionGraphController::SetLocomotionDesires( Seconds const deltaTime, Vector const& headingVelocityWS, Vector const& facingDirectionWS )
    {
        EE_ASSERT( Math::IsNearZero( facingDirectionWS.GetZ() ) );

        Vector const characterSpaceHeading = ConvertWorldSpaceVectorToCharacterSpace( headingVelocityWS );
        float const speed = characterSpaceHeading.GetLength3();

        m_headingParam.Set( this, characterSpaceHeading );
        m_speedParam.Set( this, speed );

        //-------------------------------------------------------------------------

        EE_ASSERT( !facingDirectionWS.IsNaN3() );

        if ( facingDirectionWS.IsZero3() )
        {
            m_facingParam.Set( this, Vector::WorldForward );
        }
        else
        {
            EE_ASSERT( facingDirectionWS.IsNormalized3() );
            Vector const facingDirCS = ConvertWorldSpaceVectorToCharacterSpace( facingDirectionWS ).GetNormalized2();
            m_facingParam.Set( this, facingDirCS );
        }
    }
}