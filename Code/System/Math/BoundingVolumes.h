#pragma once

#include "Transform.h"

//-------------------------------------------------------------------------

namespace EE
{
    enum class OverlapResult
    {
        NoOverlap,
        Overlap,
        FullyEnclosed,
    };

    //-------------------------------------------------------------------------

    struct AABB;
    struct OBB;

    //-------------------------------------------------------------------------
    // Axis Aligned Bounding Box
    //-------------------------------------------------------------------------

    struct EE_SYSTEM_API AABB
    {
        EE_SERIALIZE( m_center, m_halfExtents );

    public:

        inline static AABB FromMinMax( Vector const& min, Vector const& max )
        {
            EE_ASSERT( min.IsLessThanEqual3( max ) );
            Vector const extents = ( max - min ) * Vector::Half;
            Vector const center = min + extents;
            return AABB( center, extents );
        }

        // Get the combined box of two AABBs
        inline static AABB GetCombinedBox( AABB const& a, AABB const& b )
        {
            EE_ASSERT( a.IsValid() && b.IsValid() );

            Vector const newMin = Vector::Min( a.GetMin(), b.GetMin() );
            Vector const newMax = Vector::Max( a.GetMax(), b.GetMax() );
            return AABB::FromMinMax( newMin, newMax );
        }

        // Get the intersection of two AABBs
        inline static AABB GetIntersectionBox( AABB const& a, AABB const& b )
        {
            EE_ASSERT( a.IsValid() && b.IsValid() );

            if ( !a.Overlaps( b ) )
            {
                return AABB();
            }

            Vector const newMin = Vector::Max( a.GetMin(), b.GetMin() );
            Vector const newMax = Vector::Min( a.GetMax(), b.GetMax() );
            return AABB::FromMinMax( newMin, newMax );
        }

    public:

        AABB() = default;
        AABB( Vector const* pPoints, uint32_t numPoints );
        AABB( Vector const& center ) : m_center( center ), m_halfExtents( Vector::Zero ) {}
        AABB( Vector const& center, Vector const& extents ) : m_center( center ), m_halfExtents( extents ) { EE_ASSERT( IsValid() ); }
        AABB( Vector const& center, float const& extents ) : m_center( center ), m_halfExtents( extents ) { EE_ASSERT( IsValid() ); }
        explicit AABB( OBB const& box );

        inline bool IsValid() const { return m_halfExtents.IsGreaterThanEqual3( Vector::Zero ); }
        inline void Reset() { m_center = Vector::Zero; m_halfExtents = Vector::NegativeOne; }
        inline void Reset( Float3 center ) { m_center = center; m_halfExtents = Vector::Zero; }

        EE_FORCE_INLINE Vector const& GetCenter() const { return m_center; }
        EE_FORCE_INLINE Vector const& GetExtents() const { return m_halfExtents; }
        EE_FORCE_INLINE Vector GetMin() const { return m_center - m_halfExtents; }
        EE_FORCE_INLINE Vector GetMax() const { return m_center + m_halfExtents; }

        EE_FORCE_INLINE void GetCorners( Vector corners[8] ) const;
        EE_FORCE_INLINE float GetVolume() const;

        //-------------------------------------------------------------------------

        EE_FORCE_INLINE void AddPoint( Vector const& point )
        {
            Vector const min = Vector::Min( GetMin(), point );
            Vector const max = Vector::Max( GetMax(), point );
            SetFromMinMax( min, max );
        }

        EE_FORCE_INLINE void SetFromMinMax( Vector const& min, Vector const& max )
        {
            EE_ASSERT( min.IsLessThanEqual3( max ) );
            m_halfExtents = ( max - min ) * Vector::Half;
            m_center = min + m_halfExtents;
        }

        EE_FORCE_INLINE void SetCenter( Vector const& newCenter ) { m_center = newCenter; }

        // Move the AABB by the amount in deltaVector
        EE_FORCE_INLINE void Translate( Vector const& deltaVector ) { m_center += deltaVector; }

        // Applies the given transform to the current AABB corners and calculates the new AABB for the transformed box
        void ApplyTransform( Transform const& transform );

        EE_FORCE_INLINE AABB GetTransformed( Transform const& transform ) const
        {
            AABB result = *this;
            result.ApplyTransform( transform );
            return result;
        }

        EE_FORCE_INLINE void Shrink( Vector const& shrinkSize )
        {
            m_halfExtents -= shrinkSize;
        }

        EE_FORCE_INLINE void Grow( Vector const& growSize )
        {
            m_halfExtents += growSize;
        }

        // Queries
        //-------------------------------------------------------------------------

        EE_FORCE_INLINE bool ContainsPoint( Vector const& point ) const { return ( point - m_center ).Abs().IsLessThanEqual3( m_halfExtents ); }

        OverlapResult OverlapTest( AABB const& other ) const;
        OverlapResult OverlapTest( OBB const& box ) const;
        
        EE_FORCE_INLINE bool Overlaps( AABB const& other ) const;
        EE_FORCE_INLINE bool Overlaps( OBB const& box ) const;

        public:

        Vector          m_center = Vector::UnitW;
        Vector          m_halfExtents = Vector::NegativeOne;
    };

    //-------------------------------------------------------------------------
    // Oriented Bounding Box
    //-------------------------------------------------------------------------

    struct EE_SYSTEM_API OBB
    {
        EE_SERIALIZE( m_center, m_extents, m_orientation );

        OBB() = default;
        OBB( Vector center, Vector extents, Quaternion orientation = Quaternion::Identity );
        OBB( Vector const* pPoints, uint32_t numPoints );
        explicit OBB( AABB const& aabb );
        explicit OBB( AABB const& aabb, Transform const& transform );

        //-------------------------------------------------------------------------

        inline bool IsValid() const { return m_extents.IsGreaterThanEqual3( Vector::Zero ); }

        EE_FORCE_INLINE void Reset() { m_orientation = Quaternion::Identity; m_center = m_extents = Vector::Zero; }

        EE_FORCE_INLINE void GetCorners( Vector corners[8] ) const
        {
            for ( int32_t i = 0; i < 8; ++i )
            {
                corners[i] = m_center + m_orientation.RotateVector( m_extents * Vector::BoxCorners[i] );
            }
        }

        EE_FORCE_INLINE AABB GetAABB() const;

        // Positioning
        //-------------------------------------------------------------------------

        EE_FORCE_INLINE void SetCenter( Vector const& newCenter ) { m_center = newCenter; }

        EE_FORCE_INLINE void SetOrientation( Quaternion const& newOrientation ) { m_orientation = newOrientation; }

        EE_FORCE_INLINE void Translate( Vector const& deltaVector ) { m_center += deltaVector; }

        EE_FORCE_INLINE void Rotate( Quaternion const& deltaRotation ) { m_orientation = deltaRotation * m_orientation; }

        void ApplyTransform( Transform const& transform );

        void ApplyScale( Vector const& scale );

        EE_FORCE_INLINE OBB GetTransformed( Transform const& transform ) const
        {
            OBB result = *this;
            result.ApplyTransform( transform );
            return result;
        }

        // Queries
        //-------------------------------------------------------------------------

        EE_FORCE_INLINE bool ContainsPoint( Vector const& point ) const;

        //-------------------------------------------------------------------------

        EE_FORCE_INLINE OverlapResult OverlapTest( OBB const& other ) const;
        bool Overlaps( OBB const& box ) const; // Slightly faster than the full overlap test

        //-------------------------------------------------------------------------

        EE_FORCE_INLINE OverlapResult OverlapTest( AABB const& aabb ) const { return OverlapTest( OBB( aabb ) ); }
        EE_FORCE_INLINE bool Overlaps( AABB const& aabb ) const { return OverlapTest( OBB( aabb ) ) != OverlapResult::NoOverlap; }

    public:

        Quaternion  m_orientation = Quaternion::Identity;
        Vector      m_center = Vector::UnitW;
        Vector      m_extents = Vector::Zero;
    };

    //-------------------------------------------------------------------------
    // INLINE IMPLEMENTATIONS
    //-------------------------------------------------------------------------

    EE_FORCE_INLINE void AABB::GetCorners( Vector corners[8] ) const
    {
        for ( int32_t i = 0; i < 8; ++i )
        {
            corners[i] = Vector::MultiplyAdd( m_halfExtents, Vector::BoxCorners[i], m_center );
        }
    }

    EE_FORCE_INLINE float AABB::GetVolume() const
    {
        Vector const fullExtents = ( m_halfExtents * 2 );
        return fullExtents.GetX() * m_halfExtents.GetY() * m_halfExtents.GetZ();
    }

    // Full overlap test
    EE_FORCE_INLINE OverlapResult AABB::OverlapTest( AABB const& other ) const
    {
        Vector const MinA = GetMin();
        Vector const MaxA = GetMax();
        Vector const MinB = other.GetMin();
        Vector const MaxB = other.GetMax();

        if ( MinA.IsGreaterThan3( MaxB ) || MinB.IsGreaterThan3( MaxA ) )
        {
            return OverlapResult::NoOverlap;
        }

        if ( MinA.IsLessThanEqual3( MinB ) && MaxB.IsLessThanEqual3( MaxA ) )
        {
            return OverlapResult::FullyEnclosed;
        }

        return OverlapResult::Overlap;
    }

    // Fast overlap test
    EE_FORCE_INLINE bool AABB::Overlaps( AABB const& other ) const
    {
        Vector const deltaCenters = ( m_center - other.m_center ).Abs();
        Vector const sumExtents = m_halfExtents + other.m_halfExtents;
        return deltaCenters.IsLessThanEqual3( sumExtents );
    }

    EE_FORCE_INLINE bool AABB::Overlaps( OBB const& box ) const
    {
        return box.Overlaps( *this );
    }

    //-------------------------------------------------------------------------

    // Optimization from: https://zeux.io/2010/10/17/aabb-from-obb-with-component-wise-abs/
    EE_FORCE_INLINE AABB OBB::GetAABB() const
    {
        Matrix rotationTransform( m_orientation );
        for ( auto i = 0; i < 4; i++ )
        {
            rotationTransform[i] = rotationTransform[i].Abs();
        }

        Vector const newExtent = rotationTransform.RotateVector( m_extents );
        return AABB::FromMinMax( m_center - newExtent, m_center + newExtent );
    }

    EE_FORCE_INLINE bool OBB::ContainsPoint( Vector const& point ) const
    {
        Vector const untransformedPoint = m_orientation.RotateVectorInverse( point - m_center );
        return untransformedPoint.IsInBounds3( m_extents );
    }

    EE_FORCE_INLINE OverlapResult OBB::OverlapTest( OBB const& other ) const
    {
        if ( !Overlaps( other ) )
        {
            return OverlapResult::NoOverlap;
        }

        Vector const offset = other.m_center - m_center;
        for ( int32_t i = 0; i < 8; ++i )
        {
            Vector C = other.m_orientation.RotateVector( other.m_extents * Vector::BoxCorners[i] ) + offset;
            C = m_orientation.RotateVectorInverse( C );
            if ( !C.IsInBounds3( m_extents ) )
            {
                return OverlapResult::Overlap;
            }
        }

        return OverlapResult::FullyEnclosed;
    }
}