#include "PlayerGraphController_Locomotion.h"

//-------------------------------------------------------------------------

namespace EE::Player
{
    LocomotionGraphController::LocomotionGraphController( Animation::GraphInstance* pGraphInstance, Render::SkeletalMeshComponent* pMeshComponent )
        : Animation::SubGraphController( pGraphInstance, pMeshComponent )
    {
        m_speedParam.TryBind( this );
        m_headingParam.TryBind( this );
        m_facingParam.TryBind( this );
        m_isCrouchParam.TryBind( this );
        m_isSlidingParam.TryBind( this );
    }

    void LocomotionGraphController::SetIdle()
    {
        m_speedParam.Set( this, 0.0f );
        m_headingParam.Set( this, Vector::Zero );
        m_facingParam.Set( this, Vector::WorldForward );
    }

    void LocomotionGraphController::SetLocomotionDesires( Seconds const deltaTime, Vector const& headingVelocityWS, Vector const& facingDirectionWS )
    {
        Vector const characterSpaceHeading = ConvertWorldSpaceVectorToCharacterSpace( headingVelocityWS );
        float const speed = characterSpaceHeading.GetLength3();

        m_headingParam.Set( this, characterSpaceHeading );
        m_speedParam.Set( this, speed );

        //-------------------------------------------------------------------------

        if ( facingDirectionWS.IsZero3() )
        {
            m_facingParam.Set( this, Vector::WorldForward );
        }
        else
        {
            EE_ASSERT( facingDirectionWS.IsNormalized3() );
            Vector const characterSpaceFacing = ConvertWorldSpaceVectorToCharacterSpace( facingDirectionWS ).GetNormalized2();
            m_facingParam.Set( this, characterSpaceFacing );
        }
    }

    void LocomotionGraphController::SetCrouch( bool isCrouch )
    {
        m_isCrouchParam.Set( this, isCrouch );
    }

    void LocomotionGraphController::SetSliding( bool isSliding )
    {
        m_isSlidingParam.Set( this, isSliding );
    }
}