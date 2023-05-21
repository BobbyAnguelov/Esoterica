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
        m_headingParam.TryBind( this );
        m_facingParam.TryBind( this );
    }

    void InAirGraphController::SetDesiredMovement( Seconds const deltaTime, Vector const& headingVelocityWS, Vector const& facingDirectionWS )
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
}