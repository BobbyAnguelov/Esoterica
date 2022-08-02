#include "PlayerGraphController_Ability.h"

//-------------------------------------------------------------------------

namespace EE::Player
{
    AbilityGraphController::AbilityGraphController( Animation::GraphInstance* pGraphInstance, Render::SkeletalMeshComponent* pMeshComponent )
        : Animation::SubGraphController( pGraphInstance, pMeshComponent )
    {
        m_abilityID.TryBind( this );
    }

    void AbilityGraphController::StartJump()
    {
        static StringID const jumpID( "Jump" );
        m_abilityID.Set( this, jumpID );
    }

    void AbilityGraphController::StartDash()
    {
        static StringID const dashID( "Dash" );
        m_abilityID.Set( this, dashID );
    }

    void AbilityGraphController::StartSlide()
    {
        static StringID const dashID( "Slide" );
        m_abilityID.Set( this, dashID );
    }
}