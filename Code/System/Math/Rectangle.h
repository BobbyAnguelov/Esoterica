#pragma once
#include "System/_Module/API.h"
#include "System/Math/Vector.h"
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

    class Rectangle
    {

    public:

        Rectangle() = default;

        explicit Rectangle( Float2 topLeft, Float2 size )
            : m_topLeft( topLeft )
            , m_size( size )
        {
            EE_ASSERT( size.m_x >= 0 && size.m_y >= 0 );
        }

        inline Float2 GetTopLeft() const { return m_topLeft; }
        inline Float2 GetBottomRight() const { return m_topLeft + Float2( m_size.m_x, -m_size.m_y ); }
        inline Float2 GetSize() const { return m_size; }

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

        inline Float2 GetClosestPoint( Float2 const& point )
        {
            Float2 const bottomRight = GetBottomRight();

            Float2 closestPoint;
            closestPoint.m_x = Math::Clamp( point.m_x, m_topLeft.m_x, bottomRight.m_x );
            closestPoint.m_y = Math::Clamp( point.m_y, bottomRight.m_y, m_topLeft.m_y );
            return closestPoint;
        }

        inline bool Contains( Float2 const& point ) const
        {
            Float2 const bottomRight = GetBottomRight();
            return Math::IsInRangeInclusive( point.m_x, m_topLeft.m_x, bottomRight.m_x ) && Math::IsInRangeInclusive( point.m_y, bottomRight.m_y, m_topLeft.m_y );
        }

        inline bool Overlaps( Rectangle const& other ) const
        {
            Float2 const bottomRight = GetBottomRight();
            FloatRange rangeX( m_topLeft.m_x, bottomRight.m_x );
            FloatRange rangeY( bottomRight.m_y, m_topLeft.m_y );

            Float2 const otherBottomRight = other.GetBottomRight();
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

    class ScreenSpaceRectangle
    {

    public:

        ScreenSpaceRectangle() = default;

        explicit ScreenSpaceRectangle( Float2 topLeft, Float2 size )
            : m_topLeft( topLeft )
            , m_size( size )
        {
            EE_ASSERT( size.m_x >= 0 && size.m_y >= 0 );
        }

        inline Float2 GetTopLeft() const { return m_topLeft; }
        inline Float2 GetBottomRight() const { return m_topLeft + Float2( m_size.m_x, m_size.m_y ); }
        inline Float2 GetSize() const { return m_size; }

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

        inline Float2 GetClosestPoint( Float2 const& point )
        {
            Float2 const bottomRight = GetBottomRight();

            Float2 closestPoint;
            closestPoint.m_x = Math::Clamp( point.m_x, m_topLeft.m_x, bottomRight.m_x );
            closestPoint.m_y = Math::Clamp( point.m_y, bottomRight.m_y, m_topLeft.m_y );
            return closestPoint;
        }

        inline bool Contains( Float2 const& point ) const
        {
            Float2 const bottomRight = GetBottomRight();
            return Math::IsInRangeInclusive( point.m_x, m_topLeft.m_x, bottomRight.m_x ) && Math::IsInRangeInclusive( point.m_y, m_topLeft.m_y, bottomRight.m_y );
        }

        inline bool Overlaps( ScreenSpaceRectangle const& other ) const
        {
            Float2 const bottomRight = GetBottomRight();
            FloatRange rangeX( m_topLeft.m_x, bottomRight.m_x );
            FloatRange rangeY( m_topLeft.m_y, bottomRight.m_y );

            Float2 const otherBottomRight = other.GetBottomRight();
            FloatRange otherRangeX( other.m_topLeft.m_x, otherBottomRight.m_x );
            FloatRange otherRangeY( other.m_topLeft.m_y, otherBottomRight.m_y );

            return rangeX.Overlaps( otherRangeX ) && rangeY.Overlaps( otherRangeY );
        }

    private:

        Float2      m_topLeft;
        Float2      m_size;
    };
}