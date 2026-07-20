#include "BehaviorAction_MoveTo.h"
#include "Game/NPC/Animation/NPCAnimationController.h"
#include "Engine/Navmesh/Systems/WorldSystem_Navmesh.h"
#include "Engine/Navmesh/NavPower.h"
#include "Base/Math/Line.h"

//-------------------------------------------------------------------------

namespace EE
{
    void BehaviorAction_MoveTo::Start( BehaviorContext const& ctx, Vector const& goalPosition )
    {
        ctx.m_pAnimationController->SetCharacterState( CharacterAnimationState::Locomotion );

        //-------------------------------------------------------------------------

        #if EE_ENABLE_NAVPOWER
        auto spaceHandle = ctx.m_pNavmeshSystem->GetSpaceHandle();

        bfx::PathSpec pathSpec;
        pathSpec.m_snapMode = bfx::SNAP_CLOSEST;

        bfx::PathCreationOptions pathOptions;
        pathOptions.m_forceFirstPosOntoNavGraph = true;

        m_path = bfx::CreatePolylinePath( spaceHandle, Navmesh::ToBfx( ctx.m_pNPC->GetPosition() ), Navmesh::ToBfx( goalPosition ), 0, pathSpec, pathOptions );
        if ( m_path.IsValid() )
        {
            m_currentPathSegmentIdx = 0;
            m_progressAlongSegment = 0.0f;
        }
        else
        {
            m_currentPathSegmentIdx = InvalidIndex;
        }
        #endif
    }

    BehaviorAction::Status BehaviorAction_MoveTo::Update( BehaviorContext const& ctx )
    {
        #if EE_ENABLE_NAVPOWER
        if ( !m_path.IsValid() )
        {
            return Status::Failed;
        }

        float const moveSpeed = 5.5f;
        float distanceToMove = moveSpeed * ctx.GetDeltaTime();

        //-------------------------------------------------------------------------

        Vector facingDir = ctx.m_pNPC->GetForwardVector();
        EE_ASSERT( m_currentPathSegmentIdx != InvalidIndex );
        bfx::SurfaceSegment const* pCurrentSegment = m_path.GetSurfaceSegment( m_currentPathSegmentIdx );
        Vector const currentSegmentStartPos = Navmesh::FromBfx( pCurrentSegment->GetStartPos() );
        Vector const currentSegmentEndPos = Navmesh::FromBfx( pCurrentSegment->GetEndPos() );

        Vector currentPosition;
        if ( !currentSegmentStartPos.IsNearEqual3( currentSegmentEndPos ) )
        {
            currentPosition = Line( Line::StartEnd, currentSegmentStartPos, currentSegmentEndPos ).GetClosestPoint( ctx.m_pNPC->GetPosition() );
        }
        else
        {
            currentPosition = currentSegmentStartPos;
        }

        // Find the goal position for this frame
        //-------------------------------------------------------------------------

        Vector goalPosition;

        bool atEndOfPath = false;
        while ( distanceToMove > 0 )
        {
            bool const isLastSegment = m_currentPathSegmentIdx == ( m_path.GetNumSegments() - 1 );

            bfx::SurfaceSegment const* pSegment = m_path.GetSurfaceSegment( m_currentPathSegmentIdx );
            Vector segmentStart( Navmesh::FromBfx( pSegment->GetStartPos() ) );
            Vector segmentEnd( Navmesh::FromBfx( pSegment->GetEndPos() ) );

            // Handle zero length segments
            Vector const segmentVector( segmentEnd - segmentStart );
            if ( segmentVector.IsZero3() )
            {
                distanceToMove = 0.0f;
                m_progressAlongSegment = 1.0f;
                goalPosition = segmentEnd;

                if( isLastSegment )
                {
                    atEndOfPath = true;
                }

                break;
            }

            //-------------------------------------------------------------------------

            Vector segmentDir;
            float segmentLength;
            ( segmentEnd - segmentStart ).ToDirectionAndLength3( segmentDir, segmentLength );

            EE_ASSERT( segmentDir.IsNormalized3() );

            float currentDistanceAlongSegment = segmentLength * m_progressAlongSegment;
            float remainingDistance = segmentLength - currentDistanceAlongSegment;

            //-------------------------------------------------------------------------

            float newDistanceAlongSegment = currentDistanceAlongSegment + distanceToMove;
            if ( !isLastSegment && newDistanceAlongSegment > segmentLength )
            {
                distanceToMove -= remainingDistance;
                m_progressAlongSegment = 0.0f;
                m_currentPathSegmentIdx++;
            }
            else // Perform full move
            {
                distanceToMove = 0.0f;
                m_progressAlongSegment = Math::Min( newDistanceAlongSegment / segmentLength, 1.0f );
                goalPosition = Vector::Lerp( segmentStart, segmentEnd, m_progressAlongSegment );
                facingDir = segmentDir;

                if ( isLastSegment && m_progressAlongSegment == 1.0f )
                {
                    atEndOfPath = true;
                }
            }
        }

        // Calculate goal pos
        //-------------------------------------------------------------------------

        Vector const desiredDelta = ( goalPosition - currentPosition );
        Vector const movementVelocity = desiredDelta / ctx.GetDeltaTime();
        facingDir = facingDir.GetNormalized2();
        ctx.m_pAnimationController->SetLocomotionDesires( ctx.GetDeltaTime(), movementVelocity, facingDir );

        // Check if we are at the end of the path
        //-------------------------------------------------------------------------

        if ( atEndOfPath )
        {
            m_path.Release();
        }

        return m_path.IsValid() ? Status::Running : Status::Completed;
        #else
        return Status::Completed;
        #endif
    }
}
