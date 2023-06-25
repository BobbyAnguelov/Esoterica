#include "PlayerGraphController_Ability.h"

//-------------------------------------------------------------------------

namespace EE::Player
{
    AbilityGraphController::AbilityGraphController( Animation::GraphInstance* pGraphInstance, Render::SkeletalMeshComponent* pMeshComponent )
        : Animation::SubGraphController( pGraphInstance, pMeshComponent )
    {
        m_abilityID.TryBind( this );
        m_facingParam.TryBind( this );
        m_movementVelocityParam.TryBind( this );
        m_speedParam.TryBind( this );
    }

    void AbilityGraphController::StartJump()
    {
        static StringID const jumpID( "Jump" );
        m_abilityID.Set( jumpID );
    }

    void AbilityGraphController::StartDash()
    {
        static StringID const dashID( "Dash" );
        m_abilityID.Set( dashID );
    }

    void AbilityGraphController::StartSlide()
    {
        static StringID const dashID( "Slide" );
        m_abilityID.Set( dashID );
    }

    void AbilityGraphController::SetDesiredMovement( Seconds const deltaTime, Vector const& movementVelocityWS, Vector const& facingDirectionWS )
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