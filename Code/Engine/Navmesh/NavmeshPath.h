#pragma once
#include "Engine/_Module/API.h"
#include "Base/Math/Curves.h"
#include "Base/Math/Vector.h"

#if EE_ENABLE_NAVPOWER
#include "Engine/Navmesh/Navpower.h"

//-------------------------------------------------------------------------

namespace EE
{
    class DebugDrawContext;
}

//-------------------------------------------------------------------------

namespace EE::Navmesh
{
    class EE_ENGINE_API Path
    {
        friend class PathFollower;

        inline constinit static float const s_pushAwayReductionFactor = 2.0f / 3.0f;
        inline constinit static float const s_angleThresholdRadians = 10 * Math::DegreesToRadians;
        inline constinit static float const s_pushAwayDistance = 0.5f;

        struct Segment
        {
            constinit static int32_t const s_polylineSubdivisions = 20;

        public:
            Segment() = default;
            Segment( Vector const& start, Vector const& end );
            Segment( Vector const& start, Vector const& end, Vector  const& cp0, Vector const& cp1 );

        public:

            Vector                      m_start;
            Vector                      m_end;
            Vector                      m_startTangent;
            Vector                      m_endTangent;
            Vector                      m_cp0;
            Vector                      m_cp1;
            Math::Polyline              m_polyline;
            float                       m_length = 0.0f;
            float                       m_distanceAlongPath = 0.0f;
            bool                        m_isBezier = false;
        };

        // Intermediate point, used for smoothing
        struct Point
        {
            bfx::Vector3                m_originalPoint;
            bfx::AreaHandle             m_originalPointArea;
            bfx::Vector3                m_normal;
            bfx::Vector3                m_pushedPoint;
            bfx::AreaHandle             m_pushedPointArea;
            float                       m_pushDistance = 0.0f;
        };

    public:

        Path() = default;
        Path( bfx::SpaceHandle& space, Vector const& start, Vector const& end );

        inline bool IsValid() const { return m_pathPtr.IsValid(); }

        inline Vector GetStartPoint() const { return m_start; }
        inline Vector GetEndPoint() const { return m_end; }
        inline float GetLength() const { return m_length; }

        inline int32_t GetNumSegments() const { return int32_t( m_smoothedPath.size() ); }
        inline TVector<Segment> const& GetPathSegments() const { return m_smoothedPath; }

        #if EE_DEVELOPMENT_TOOLS
        void DrawDebug( DebugDrawContext& ctx );
        #endif

    private:

        void PerformSmoothing();

        #if EE_DEVELOPMENT_TOOLS
        void DrawRawPath( DebugDrawContext& ctx );
        void DrawSmoothPathBuildInfo( DebugDrawContext& ctx );
        void DrawSmoothPath( DebugDrawContext& ctx );
        #endif

    private:

        bfx::PolylinePathRCPtr          m_pathPtr;
        bfx::SpaceHandle                m_space;
        bfx::PathSpec                   m_pathSpec;
        Vector                          m_start;
        Vector                          m_end;
        TVector<Segment>                m_smoothedPath;
        float                           m_length = 0.0f;

        #if EE_DEVELOPMENT_TOOLS
        TVector<Point>                  m_debugPoints;
        #endif
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API PathFollower
    {
    public:

        PathFollower() = default;
        PathFollower( Path const* pPath ) { FollowPath( pPath ); }

        inline bool IsValid() const { return m_pPath != nullptr && m_pPath->IsValid(); }
        inline void Clear() { *this = PathFollower(); }
        void FollowPath( Path const* pPath, float startDistanceAlongPath = 0.0f );

        inline bool IsAtTheEnd() const { return GetRemainingDistance() == 0.0f; }
        inline float GetTravelledDistance() const { return m_distanceTravelled; }
        inline float GetRemainingDistance() const { EE_ASSERT( IsValid() ); return m_pPath->m_length - m_distanceTravelled; }

        // Move distance along the path, will be clamped to the end. Returns true if we reach the end of the path.
        bool MoveAlongPath( float distanceToTravel );
        Vector GetPosition() const { return m_currentPosition; }
        Vector GetDirection() const { return m_currentDirection; }

    private:

        void MoveAlongSegment( Path::Segment const* pSegment, float distanceToTravel );

    private:

        Path const*     m_pPath = nullptr;
        float           m_distanceTravelled = 0.0f;
        Vector          m_currentPosition = Vector::Zero;
        Vector          m_currentDirection = Vector::WorldForward;
        int32_t         m_pathSegmentIndex = InvalidIndex;
        int32_t         m_curveSegmentIndex = InvalidIndex;
        float           m_distanceTravelledAlongSegment = 0.0f;
    };
}
#endif