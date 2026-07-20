#pragma once

#include "Base/Math/Matrix.h"
#include "Base/Types/Percentage.h"

//-------------------------------------------------------------------------
// Line Helpers
//-------------------------------------------------------------------------

namespace EE
{
    namespace Math { struct Intersect; }

    // Line - an infinite line defined by a start point and direction
    //-------------------------------------------------------------------------

    class Line
    {
        friend Math::Intersect;

    public:

        enum CtorStartEnd
        {
            StartEnd,
        };

        enum CtorStartDirection
        {
            StartDirection,
        };

    public:

        Line( CtorStartDirection, Vector const& startPoint, Vector const& direction )
            : m_startPoint( startPoint )
            , m_direction( direction )
        {
            EE_ASSERT( m_direction.IsNormalized3() );
        }

        Line( CtorStartEnd, Vector const& startPoint, Vector const& endPoint )
            : m_startPoint( startPoint )
            , m_direction( ( endPoint - startPoint ).GetNormalized3() )
        {
            EE_ASSERT( m_direction.IsNormalized3() );
        }

        inline Vector GetStartPoint() const { return m_startPoint; }

        inline Vector GetDirection() const { return m_direction; }

        //-------------------------------------------------------------------------

        // Get the distance along the line from the start point for the projected closest point
        inline float GetDistanceAlongLineToProjectedPoint( Vector const& point ) const
        {
            auto const dot = Vector::Dot3( ( point - m_startPoint ), m_direction );
            return dot.ToFloat();
        }

        // Get a point on the line at the specified distance from the start
        inline Vector GetPointAtDistance( float distanceFromStartPoint ) const 
        { 
            return Vector::MultiplyAdd( m_direction, Vector( distanceFromStartPoint ), m_startPoint );
        }

        // Get the closest point on a line to a given point
        inline Vector GetClosestPoint( Vector const& point, float* pOutDistanceAlongPath = nullptr ) const
        {
            float const distanceAlongPath = GetDistanceAlongLineToProjectedPoint( point );
            if ( pOutDistanceAlongPath != nullptr ) { *pOutDistanceAlongPath = distanceAlongPath; }
            return GetPointAtDistance( distanceAlongPath );
        }

        // Get the distance between a line and a given point
        inline float GetDistanceBetweenLineAndPoint( Vector const& point, Vector* pOutClosestPoint = nullptr ) const
        {
            Vector const closestPointOnLine = GetClosestPoint( point );
            if ( pOutClosestPoint != nullptr ) { *pOutClosestPoint = closestPointOnLine; }
            return closestPointOnLine.GetDistance3( point );
        }

    protected:

        Line() = default;

    protected:

        Vector m_startPoint;
        Vector m_direction;
    };

    // Line Segment - a line with a fixed start and end points
    //-------------------------------------------------------------------------

    class EE_BASE_API LineSegment
    {
        friend struct Intersect;

    public:

        // Get the closest points between two line segments and optionally the squared distance between them
        static void GetClosestPointsBetweenSegments( LineSegment const& p, LineSegment const& q, float& tP, Vector& closestP, float& tQ, Vector& closestQ, float* pSquaredDistanceBetweenPoints = nullptr );

    public:

        LineSegment( Vector startPoint, Vector endPoint )
            : m_startPoint( startPoint )
            , m_endPoint( endPoint )
        {
            float length;
            ( endPoint - startPoint ).ToDirectionAndLength3( m_direction, length );
            m_length = Vector( length );
        }

        inline Vector GetStartPoint() const { return m_startPoint; }

        inline Vector GetEndPoint() const { return m_endPoint; }

        inline Vector GetDirection() const { return m_direction; }

        inline float GetLength() const { return m_length.ToFloat(); }

        //-------------------------------------------------------------------------

        // Get the distance along the segment starting at the start point to the projected point
        inline float GetDistanceAlongSegmentToProjectedPoint( Vector const& point ) const
        {
            auto const dot = Vector::Dot3( ( point - m_startPoint ), m_direction );
            float distance = Math::Clamp( dot.ToFloat(), 0.0f, GetLength() );
            return distance;
        }

        // The scalar project aka distance along the segment (clamped between start and end point)
        inline Percentage CalculatePercentageAlongSegmentToProjectedPoint( Vector const& point ) const
        {
            float T = GetDistanceAlongSegmentToProjectedPoint( point ) / GetLength();
            return T;
        }

        // Returns a point on the segment at the desired percentage between start and end points
        inline Vector GetPointAtDistance( float distanceAlongSegment ) const
        {
            Vector const distance( Math::Clamp( distanceAlongSegment, 0.0f, m_length.ToFloat() ) );
            return Vector::MultiplyAdd( m_direction, distance, m_startPoint );
        }

        // Returns a point on the segment at the desired percentage between start and end points
        inline Vector GetPointAtPercentage( Percentage percentageAlongSegment ) const
        {
            Vector const distance = m_length * Math::Clamp( percentageAlongSegment.ToFloat(), 0.0f, 1.0f );
            return Vector::MultiplyAdd( m_direction, distance, m_startPoint );
        }

        inline Vector GetClosestPoint( Vector const& point, Percentage* pOutPercentage = nullptr ) const
        {
            Percentage T = CalculatePercentageAlongSegmentToProjectedPoint( point );
            if ( pOutPercentage != nullptr ) { *pOutPercentage = T; }
            return GetPointAtPercentage( T );
        }

        // Returns the shortest distance between the segment and the specified point
        inline float GetDistanceToPoint( Vector const& point, Vector* pOutClosestPoint = nullptr ) const
        {
            Vector const closestPointOnSegment = GetClosestPoint( point );
            if ( pOutClosestPoint != nullptr ) { *pOutClosestPoint = closestPointOnSegment; }
            return closestPointOnSegment.GetDistance3( point );
        }

        //-------------------------------------------------------------------------

        inline LineSegment& Transform( Matrix const& transform )
        {
            m_startPoint = transform.TransformPoint( m_startPoint );
            m_endPoint = transform.TransformPoint( m_endPoint );
            m_direction = ( m_endPoint - m_startPoint ).GetNormalized3();
            m_length = Vector( m_startPoint.GetDistance3( m_endPoint ) );
            return *this;
        }

        inline LineSegment GetTransformed( Matrix const& transform ) const
        {
            LineSegment line = *this;
            line.Transform( transform );
            return line;
        }

    private:

        LineSegment() = default;

    private:

        Vector m_startPoint;
        Vector m_endPoint;
        Vector m_direction;
        Vector m_length;
    };

    // Ray - a line that is infinite in one direction
    //-------------------------------------------------------------------------

    class Ray
    {
        friend struct Intersect;

    public:

        enum CtorStartEnd
        {
            StartEnd,
        };

        enum CtorStartDirection
        {
            StartDirection,
        };

    public:

        Ray( Vector const& startPoint, Vector const& direction )
        { 
            EE_ASSERT( direction.IsNormalized3() );
            m_startPoint = startPoint;
            m_direction = direction;
        }

        Ray( CtorStartDirection, Vector const& startPoint, Vector const& direction )
            : Ray( startPoint, direction )
        {}

        Ray( CtorStartEnd, Vector const& startPoint, Vector const& endPoint )
            : m_startPoint( startPoint )
        {
            EE_ASSERT( !startPoint.IsNearEqual3( endPoint ) );

            float length;
            ( endPoint - startPoint ).ToDirectionAndLength3( m_direction, length );
        }

        Ray( Line const& line )
        {
            m_startPoint = line.GetStartPoint();
            m_direction = line.GetDirection();
        }

        Ray( LineSegment const& lineSegment )
        {
            m_startPoint = lineSegment.GetStartPoint();
            m_direction = lineSegment.GetDirection();
        }

        inline Vector GetStartPoint() const { return m_startPoint; }

        inline Vector GetDirection() const { return m_direction; }

        //-------------------------------------------------------------------------

        // The scalar project aka distance along the ray (all points behind the ray are clamped to 0)
        inline float CalculateDistanceAlongRayToProjectedPoint( Vector const point ) const
        {
            auto const dot = Vector::Dot3( ( point - m_startPoint ), m_direction );
            float const distance = Math::Max( 0.f, dot.ToFloat() );
            return distance;
        }

        inline Vector GetPointAtDistance( float distanceFromStartPoint ) const 
        { 
            EE_ASSERT( distanceFromStartPoint >= 0.0f );
            return Vector::MultiplyAdd( m_direction, Vector( distanceFromStartPoint ), m_startPoint );
        }

        inline Vector GetClosestPoint( Vector const& point, Percentage* pOutPercentage = nullptr ) const
        {
            float T = CalculateDistanceAlongRayToProjectedPoint( point );
            if ( pOutPercentage != nullptr ) { *pOutPercentage = T; }
            return GetPointAtDistance( T );
        }

        // Returns the shortest distance between the ray and the specified point
        inline float GetDistanceToPoint( Vector const& point, Vector* pOutClosestPoint = nullptr ) const
        {
            Vector const closestPoint = GetClosestPoint( point );
            if ( pOutClosestPoint != nullptr ) { *pOutClosestPoint = closestPoint; }
            return closestPoint.GetDistance3( point );
        }

        //-------------------------------------------------------------------------

        inline Ray& Transform( Matrix const& transform )
        {
            m_startPoint = transform.TransformPoint( m_startPoint );
            m_direction = transform.RotateVector( m_direction );
            return *this;
        }

        inline Ray GetTransformed( Matrix const& transform ) const
        {
            Ray ray = *this;
            ray.Transform( transform );
            return ray;
        }

        //-------------------------------------------------------------------------

        inline LineSegment ToLineSegment( float length ) const
        { 
            return LineSegment( m_startPoint, GetPointAtDistance( length ) );
        }

    private:

        Vector m_startPoint;
        Vector m_direction;
    };
}