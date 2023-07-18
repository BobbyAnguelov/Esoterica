#include "AIBehavior_CombatPositioning.h"
#include "Engine/Navmesh/Systems/WorldSystem_Navmesh.h"
#include "Base/Math/MathRandom.h"
#include "Base/Math/BoundingVolumes.h"

//-------------------------------------------------------------------------

namespace EE::AI
{
    void CombatPositionBehavior::StartInternal( BehaviorContext const& ctx )
    {
        m_waitTimer.Start( Math::GetRandomFloat( 1.0f, 3.0f ) );
    }

    Behavior::Status CombatPositionBehavior::UpdateInternal( BehaviorContext const& ctx )
    {
        // Remain in idle if there is no valid navmesh
        AABB navmeshBounds = ctx.m_pNavmeshSystem->GetNavmeshBounds( 0 );
        if ( !navmeshBounds.IsValid() )
        {
            return Status::Running;
        }

        //-------------------------------------------------------------------------

        if ( m_waitTimer.IsRunning() )
        {
            m_idleAction.Update( ctx );

            // Wait for the timer to elapse and start a move
            if ( m_waitTimer.Update( ctx.GetDeltaTime() ) )
            {
                Vector const boundsMin = navmeshBounds.GetMin();
                Vector const boundsMax = navmeshBounds.GetMax();
                Vector const moveGoalPosition( Math::GetRandomFloat( boundsMin.GetX(), boundsMax.GetX() ), Math::GetRandomFloat( boundsMin.GetY(), boundsMax.GetY() ), navmeshBounds.GetCenter().GetZ() );

                m_moveToAction.Start( ctx, moveGoalPosition );
            }
        }
        else // We're moving
        {
            // Wait for the move to complete
            if ( m_moveToAction.IsRunning() )
            {
                m_moveToAction.Update( ctx );
            }

            // if the move completed, restart the wait timer
            if ( !m_moveToAction.IsRunning() )
            {
                m_idleAction.Start( ctx );
                m_waitTimer.Start( Math::GetRandomFloat( 1.0f, 3.0f ) );
            }
        }

        //-------------------------------------------------------------------------

        return Status::Running;
    }

    void CombatPositionBehavior::StopInternal( BehaviorContext const& ctx, StopReason reason )
    {

    }
}