#include "BoundingVolumes.h"
#include "EigenVectors.h"

//-------------------------------------------------------------------------

namespace EE
{
    Sphere::Sphere( Vector* points, uint32_t numPoints )
    {
        EE_ASSERT( points != nullptr && numPoints > 0 );

        // Find the points with minimum and maximum m_x, m_y, and m_z
        Vector minX, maxX, minY, maxY, minZ, maxZ;
        minX = maxX = minY = maxY = minZ = maxZ = points[0];

        for ( uint32_t i = 1u; i < numPoints; ++i )
        {
            float px = points[i].m_x;
            float py = points[i].m_y;
            float pz = points[i].m_z;

            if ( px < minX.m_x )
            {
                minX = points[i];
            }

            if ( px > maxX.m_x )
            {
                maxX = points[i];
            }

            if ( py < minY.m_y )
            {
                minY = points[i];
            }

            if ( py > maxY.m_y )
            {
                maxY = points[i];
            }

            if ( pz < minZ.m_z )
            {
                minZ = points[i];
            }

            if ( pz >  maxZ.m_z )
            {
                maxZ = points[i];
            }
        }

        // Use the min/max pair that are farthest apart to form the initial sphere.
        Vector DeltaX = maxX - minX;
        Vector DistX = DeltaX.Length3();

        Vector DeltaY = maxY - minY;
        Vector DistY =  DeltaY.Length3();

        Vector DeltaZ = maxZ - minZ;
        Vector DistZ = DeltaZ.Length3();

        if ( DistX.IsGreaterThan3( DistY ) )
        {
            if ( DistX.IsGreaterThan3( DistZ ) )
            {
                // Use min/max m_x.
                m_center = Vector::Lerp( maxX, minX, 0.5f );
                m_radius = DistX * 0.5f;
            }
            else
            {
                // Use min/max m_z.
                m_center = Vector::Lerp( maxZ, minZ, 0.5f );
                m_radius = DistZ * 0.5f;
            }
        }
        else // Y >= X
        {
            if ( DistY.IsGreaterThan3( DistZ ) )
            {
                // Use min/max m_y.
                m_center = Vector::Lerp( maxY, minY, 0.5f );
                m_radius = DistY * 0.5f;
            }
            else
            {
                // Use min/max m_z.
                m_center = Vector::Lerp( maxZ, minZ, 0.5f );
                m_radius = DistZ, 0.5f;
            }
        }

        // Add any points not inside the sphere.
        for ( uint32_t i = 0u; i < numPoints; ++i )
        {
            Vector Delta = points[i] - m_center;
            Vector Dist =  Delta.Length3();

            if ( Dist.IsGreaterThan3( m_radius ) )
            {
                // Adjust sphere to include the new point.
                m_radius = ( m_radius + Dist ) * 0.5f;
                m_center = m_center + ( ( Vector::One - ( m_radius / Dist ) ) * Delta );
            }
        }
    }

    Sphere::Sphere( AABB const& box )
        : m_center( box.m_center )
        , m_radius( box.m_extents.Length3() )
    {}

    Sphere::Sphere( OBB const& box )
        : m_center( box.m_center )
        , m_radius( box.m_extents.Length3() )
    {}

    //-------------------------------------------------------------------------

    AABB::AABB( OBB const& box )
    {
        //TODO
        EE_UNIMPLEMENTED_FUNCTION();
    }

    AABB::AABB( Vector const* pPoints, uint32_t numPoints )
    {
        EE_ASSERT( pPoints != nullptr && numPoints > 0 );

        Vector min( FLT_MAX );
        Vector max( -FLT_MAX );

        for ( uint32_t i = 0u; i < numPoints; i++ )
        {
            min = Vector::Min( min, pPoints[i] );
            max = Vector::Max( max, pPoints[i] );
        }

        SetFromMinMax( min, max );
    }

    EE::OverlapResult AABB::OverlapTest( Sphere const& sphere ) const
    {
        Vector const boxMin = m_center - m_extents;
        Vector const boxMax = m_center + m_extents;

        // Find the distance to the nearest point on the box.
        // for each i in (m_x, m_y, m_z)
        // if (sphere.m_center(i) < BoxMin(i)) d2 += (sphere.m_center(i) - BoxMin(i)) ^ 2
        // else if (sphere.m_center(i) > BoxMax(i)) d2 += (sphere.m_center(i) - BoxMax(i)) ^ 2

        // Compute d for each dimension.
        Vector const lessThanMin = sphere.m_center.LessThan( boxMin );
        Vector const greaterThanMax = sphere.m_center.GreaterThan( boxMax );
        Vector const minDelta = sphere.m_center - boxMin;
        Vector const maxDelta = sphere.m_center - boxMax;

        // Choose value for each dimension based on the comparison.
        Vector d = Vector::Zero;
        d = Vector::Select( d, minDelta, lessThanMin );
        d = Vector::Select( d, maxDelta, greaterThanMax );

        // Use a dot-product to square them and sum them together.
        Vector const d2 = d.Dot3( d );
        if ( d2.IsGreaterThan3( sphere.m_radius * sphere.m_radius ) )
        {
            return OverlapResult::NoOverlap;
        }

        Vector insideAll = ( boxMin + sphere.m_radius ).LessThanEqual( sphere.m_center );
        insideAll = SIMD::Int::And( insideAll, sphere.m_center.LessThanEqual( boxMax - sphere.m_radius ) );
        insideAll = SIMD::Int::And( insideAll, ( boxMax - boxMin ).GreaterThan( sphere.m_radius ) );
        return SIMD::Int::Equal( insideAll, SIMD::g_trueMask ) ? OverlapResult::FullyEnclosed : OverlapResult::Overlap;
    }

    OverlapResult AABB::OverlapTest( OBB const& box ) const
    {
        EE_ASSERT( box.m_orientation.IsNormalized() );

        if ( !box.Overlaps( *this ) )
        {
            return OverlapResult::NoOverlap;
        }

        //-------------------------------------------------------------------------

        Vector insideAll( SIMD::g_trueMask );
        for ( uint32_t i = 0; i < 8; i++ )
        {
            Vector const c = box.m_center + box.m_orientation.RotateVector( box.m_extents * Vector::BoxCorners[i] );
            Vector const d = c.GetAbs();
            insideAll = SIMD::Int::And( insideAll, d.LessThanEqual( m_extents ) );
        }

        return ( SIMD::Int::Equal( insideAll, SIMD::g_trueMask ) ) ? OverlapResult::FullyEnclosed : OverlapResult::Overlap;
    }

    void AABB::ApplyTransform( Transform const& transform )
    {
        Vector min( FLT_MAX );
        Vector max( -FLT_MAX );

        // First corner
        Vector corner = Vector::MultiplyAdd( m_extents, Vector::BoxCorners[0], m_center );
        corner = transform.TransformPoint( corner );
        min = max = corner;

        // Remaining 7 corners
        for ( auto i = 1; i < 8; i++ )
        {
            corner = Vector::MultiplyAdd( m_extents, Vector::BoxCorners[i], m_center );
            corner = transform.TransformPoint( corner );
            min = Vector::Min( min, corner );
            max = Vector::Max( max, corner );
        }

        SetFromMinMax( min, max );
    }

    //-------------------------------------------------------------------------

    OBB::OBB( AABB const& aabb )
        : m_orientation( Quaternion::Identity )
        , m_center( aabb.GetCenter() )
        , m_extents( aabb.GetExtents() )
    {}

    OBB::OBB( AABB const& aabb, Transform const& transform )
    {
        Vector const center( aabb.GetCenter() );

        m_center = transform.TransformPoint( center ).ToFloat3();
        m_orientation = transform.GetRotation();
        m_extents = aabb.GetExtents();
    }

    OBB::OBB( Vector center, Vector extents, Quaternion orientation )
        : m_orientation( orientation )
        , m_center( center )
        , m_extents( extents )
    {
        EE_ASSERT( m_extents.IsGreaterThanEqual3( Vector::Zero ) && m_orientation.IsNormalized() );
    }

    // Copied from DirectXMath:
    //-----------------------------------------------------------------------------
    // Find the approximate minimum oriented bounding box containing a set of
    // points.  Exact computation of minimum oriented bounding box is possible but
    // is slower and requires a more complex algorithm.
    // The algorithm works by computing the inertia tensor of the points and then
    // using the eigenvectors of the inertia tensor as the axes of the box.
    // Computing the inertia tensor of the convex hull of the points will usually
    // result in better bounding box but the computation is more complex.
    // Exact computation of the minimum oriented bounding box is possible but the
    // best know algorithm is O(N^3) and is significantly more complex to implement.
    //
    // WARNING: this doesnt handle symmetric point clounds very well!!!!
    //-----------------------------------------------------------------------------

    OBB::OBB( Vector const* pPoints, uint32_t numPoints )
    {
        EE_ASSERT( pPoints != nullptr && numPoints > 0 );

        // Compute the center of mass and inertia tensor of the points.
        Vector CenterOfMass = Vector::Zero;
        for ( size_t i = 0; i < numPoints; ++i )
        {
            CenterOfMass = CenterOfMass + pPoints[i];
        }

        CenterOfMass = CenterOfMass * Vector( 1.0f / numPoints );

        // Compute the inertia tensor of the points around the center of mass.
        // Using the center of mass is not strictly necessary, but will hopefully
        // improve the stability of finding the eigenvectors.
        Vector XX_YY_ZZ = Vector::Zero;
        Vector XY_XZ_YZ = Vector::Zero;

        for ( size_t i = 0; i < numPoints; ++i )
        {
            Vector point = pPoints[i] - CenterOfMass;
            XX_YY_ZZ = XX_YY_ZZ + ( point * point );

            Vector XXY = point.Swizzle<0, 0, 1, 3>();
            Vector YZZ = point.Swizzle<1, 2, 2, 3>();

            XY_XZ_YZ = XY_XZ_YZ + ( XXY * YZZ );
        }

        // Compute the eigenvectors of the inertia tensor.
        Vector v1, v2, v3;
        Math::CalculateEigenVectorsFromCovarianceMatrix( XX_YY_ZZ.GetX(), XX_YY_ZZ.GetY(), XX_YY_ZZ.GetZ(), XY_XZ_YZ.GetX(), XY_XZ_YZ.GetY(), XY_XZ_YZ.GetZ(), v1, v2, v3 );

        // Put them in a matrix.
        Matrix R;
        R.m_rows[0] = v1.GetWithW0();
        R.m_rows[1] = v2.GetWithW0();
        R.m_rows[2] = v3.GetWithW0();
        R.m_rows[3] = Vector::UnitW;

        // Multiply by -1 to convert the matrix into a right handed coordinate
        // system (determinant ~= 1) in case the eigenvectors form a left handed
        // coordinate system (determinant ~= -1) because XMQuaternionRotationMatrix only
        // works on right handed matrices.
        Vector det = R.GetDeterminant();
        if ( det.IsLessThan4( Vector::Zero ) )
        {
            R.m_rows[0].Negate();
            R.m_rows[1].Negate();
            R.m_rows[2].Negate();
        }

        // Get the rotation quaternion from the matrix.
        Quaternion vOrientation = R.GetRotation();

        // Make sure it is normal (in case the vectors are slightly non-orthogonal).
        vOrientation.Normalize();

        // Rebuild the rotation matrix from the quaternion.
        R = Matrix( vOrientation );

        // Build the rotation into the rotated space.
        Matrix InverseR = R.GetTransposed();

        // Find the minimum OBB using the eigenvectors as the axes.
        Vector vMin, vMax;
        vMin = vMax = InverseR.RotateVector( pPoints[0] );
        for ( size_t i = 1; i < numPoints; ++i )
        {
            Vector rotatedPoint = InverseR.RotateVector( pPoints[i] );
            vMin = Vector::Min( vMin, rotatedPoint );
            vMax = Vector::Max( vMax, rotatedPoint );
        }

        m_extents = ( vMax - vMin ) * Vector::Half;
        m_orientation = vOrientation;

        // Rotate the center into world space.
        Vector vCenter = vMin + m_extents;
        vCenter = R.RotateVector( vCenter );
        m_center = vCenter;
    }

    void OBB::ApplyTransform( Transform const& transform )
    {
        Transform currentTransform( m_orientation, m_center );
        currentTransform = currentTransform * transform;

        m_center = currentTransform.GetTranslation();
        m_orientation = currentTransform.GetRotation();
        m_extents = ( m_extents * currentTransform.GetScale() ).Abs();
    }

    void OBB::ApplyScale( Vector const& scale )
    {
        EE_ASSERT( !scale.IsAnyEqualToZero3() );
        m_extents *= scale;
    }

    bool OBB::Overlaps( OBB const& box ) const
    {
        // Build the 3x3 rotation matrix that defines the orientation of B relative to A.

        Quaternion Q = m_orientation * box.m_orientation.GetConjugate();
        Matrix R( Q );

        // Compute the translation of B relative to A.
        Vector t = m_orientation.RotateVectorInverse( box.m_center - m_center );

        // h(A) = extents of A.
        // h(B) = extents of B.
        //
        // a(u) = axes of A = (1,0,0), (0,1,0), (0,0,1)
        // b(u) = axes of B relative to A = (r00,r10,r20), (r01,r11,r21), (r02,r12,r22)
        //  
        // For each possible separating axis l:
        //   d(A) = sum (for i = u,v,m_w) h(A)(i) * abs( a(i) dot l )
        //   d(B) = sum (for i = u,v,m_w) h(B)(i) * abs( b(i) dot l )
        //   if abs( t dot l ) > d(A) + d(B) then disjoint
        //

        // Rows. Note R[0,1,2]X.m_w = 0.
        Vector const R0X = R[0];
        Vector const R1X = R[1];
        Vector const R2X = R[2];

        R = R.Transpose();

        // Columns. Note RX[0,1,2].m_w = 0.
        Vector const RX0 = R[0];
        Vector const RX1 = R[1];
        Vector const RX2 = R[2];

        Vector const NRX0 = RX0.GetNegated();
        Vector const NRX1 = RX1.GetNegated();
        Vector const NRX2 = RX2.GetNegated();

        // Absolute value of rows.
        Vector const AR0X = R0X.GetAbs();
        Vector const AR1X = R1X.GetAbs();
        Vector const AR2X = R2X.GetAbs();

        // Absolute value of columns.
        Vector const ARX0 = RX0.GetAbs();
        Vector const ARX1 = RX1.GetAbs();
        Vector const ARX2 = RX2.GetAbs();

        // Test each of the 15 possible separating axes.
        Vector d, d_A, d_B;

        // l = a(u) = (1, 0, 0)
        // t dot l = t.m_x
        // d(A) = h(A).m_x
        // d(B) = h(B) dot abs(r00, r01, r02)
        d = t.GetSplatX();
        d_A = m_extents.GetSplatX();
        d_B = box.m_extents.Dot3( AR0X );
        Vector NoIntersection = d.Abs().GreaterThan( ( d_A + d_B ).Abs() );

        #define NO_INTERSECTION_TEST NoIntersection = SIMD::Int::Or( NoIntersection, d.Abs().GreaterThan( ( d_A + d_B ).Abs() ) );

        // l = a(v) = (0, 1, 0)
        // t dot l = t.m_y
        // d(A) = h(A).m_y
        // d(B) = h(B) dot abs(r10, r11, r12)
        d = t.GetSplatY();
        d_A = m_extents.GetSplatY();
        d_B = box.m_extents.Dot3( AR1X );
        NO_INTERSECTION_TEST

        // l = a(m_w) = (0, 0, 1)
        // t dot l = t.m_z
        // d(A) = h(A).m_z
        // d(B) = h(B) dot abs(r20, r21, r22)
        d = t.GetSplatZ();
        d_A = m_extents.GetSplatZ();
        d_B = box.m_extents.Dot3( AR2X );
        NO_INTERSECTION_TEST

        // l = b(u) = (r00, r10, r20)
        // d(A) = h(A) dot abs(r00, r10, r20)
        // d(B) = h(B).m_x
        d = t.Dot3( RX0 );
        d_A = m_extents.Dot3( ARX0 );
        d_B = box.m_extents.GetSplatX();
        NO_INTERSECTION_TEST

        // l = b(v) = (r01, r11, r21)
        // d(A) = h(A) dot abs(r01, r11, r21)
        // d(B) = h(B).m_y
        d = t.Dot3( RX1 );
        d_A = m_extents.Dot3( ARX1 );
        d_B = box.m_extents.GetSplatY();
        NO_INTERSECTION_TEST

        // l = b(m_w) = (r02, r12, r22)
        // d(A) = h(A) dot abs(r02, r12, r22)
        // d(B) = h(B).m_z
        d = t.Dot3( RX2 );
        d_A = m_extents.Dot3( ARX2 );
        d_B = box.m_extents.GetSplatZ();
        NO_INTERSECTION_TEST

        // l = a(u) m_x b(u) = (0, -r20, r10)
        // d(A) = h(A) dot abs(0, r20, r10)
        // d(B) = h(B) dot abs(0, r02, r01)
        d = t.Dot3( Vector::Permute<3, 6, 1, 0>( RX0, NRX0 ) );
        d_A = m_extents.Dot3( ARX0.Swizzle<3, 2, 1, 0>() );
        d_B = box.m_extents.Dot3( AR0X.Swizzle<3, 2, 1, 0>() );
        NO_INTERSECTION_TEST

        // l = a(u) m_x b(v) = (0, -r21, r11)
        // d(A) = h(A) dot abs(0, r21, r11)
        // d(B) = h(B) dot abs(r02, 0, r00)
        d = t.Dot3( Vector::Permute<3, 6, 1, 0>( RX1, NRX1 ) );
        d_A = m_extents.Dot3( ARX1.Swizzle<3, 2, 1, 0>() );
        d_B = box.m_extents.Dot3( AR0X.Swizzle<2, 3, 0, 1>() );
        NO_INTERSECTION_TEST

        // l = a(u) m_x b(m_w) = (0, -r22, r12)
        // d(A) = h(A) dot abs(0, r22, r12)
        // d(B) = h(B) dot abs(r01, r00, 0)
        d = t.Dot3( Vector::Permute<3, 6, 1, 0>( RX2, NRX2 ) );
        d_A = m_extents.Dot3( ARX2.Swizzle<3, 2, 1, 0>() );
        d_B = box.m_extents.Dot3( AR0X.Swizzle<1, 0, 3, 2>() );
        NO_INTERSECTION_TEST

        // l = a(v) m_x b(u) = (r20, 0, -r00)
        // d(A) = h(A) dot abs(r20, 0, r00)
        // d(B) = h(B) dot abs(0, r12, r11)
        d = t.Dot3( Vector::Permute<2, 3, 4, 1>( RX0, NRX0 ) );
        d_A = m_extents.Dot3( ARX0.Swizzle<2, 3, 0, 1>(  ) );
        d_B = box.m_extents.Dot3( AR1X.Swizzle<3, 2, 1, 0>(  ) );

        // l = a(v) m_x b(v) = (r21, 0, -r01)
        // d(A) = h(A) dot abs(r21, 0, r01)
        // d(B) = h(B) dot abs(r12, 0, r10)
        d = t.Dot3( Vector::Permute<2, 3, 4, 1>( RX1, NRX1 ) );
        d_A = m_extents.Dot3( ARX1.Swizzle<2, 3, 0, 1>() );
        d_B = box.m_extents.Dot3( AR1X.Swizzle<2, 3, 0, 1>() );
        NO_INTERSECTION_TEST

        // l = a(v) m_x b(m_w) = (r22, 0, -r02)
        // d(A) = h(A) dot abs(r22, 0, r02)
        // d(B) = h(B) dot abs(r11, r10, 0)
        d = t.Dot3( Vector::Permute<2, 3, 4, 1>( RX2, NRX2 ) );
        d_A = m_extents.Dot3( ARX2.Swizzle<2, 3, 0, 1>() );
        d_B = box.m_extents.Dot3( AR1X.Swizzle<1, 0, 3, 2>() );
        NO_INTERSECTION_TEST

        // l = a(m_w) m_x b(u) = (-r10, r00, 0)
        // d(A) = h(A) dot abs(r10, r00, 0)
        // d(B) = h(B) dot abs(0, r22, r21)
        d = t.Dot3( Vector::Permute<5, 0, 3, 2>( RX0, NRX0 ) );
        d_A = m_extents.Dot3( ARX0.Swizzle<1, 0, 3, 2>() );
        d_B = box.m_extents.Dot3( AR2X.Swizzle<3, 2, 1, 0>() );
        NO_INTERSECTION_TEST

        // l = a(m_w) m_x b(v) = (-r11, r01, 0)
        // d(A) = h(A) dot abs(r11, r01, 0)
        // d(B) = h(B) dot abs(r22, 0, r20)
        d = t.Dot3( Vector::Permute<5, 0, 3, 2>( RX1, NRX1 ) );
        d_A = m_extents.Dot3( ARX1.Swizzle<1, 0, 3, 2>() );
        d_B = box.m_extents.Dot3( AR2X.Swizzle<2, 3, 0, 1>() );
        NO_INTERSECTION_TEST

        // l = a(m_w) m_x b(m_w) = (-r12, r02, 0)
        // d(A) = h(A) dot abs(r12, r02, 0)
        // d(B) = h(B) dot abs(r21, r20, 0)
        d = t.Dot3( Vector::Permute<5, 0, 3, 2>( RX2, NRX2 ) );
        d_A = m_extents.Dot3( ARX2.Swizzle<1, 0, 3, 2>() );
        d_B = box.m_extents.Dot3( AR2X.Swizzle<1, 0, 3, 2>() );
        NO_INTERSECTION_TEST

        #undef NO_INTERSECTION_TEST

        // No separating axis found, boxes must intersect.
        return SIMD::Int::NotEqual( NoIntersection, SIMD::g_trueMask ) ? true : false;
    }

    bool OBB::Overlaps( Sphere const& sphere ) const
    {
        EE_ASSERT( m_orientation.IsNormalized() );

        // Transform the center of the sphere to be local to the box.
        // BoxMin = -m_extents
        // BoxMax = +m_extents
        Vector const transformedSphereCenter = m_orientation.RotateVectorInverse( sphere.m_center - m_center );

        // Find the distance to the nearest point on the box.
        // for each i in (m_x, m_y, m_z)
        // if (transformedSphereCenter(i) < BoxMin(i)) d2 += (transformedSphereCenter(i) - BoxMin(i)) ^ 2
        // else if (transformedSphereCenter(i) > BoxMax(i)) d2 += (transformedSphereCenter(i) - BoxMax(i)) ^ 2

        // Compute d for each dimension.
        Vector const lessThanMin = transformedSphereCenter.LessThan( m_extents.GetNegated() );
        Vector const greaterThanMax = transformedSphereCenter.GreaterThan( m_extents );
        Vector const minDelta = transformedSphereCenter + m_extents;
        Vector const maxDelta = transformedSphereCenter - m_extents;

        // Choose value for each dimension based on the comparison.
        Vector d = Vector::Zero;
        d = Vector::Select( d, minDelta, lessThanMin );
        d = Vector::Select( d, maxDelta, greaterThanMax );

        // Use a dot-product to square them and sum them together.
        Vector const d2 = d.Dot3( d );
        return d2.IsLessThanEqual4( sphere.m_radius * sphere.m_radius );
    }

    OverlapResult OBB::OverlapTest( Sphere const& sphere ) const
    {
        EE_ASSERT( m_orientation.IsNormalized() );

        // Transform the center of the sphere to be local to the box.
        // BoxMin = -m_extents
        // BoxMax = +m_extents
        Vector const transformedSphereCenter = m_orientation.RotateVectorInverse( sphere.m_center - m_center );

        // Find the distance to the nearest point on the box.
        // for each i in (m_x, m_y, m_z)
        // if (transformedSphereCenter(i) < BoxMin(i)) d2 += (transformedSphereCenter(i) - BoxMin(i)) ^ 2
        // else if (transformedSphereCenter(i) > BoxMax(i)) d2 += (transformedSphereCenter(i) - BoxMax(i)) ^ 2

        // Compute d for each dimension.
        Vector const lessThanMin = transformedSphereCenter.LessThan( m_extents.GetNegated() );
        Vector const greaterThanMax = transformedSphereCenter.GreaterThan( m_extents );
        Vector const minDelta = transformedSphereCenter + m_extents;
        Vector const maxDelta = transformedSphereCenter - m_extents;

        // Choose value for each dimension based on the comparison.
        Vector d = Vector::Zero;
        d = Vector::Select( d, minDelta, lessThanMin );
        d = Vector::Select( d, maxDelta, greaterThanMax );

        // Use a dot-product to square them and sum them together.
        Vector const d2 = d.Dot3( d );
        Vector const sphereRadiusSq = sphere.m_radius * sphere.m_radius;
        if ( d2.IsGreaterThan4( sphereRadiusSq ) )
        {
            return OverlapResult::NoOverlap;
        }

        // See if we are completely inside the box
        Vector SMin = sphere.m_center - sphere.m_radius;
        Vector SMax = sphere.m_center + sphere.m_radius;
        if ( SMin.IsInBounds3( m_extents ) && SMax.IsInBounds3( m_extents ) )
        {
            return OverlapResult::FullyEnclosed;
        }

        return OverlapResult::Overlap;
    }
}