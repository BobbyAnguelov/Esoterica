#pragma once
#include "Base/Math/Vector.h"

//-------------------------------------------------------------------------

namespace EE::Math
{
    class Triangle
    {

    public:

        explicit Triangle( Vector v0, Vector v1, Vector v2 )
            : m_vert0( v0 )
            , m_vert1( v1 )
            , m_vert2( v2 )
        {}

        explicit Triangle( Float3 v0, Float3 v1, Float3 v2 )
            : m_vert0( v0 )
            , m_vert1( v1 )
            , m_vert2( v2 )
        {}

        explicit Triangle( Float4 v0, Float4 v1, Float4 v2 )
            : m_vert0( v0 )
            , m_vert1( v1 )
            , m_vert2( v2 )
        {}

        EE_FORCE_INLINE float GetArea() const
        {
            Vector const V1V0 = m_vert1 - m_vert0;
            Vector const V2V0 = m_vert2 - m_vert0;
            Vector const area = Vector::Cross3( V1V0, V2V0 ).Length3() * Vector::Half;
            return area.GetX();
        }

        EE_FORCE_INLINE bool IsDegenerate() const
        {
            return Math::IsNearZero( GetArea() );
        }

        EE_FORCE_INLINE Vector GetPerimeter() const
        {
            Vector const V1V0 = m_vert1 - m_vert0;
            Vector const V2V0 = m_vert2 - m_vert0;
            Vector const V2V1 = m_vert2 - m_vert1;
            return V1V0.Length3() + V2V0.Length3() + V2V1.Length3();
        }

        EE_FORCE_INLINE Vector GetCentroid() const
        {
            static Vector const vOneDivThree( 1.0f / 3 );
            return ( m_vert0 + m_vert1 + m_vert2 ) * vOneDivThree;
        }

        EE_FORCE_INLINE Vector GetIncenter() const
        {
            Vector const A = m_vert1 - m_vert0;
            Vector const B = m_vert2 - m_vert0;
            Vector const C = m_vert2 - m_vert1;

            Vector const lengthA = A.Length3();
            Vector const lengthB = B.Length3();
            Vector const lengthC = C.Length3();

            Vector const perimeter = lengthA + lengthB + lengthC;
            Vector const sideLengths( A.GetX(), B.GetX(), C.GetX(), 0 );

            Vector const X = Vector::Dot3( Vector( m_vert0.GetX(), m_vert1.GetX(), m_vert2.GetX() ), sideLengths ) / perimeter;
            Vector const Y = Vector::Dot3( Vector( m_vert0.GetY(), m_vert1.GetY(), m_vert2.GetY() ), sideLengths ) / perimeter;
            Vector const Z = Vector::Dot3( Vector( m_vert0.GetZ(), m_vert1.GetZ(), m_vert2.GetZ() ), sideLengths ) / perimeter;

            Vector const inCenter = Vector( X.GetX(), Y.GetY(), Z.GetZ(), 0 );
            return inCenter;
        }

        EE_FORCE_INLINE Vector GetCircumcenter() const
        {
            EE_UNIMPLEMENTED_FUNCTION();
            return Vector();
        }

        EE_FORCE_INLINE Vector GetOrthocenter() const
        {
            EE_UNIMPLEMENTED_FUNCTION();
            return Vector();
        }

    private:

        Vector m_vert0;
        Vector m_vert1;
        Vector m_vert2;
    };
}