#pragma once
#include "Base/_Module/API.h"
#include "Base/Math/Vector.h"
#include "NumericRange.h"

//-------------------------------------------------------------------------

namespace EE::Math
{
    // Simple rectangle helper
    //-------------------------------------------------------------------------
    // Uses Cartesian coordinates so bottom right is always +x and -y from top left
    //
    //  TopLeft (0, 1) *---*
    //                 |   |
    //                 |   |
    //                 *---* Bottom Right( 1, 0 )
    //
    //-------------------------------------------------------------------------

    class EE_BASE_API Rectangle
    {

    public:

        Rectangle() = default;

        explicit Rectangle( Float2 topLeft, Float2 size )
            : m_topLeft( topLeft )
            , m_size( size )
        {
            EE_ASSERT( size.m_x >= 0 && size.m_y >= 0 );
        }

        inline void Reset() { m_topLeft = Float2::Zero; m_size = Float2::Zero; }

        // Get top left point
        inline Float2 GetTL() const { return m_topLeft;}

        // Get top right point
        inline Float2 GetTR() const { return m_topLeft + Float2( m_size.m_x, 0.0f );}

        // Get bottom left point
        inline Float2 GetBL() const { return m_topLeft + Float2( 0.0f, -m_size.m_y );}

        // Get bottom right point
        inline Float2 GetBR() const { return m_topLeft + Float2( m_size.m_x, -m_size.m_y ); }

        // Get size of rect
        inline Float2 GetSize() const { return m_size; }

        // Get are of rect
        inline float GetArea() const
        {
            return m_size.m_x * m_size.m_y;
        }

        inline bool IsDegenerate() const
        {
            return Math::IsNearZero( GetArea() );
        }

        inline Float2 GetCenter() const
        {
            return Float2( m_topLeft.m_x + ( m_size.m_x / 2.0f ), m_topLeft.m_y + ( m_size.m_x / 2.0f ) );
        }

        inline Float2 GetClosestPoint( Float2 const& point ) const
        {
            Float2 const bottomRight = GetBR();

            Float2 closestPoint;
            closestPoint.m_x = Math::Clamp( point.m_x, m_topLeft.m_x, bottomRight.m_x );
            closestPoint.m_y = Math::Clamp( point.m_y, bottomRight.m_y, m_topLeft.m_y );
            return closestPoint;
        }

        Float2 GetClosestPointOnBorder( Float2 const& point ) const;

        inline bool Contains( Float2 const& point ) const
        {
            Float2 const bottomRight = GetBR();
            return Math::IsInRangeInclusive( point.m_x, m_topLeft.m_x, bottomRight.m_x ) && Math::IsInRangeInclusive( point.m_y, bottomRight.m_y, m_topLeft.m_y );
        }

        inline bool Overlaps( Rectangle const& other ) const
        {
            Float2 const bottomRight = GetBR();
            FloatRange rangeX( m_topLeft.m_x, bottomRight.m_x );
            FloatRange rangeY( bottomRight.m_y, m_topLeft.m_y );

            Float2 const otherBottomRight = other.GetBR();
            FloatRange otherRangeX( other.m_topLeft.m_x, otherBottomRight.m_x );
            FloatRange otherRangeY( otherBottomRight.m_y, other.m_topLeft.m_y );

            return rangeX.Overlaps( otherRangeX ) && rangeY.Overlaps( otherRangeY );
        }

    private:

        Float2      m_topLeft;
        Float2      m_size;
    };

    //-------------------------------------------------------------------------
    // Screen space rectangle helper
    //-------------------------------------------------------------------------
    // Uses screen coordinates so bottom right is always +x and +y from top left
    //
    //  TopLeft (0, 0) *---*
    //                 |   |
    //                 |   |
    //                 *---* Bottom Right( 1, 1 )
    //
    //-------------------------------------------------------------------------

    class EE_BASE_API ScreenSpaceRectangle
    {

    public:

        ScreenSpaceRectangle() = default;

        explicit ScreenSpaceRectangle( Float2 topLeft, Float2 size )
            : m_topLeft( topLeft )
            , m_size( size )
        {
            EE_ASSERT( size.m_x >= 0 && size.m_y >= 0 );
        }

        inline void Reset() { m_topLeft = Float2::Zero; m_size = Float2::Zero; }

        // Get top left point
        inline Float2 GetTL() const { return m_topLeft; }

        // Get top right point
        inline Float2 GetTR() const { return m_topLeft + Float2( m_size.m_x, 0.0f ); }

        // Get bottom left point
        inline Float2 GetBL() const { return m_topLeft + Float2( 0.0f, m_size.m_y ); }

        // Get bottom right point
        inline Float2 GetBR() const { return m_topLeft + Float2( m_size.m_x, m_size.m_y ); }

        // Get size of rect
        inline Float2 GetSize() const { return m_size; }

        // Get are of rect
        inline float GetArea() const
        {
            return m_size.m_x * m_size.m_y;
        }

        inline bool IsDegenerate() const
        {
            return Math::IsNearZero( GetArea() );
        }

        inline Float2 GetCenter() const
        {
            return Float2( m_topLeft.m_x + ( m_size.m_x / 2.0f ), m_topLeft.m_y + ( m_size.m_x / 2.0f ) );
        }

        inline Float2 GetClosestPoint( Float2 const& point ) const
        {
            Float2 const bottomRight = GetBR();

            Float2 closestPoint;
            closestPoint.m_x = Math::Clamp( point.m_x, m_topLeft.m_x, bottomRight.m_x );
            closestPoint.m_y = Math::Clamp( point.m_y, bottomRight.m_y, m_topLeft.m_y );
            return closestPoint;
        }

        Float2 GetClosestPointOnBorder( Float2 const& point ) const;

        inline bool Contains( Float2 const& point ) const
        {
            Float2 const bottomRight = GetBR();
            return Math::IsInRangeInclusive( point.m_x, m_topLeft.m_x, bottomRight.m_x ) && Math::IsInRangeInclusive( point.m_y, m_topLeft.m_y, bottomRight.m_y );
        }

        inline bool Overlaps( ScreenSpaceRectangle const& other ) const
        {
            Float2 const bottomRight = GetBR();
            FloatRange rangeX( m_topLeft.m_x, bottomRight.m_x );
            FloatRange rangeY( m_topLeft.m_y, bottomRight.m_y );

            Float2 const otherBottomRight = other.GetBR();
            FloatRange otherRangeX( other.m_topLeft.m_x, otherBottomRight.m_x );
            FloatRange otherRangeY( other.m_topLeft.m_y, otherBottomRight.m_y );

            return rangeX.Overlaps( otherRangeX ) && rangeY.Overlaps( otherRangeY );
        }

    private:

        Float2      m_topLeft;
        Float2      m_size;
    };
}