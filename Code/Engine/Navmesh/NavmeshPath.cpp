#include "NavmeshPath.h"
#include "Base/Drawing/DebugDrawing.h"
#include "Base/Math/MathUtils.h"

//-------------------------------------------------------------------------

#if EE_ENABLE_NAVPOWER
namespace EE::Navmesh
{
    Path::Segment::Segment( Vector const& start, Vector const& end )
    {
        m_isBezier = false;
        m_start = start;
        m_end = end;
        m_length = m_end.GetDistance3( m_start );

        if ( m_length > 0.0f )
        {
            m_startTangent = m_endTangent = ( m_end - m_start ).GetNormalized3();
        }
        else
        {
            m_startTangent = m_endTangent = Vector::WorldForward;
        }
    }

    Path::Segment::Segment( Vector const& start, Vector const& end, Vector const& cp0, Vector const& cp1 )
    {
        m_isBezier = true;
        m_start = start;
        m_end = end;
        m_cp0 = cp0;
        m_cp1 = cp1;

        m_polyline.m_segments.reserve( s_polylineSubdivisions );
        Math::CubicBezier::CreatePolyline( m_start, m_cp0, m_cp1, m_end, s_polylineSubdivisions, m_polyline );
        m_length = m_polyline.m_length;

        if ( m_length > 0.0f )
        {
            m_startTangent = m_polyline.m_segments.front().m_direction;
            m_endTangent = m_polyline.m_segments.back().m_direction;
        }
        else
        {
            m_startTangent = m_endTangent = Vector::WorldForward;
        }
    }

    //-------------------------------------------------------------------------

    Path::Path( bfx::SpaceHandle& space, Vector const& start, Vector const& end )
        : m_space( space )
    {
        bfx::PathCreationOptions options;
        options.m_forceFirstPosOntoNavGraph = true;

        m_pathPtr = bfx::CreatePolylinePath( space, ToBfx( start ), ToBfx( end ), 0, m_pathSpec, options );
        if ( m_pathPtr.IsValid() )
        {
            PerformSmoothing();
        }
    }

    void Path::PerformSmoothing()
    {
        int32_t const numSegments = m_pathPtr.GetNumSegments();
        m_smoothedPath.reserve( numSegments );

        if ( numSegments == 1 )
        {
            EE_ASSERT( m_pathPtr.GetSegmentType( 0 ) == bfx::SURFACE_SEGMENT );
            bfx::SurfaceSegment* pSegment = m_pathPtr.GetSurfaceSegment( 0 );
            m_start = FromBfx( pSegment->GetStartPos() );
            m_end = FromBfx( pSegment->GetStartPos() );
            m_length = m_end.GetDistance3( m_start );
            m_smoothedPath.emplace_back( m_start, m_end );
            return;
        }

        //-------------------------------------------------------------------------

        bfx::ProbeSpec probeSpec;
        bfx::ProbeResults probeResults;

        m_start = FromBfx( m_pathPtr.GetSurfaceSegment( 0 )->GetStartPos() );
        m_end = FromBfx( m_pathPtr.GetSurfaceSegment( numSegments - 1 )->GetEndPos() );

        //-------------------------------------------------------------------------
        // Push Path Points
        //-------------------------------------------------------------------------

        TInlineVector<Point, 75> points;

        int32_t const lastSegmentIdx = numSegments - 1;
        for ( int32_t i = 0; i < numSegments; i++ )
        {
            bfx::SegmentType segmentType = m_pathPtr.GetSegmentType( i );
            if ( segmentType == bfx::SURFACE_SEGMENT )
            {
                bfx::SurfaceSegment* pSegment = m_pathPtr.GetSurfaceSegment( i );

                if ( i == 0 )
                {
                    points.emplace_back( pSegment->GetStartPos(), pSegment->GetArea(), bfx::ZERO_VECTOR, pSegment->GetStartPos(), pSegment->GetArea() );
                }
                else if ( i == lastSegmentIdx )
                {
                    points.emplace_back( pSegment->GetEndPos(), pSegment->GetArea(), bfx::ZERO_VECTOR, pSegment->GetEndPos(), pSegment->GetArea() );
                }

                auto const cornerType = pSegment->GetCornerTypeAtEnd();
                if ( cornerType == bfx::CORNERTYPE_REAL_CORNER || cornerType == bfx::CORNERTYPE_INTERMEDIATE_AREA_BOUNDARY )
                {
                    // Calculate segment dir and normal
                    Vector const segmentStartPos = FromBfx( pSegment->GetStartPos() );
                    Vector const segmentEndPos = FromBfx( pSegment->GetEndPos() );
                    Vector const segmentDir = ( segmentEndPos - segmentStartPos ).GetNormalized2();
                    Vector const segmentNormal = ( FromBfx( pSegment->GetNormal() ).Cross3( segmentDir ) ).GetNormalized2();

                    // Get next segment dir and normal
                    bfx::SurfaceSegment* pNextSegment = m_pathPtr.GetSurfaceSegment( i + 1 );
                    Vector const nextSegmentStartPos = FromBfx( pNextSegment->GetStartPos() );
                    Vector const nextSegmentEndPos = FromBfx( pNextSegment->GetEndPos() );
                    Vector const nextSegmentDir = ( nextSegmentEndPos - nextSegmentStartPos ).GetNormalized2();

                    // Calculate the corner normal
                    //-------------------------------------------------------------------------

                    Vector cornerNormal = Vector::Zero;

                    // If this is a boundary cross, just keep the push away normal from the previous points as we are likely on a straight line
                    Vector const nextSegmentNormal = ( FromBfx( pNextSegment->GetNormal() ).Cross3( nextSegmentDir ) ).GetNormalized2();
                    if ( cornerType == bfx::CORNERTYPE_INTERMEDIATE_AREA_BOUNDARY && points.size() > 1 )
                    {
                        cornerNormal = FromBfx( points.back().m_normal );
                    }
                    else // Calculate corner normal using the two segment directions
                    {
                        Vector originalEndPoint = FromBfx( pSegment->GetEndPos() );
                        cornerNormal = ( segmentNormal + nextSegmentNormal ) / 2;
                        if ( !Math::IsVectorToTheLeft( segmentDir, nextSegmentDir ) )
                        {
                            cornerNormal.Negate();
                        }
                    }

                    // Handle last segment explicitly
                    if ( ( i + 1 ) == lastSegmentIdx )
                    {
                        float const segmentLength = ( nextSegmentEndPos - nextSegmentStartPos ).GetLength3();
                        if ( segmentLength < s_pushAwayDistance )
                        {
                            cornerNormal = Vector::Lerp( nextSegmentDir.GetNegated(), cornerNormal, segmentLength / s_pushAwayDistance ).GetNormalized2();
                        }
                    }

                    // Push
                    //-------------------------------------------------------------------------

                    // Push corner by fixed distance
                    bfx::Vector3 const pushStartPoint = pSegment->GetEndPos();
                    bfx::Vector3 const pushNormal = ToBfx( cornerNormal );
                    bfx::AreaHandle const pushStartArea = pSegment->GetArea();
                    bfx::NavProbe( pushStartArea, pushStartPoint, pushNormal, s_pushAwayDistance, m_pathSpec, probeSpec, probeResults );

                    // If we collided, pull it back a bit
                    if ( probeResults.m_collided )
                    {
                        bfx::NavProbe( pushStartArea, pushStartPoint, pushNormal, probeResults.m_distTravelled * s_pushAwayReductionFactor, m_pathSpec, probeSpec, probeResults );
                    }

                    // Check visibility to previous pushed point and next segment
                    bool bothReachable = false;
                    int32_t watchdogCounter = 0;
                    while ( !bothReachable )
                    {
                        if ( watchdogCounter++ > 20 )
                        {
                            EE_UNREACHABLE_CODE();
                            break;
                        }

                        if ( !bfx::IsStraightLineReachable( probeResults.m_endPos, probeResults.m_endArea, points.back().m_pushedPoint, points.back().m_pushedPointArea, m_pathSpec ) )
                        {
                            bfx::NavProbe( pushStartArea, pushStartPoint, pushNormal, probeResults.m_distTravelled * s_pushAwayReductionFactor, m_pathSpec, probeSpec, probeResults );
                        }
                        else if ( !bfx::IsStraightLineReachable( probeResults.m_endPos, probeResults.m_endArea, pNextSegment->GetEndPos(), pNextSegment->GetArea(), m_pathSpec ) )
                        {
                            bfx::NavProbe( pushStartArea, pushStartPoint, pushNormal, probeResults.m_distTravelled * s_pushAwayReductionFactor, m_pathSpec, probeSpec, probeResults );
                        }
                        else
                        {
                            bothReachable = true;
                        }
                    }

                    points.emplace_back( pushStartPoint, pushStartArea, pushNormal, probeResults.m_endPos, probeResults.m_endArea, probeResults.m_distTravelled );
                }
            }
            else
            {
                EE_UNIMPLEMENTED_FUNCTION();
            }
        }

        //-------------------------------------------------------------------------
        // Remove intermediate reachable points
        //-------------------------------------------------------------------------

        for ( int32_t i = 0; i < (int32_t) points.size(); i++ )
        {
            Point const& startPoint = points[i];

            for ( int32_t j = i + 2; j < (int32_t) points.size(); j++ )
            {
                Point const& endPoint = points[j];

                // If we can successfully skip the intermediate point, check the elevation difference, if that is within the threshold remove the intermediate point
                if ( bfx::IsStraightLineReachable( startPoint.m_pushedPoint, startPoint.m_pushedPointArea, endPoint.m_pushedPoint, endPoint.m_pushedPointArea, m_pathSpec ) )
                {
                    Vector const elevationChangeVector = ( Vector( 1, 0, points[i + 2].m_pushedPoint.m_z - points[i + 1].m_pushedPoint.m_z ) ).GetNormalized3();
                    Radians const elevationChangeAngle = Math::CalculateAngleBetweenUnitVectors( Vector::UnitX, elevationChangeVector );
                    if ( elevationChangeAngle < s_angleThresholdRadians )
                    {
                        // Delete intermediate point
                        points.erase( points.begin() + j - 1 );
                        j--;
                    }
                }
                else // Not-reachable so no need to check further
                {
                    break;
                }
            }
        }

        //-------------------------------------------------------------------------
        // Create Path
        //-------------------------------------------------------------------------

        // We are already at the goal
        if ( points.size() == 1 )
        {
            m_smoothedPath.emplace_back( m_start, m_start );
        }
        // Simple straight line path
        else if ( points.size() == 2 )
        {
            m_smoothedPath.emplace_back( FromBfx( points[0].m_pushedPoint ), FromBfx( points[1].m_pushedPoint ) );
            m_length += m_smoothedPath.back().m_length;
        }
        else // Regular path
        {
            for ( int32_t i = 1; i < int32_t( points.size() ) - 1; i++ )
            {
                Point const& point = points[i];
                Vector const cornerPoint = FromBfx( point.m_pushedPoint );

                // Calculate curve control points
                //-------------------------------------------------------------------------

                Vector incomingDir, outGoingDir;
                float incomingLength = 0, outgoingLength = 0;
                ( cornerPoint - FromBfx( points[i - 1].m_pushedPoint ) ).ToDirectionAndLength3( incomingDir, incomingLength );
                ( FromBfx( points[i + 1].m_pushedPoint ) - cornerPoint ).ToDirectionAndLength3( outGoingDir, outgoingLength );

                float const incomingDist = Math::Clamp( 0.35f * incomingLength, 0.1f, 2.0f );
                float const outgoingDist = Math::Clamp( 0.35f * outgoingLength, 0.1f, 2.0f );

                Vector const curveStartPoint = Vector::MultiplyAdd( incomingDir, Vector( -incomingDist ), cornerPoint );
                Vector const curveEndPoint = Vector::MultiplyAdd( outGoingDir, Vector( outgoingDist ), cornerPoint );
                Vector const cp0 = cornerPoint;
                Vector const cp1 = cornerPoint;

                // Add previous segment
                //-------------------------------------------------------------------------

                if ( m_smoothedPath.empty() )
                {
                    m_smoothedPath.emplace_back( m_start, curveStartPoint );
                    m_length += m_smoothedPath.back().m_length;
                }
                else [[likely]]
                {
                    EE_ASSERT( m_smoothedPath.back().m_isBezier );
                    Segment& segment = m_smoothedPath.emplace_back( m_smoothedPath.back().m_end, curveStartPoint );

                    Segment const& previousSegment = *( m_smoothedPath.end() - 2 );
                    segment.m_distanceAlongPath = previousSegment.m_distanceAlongPath + previousSegment.m_length;
                    m_length += m_smoothedPath.back().m_length;
                }

                // Add curve
                //-------------------------------------------------------------------------

                Segment& segment = m_smoothedPath.emplace_back( curveStartPoint, curveEndPoint, cp0, cp1 );
                Segment const& previousSegment = *( m_smoothedPath.end() - 2 );
                segment.m_distanceAlongPath = previousSegment.m_distanceAlongPath + previousSegment.m_length;
                m_length += m_smoothedPath.back().m_length;
            }

            // Add last segment
            //-------------------------------------------------------------------------

            EE_ASSERT( m_smoothedPath.back().m_isBezier );
            Segment& lastSegment = m_smoothedPath.emplace_back( m_smoothedPath.back().m_end, m_end );
            Segment const& secondToLastSegment = *( m_smoothedPath.end() - 2 );
            lastSegment.m_distanceAlongPath = secondToLastSegment.m_distanceAlongPath + secondToLastSegment.m_length;
            m_length += m_smoothedPath.back().m_length;
        }

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        m_debugPoints.clear();
        m_debugPoints.insert( m_debugPoints.end(), points.begin(), points.end() );

        float debugLength = 0;
        for ( int32_t i = 0; i < m_smoothedPath.size(); i++ )
        {
            debugLength += m_smoothedPath[i].m_length;
        }
        EE_ASSERT( debugLength == m_length );
        #endif
    }

    #if EE_DEVELOPMENT_TOOLS
    void Path::DrawDebug( DebugDrawContext& ctx )
    {
        DrawRawPath( ctx );
        DrawSmoothPathBuildInfo( ctx );
        DrawSmoothPath( ctx );
    }

    void Path::DrawRawPath( DebugDrawContext& ctx )
    {
        if ( !IsValid() )
        {
            return;
        }

        static Color const segmentColors[2] = { Colors::Blue, Colors::LightBlue };

        int32_t const numSegments = m_pathPtr.GetNumSegments();
        for ( int32_t i = 0; i < numSegments; i++ )
        {
            bfx::SegmentType segmentType = m_pathPtr.GetSegmentType( i );
            if ( segmentType == bfx::LINK_SEGMENT )
            {
                bfx::LinkSegment* pSegment = m_pathPtr.GetLinkSegment( i );
                ctx.DrawLine( FromBfx( pSegment->GetStartPos() ), FromBfx( pSegment->GetEndPos() ), Colors::HotPink, 1.0f );
            }
            else // SURFACE_SEGMENT
            {
                bfx::SurfaceSegment* pSegment = m_pathPtr.GetSurfaceSegment( i );
                ctx.DrawLine( FromBfx( pSegment->GetStartPos() ), FromBfx( pSegment->GetEndPos() ), segmentColors[i % 2], 1.0f );
            }
        }
    }

    void Path::DrawSmoothPathBuildInfo( DebugDrawContext& ctx )
    {
        int32_t numPoints = (int32_t) m_debugPoints.size();
        if ( numPoints == 0 )
        {
            return;
        }

        for ( int32_t c = 0; c < numPoints; c++ )
        {
            Point const& point = m_debugPoints[c];
            Vector op = FromBfx( point.m_originalPoint );
            Vector pn = FromBfx( point.m_normal );
            Vector pp = FromBfx( point.m_pushedPoint );

            ctx.DrawPoint( op, Colors::Red, 10.0f );
            ctx.DrawText3D( pp, InlineString( InlineString::CtorSprintf(), "%d", c ).c_str(), Colors::Cyan );

            if ( !FromBfx( point.m_normal ).IsNearZero3() )
            {
                ctx.DrawArrow( op, op + ( pn * 0.5f ), Colors::Yellow, 1.0f );
                ctx.DrawPoint( pp, Colors::White, 10.0f );
            }

            if ( c > 0 )
            {
                ctx.DrawLine( FromBfx( m_debugPoints[c - 1].m_pushedPoint ), pp, Colors::Yellow, 1.0f );
            }
        }
    }

    void Path::DrawSmoothPath( DebugDrawContext& ctx )
    {
        Color const segmentColors[2] = { Colors::Red, Colors::Orange };

        float zOffset = 0.1f;
        Vector const offset( 0, 0, zOffset );
        int32_t color = 0;
        for ( Segment const& segment : m_smoothedPath )
        {
            if ( segment.m_isBezier )
            {
                segment.m_polyline.Draw( ctx, segmentColors[color], 2.0f, zOffset );
            }
            else
            {
                ctx.DrawLine( segment.m_start + offset, segment.m_end + offset, segmentColors[color], 2.0f );
            }

            color = ( ++color ) % 2;
        }
    }
    #endif

    //-------------------------------------------------------------------------

    void PathFollower::FollowPath( Path const* pPath, float startDistanceAlongPath )
    {
        EE_ASSERT( pPath != nullptr && pPath->IsValid() );
        m_pPath = pPath;

        if ( m_pPath->GetLength() > 0 )
        {
            m_currentPosition = pPath->GetStartPoint();
            m_currentDirection = pPath->m_smoothedPath[0].m_startTangent;
            m_pathSegmentIndex = 0;
            m_curveSegmentIndex = InvalidIndex;
            m_distanceTravelledAlongSegment = 0.0f;
            m_distanceTravelled = 0.0f;

            if ( startDistanceAlongPath > 0.0f )
            {
                MoveAlongPath( startDistanceAlongPath );
            }
        }
        else
        {
            m_currentPosition = pPath->GetEndPoint();
            m_currentDirection = Vector::WorldForward;
            m_pathSegmentIndex = InvalidIndex;
            m_curveSegmentIndex = InvalidIndex;
            m_distanceTravelledAlongSegment = 0.0f;
            m_distanceTravelled = 0.0f;
        }
    }

    bool PathFollower::MoveAlongPath( float const distanceToTravel )
    {
        EE_ASSERT( IsValid() );
        EE_ASSERT( distanceToTravel >= 0.0f );

        float const desiredDistanceAlongPath = Math::Min( m_distanceTravelled + distanceToTravel, m_pPath->m_length );
        if ( desiredDistanceAlongPath == m_pPath->m_length )
        {
            m_currentPosition = m_pPath->GetEndPoint();
            m_currentDirection = m_pPath->m_smoothedPath.back().m_endTangent;
            m_distanceTravelled = m_pPath->m_length;
            m_pathSegmentIndex = InvalidIndex;
            m_curveSegmentIndex = InvalidIndex;
            return true;
        }

        // Are we still on the same segment
        //-------------------------------------------------------------------------

        {
            Path::Segment const* pSegment = &m_pPath->m_smoothedPath[m_pathSegmentIndex];
            if ( ( pSegment->m_distanceAlongPath + pSegment->m_length ) > desiredDistanceAlongPath )
            {
                MoveAlongSegment( pSegment, distanceToTravel );
                return IsAtTheEnd();
            }
        }

        // Need to switch segments
        //-------------------------------------------------------------------------

        {
            // Find the curve index that contains the desired travel distance
            int32_t const numSegments = m_pPath->GetNumSegments();
            for ( m_pathSegmentIndex; m_pathSegmentIndex < numSegments; m_pathSegmentIndex++ )
            {
                Path::Segment const* pNextSegment = &m_pPath->m_smoothedPath[m_pathSegmentIndex];
                if ( desiredDistanceAlongPath <= (pNextSegment->m_distanceAlongPath + pNextSegment->m_length ) )
                {
                    break;
                }
            }

            Path::Segment const* pSegment = &m_pPath->m_smoothedPath[m_pathSegmentIndex];
            m_curveSegmentIndex = pSegment->m_isBezier ? 0 : InvalidIndex;
            m_distanceTravelledAlongSegment = 0;

            m_distanceTravelled = m_pPath->m_smoothedPath[m_pathSegmentIndex].m_distanceAlongPath;
            float const remainingDistanceToTravel = desiredDistanceAlongPath - m_distanceTravelled;
            MoveAlongSegment( pSegment, remainingDistanceToTravel );
        }

        //-------------------------------------------------------------------------

        return IsAtTheEnd();
    }

    void PathFollower::MoveAlongSegment( Path::Segment const* pSegment, float distanceToTravel )
    {
        EE_ASSERT( pSegment != nullptr );
        EE_ASSERT( !pSegment->m_isBezier || m_curveSegmentIndex != InvalidIndex );
        EE_ASSERT( distanceToTravel >= 0.0f );
        EE_ASSERT( ( m_distanceTravelledAlongSegment + distanceToTravel ) <= pSegment->m_length );

        if ( pSegment->m_isBezier )
        {
            int32_t const numCurveSegments = pSegment->m_polyline.GetNumSegments();
            EE_ASSERT( m_curveSegmentIndex < numCurveSegments );

            float const desiredDistanceAlongCurve = ( m_distanceTravelledAlongSegment + distanceToTravel );

            // Find the curve index that contains the desired travel distance
            for ( m_curveSegmentIndex; m_curveSegmentIndex < numCurveSegments; m_curveSegmentIndex++ )
            {
                Math::Polyline::Segment const* pNextCurveSegment = &pSegment->m_polyline.m_segments[m_curveSegmentIndex];
                if ( desiredDistanceAlongCurve <= ( pNextCurveSegment->m_distanceAlongCurve + pNextCurveSegment->m_length ) )
                {
                    break;
                }
            }

            auto const& curveSegment = pSegment->m_polyline.m_segments[m_curveSegmentIndex];
            float const T = ( desiredDistanceAlongCurve - curveSegment.m_distanceAlongCurve ) / curveSegment.m_length;
            EE_ASSERT( T >= 0.f && T <= 1.0f );
            m_currentPosition = Vector::Lerp( curveSegment.m_start, curveSegment.m_end, T );
            m_currentDirection = Vector::Lerp( curveSegment.m_previousSegmentDirection, curveSegment.m_direction, T );
            m_distanceTravelledAlongSegment = desiredDistanceAlongCurve;
        }
        else // Line segment
        {
            m_distanceTravelledAlongSegment += distanceToTravel;
            float const T = m_distanceTravelledAlongSegment / pSegment->m_length;
            EE_ASSERT( T >= 0.f && T <= 1.0f );
            m_currentPosition = Vector::Lerp( pSegment->m_start, pSegment->m_end, T );
            m_currentDirection = pSegment->m_startTangent;
        }

        m_distanceTravelled = Math::Min( m_distanceTravelled + distanceToTravel, m_pPath->m_length );
    }
}
#endif