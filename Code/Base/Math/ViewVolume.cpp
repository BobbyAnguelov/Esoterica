#include "ViewVolume.h"
#include "Base/Drawing/DebugDrawing.h"

//-------------------------------------------------------------------------

namespace EE::Math
{
    // Taken from DirectXMath: XMMatrixPerspectiveFovRH
    Matrix CreatePerspectiveProjectionMatrix( float verticalFOV, float aspectRatio, float nearPlaneZ, float farPlaneZ )
    {
        EE_ASSERT( nearPlaneZ > 0.f && farPlaneZ > 0.f );
        EE_ASSERT( !Math::IsNearEqual( verticalFOV, 0.0f, 0.00001f * 2.0f ) && !Math::IsNearEqual( aspectRatio, 0.0f, 0.00001f ) && !Math::IsNearEqual( farPlaneZ, nearPlaneZ, 0.00001f ) );

        float const halfFOV = verticalFOV * 0.5f;
        float const yScale = Math::Cos( halfFOV ) / Math::Sin( halfFOV );
        float const xScale = yScale / aspectRatio;
        float const nearMinusFarZ = ( nearPlaneZ - farPlaneZ );

        Matrix projectionMatrix( ZeroInit );
        projectionMatrix.m_values[0][0] = xScale;
        projectionMatrix.m_values[1][1] = yScale;
        projectionMatrix.m_values[2][2] = farPlaneZ / nearMinusFarZ;
        projectionMatrix.m_values[2][3] = -1.0f;
        projectionMatrix.m_values[3][2] = nearPlaneZ * farPlaneZ / nearMinusFarZ;

        return projectionMatrix;
    }

    // Taken from DirectXMath: XMMatrixOrthographicRH
    Matrix CreateOrthographicProjectionMatrix( float width, float height, float nearPlaneZ, float farPlaneZ )
    {
        EE_ASSERT( !Math::IsNearEqual( width, 0.0f, 0.00001f ) && !Math::IsNearEqual( height, 0.0f, 0.00001f ) && !Math::IsNearEqual( farPlaneZ, nearPlaneZ, 0.00001f ) );

        float const range = 1.0f / ( nearPlaneZ - farPlaneZ );

        Matrix projectionMatrix( NoInit );
        projectionMatrix.m_values[0][0] = 2.0f / width;
        projectionMatrix.m_values[0][1] = 0.0f;
        projectionMatrix.m_values[0][2] = 0.0f;
        projectionMatrix.m_values[0][3] = 0.0f;

        projectionMatrix.m_values[1][0] = 0.0f;
        projectionMatrix.m_values[1][1] = 2.0f / height;
        projectionMatrix.m_values[1][2] = 0.0f;
        projectionMatrix.m_values[1][3] = 0.0f;

        projectionMatrix.m_values[2][0] = 0.0f;
        projectionMatrix.m_values[2][1] = 0.0f;
        projectionMatrix.m_values[2][2] = range;
        projectionMatrix.m_values[2][3] = 0.0f;

        projectionMatrix.m_values[3][0] = 0.0f;
        projectionMatrix.m_values[3][1] = 0.0f;
        projectionMatrix.m_values[3][2] = range * nearPlaneZ;
        projectionMatrix.m_values[3][3] = 1.0f;

        return projectionMatrix;
    }

    // Taken from DirectXMath: XMMatrixOrthographicOffCenterRH
    // TODO: Rewrite with EE math lib
    static Matrix CreateOrthographicProjectionMatrixOffCenter( float left, float right, float bottom, float top, float nearPlane, float farPlane )
    {
        EE_ASSERT( !Math::IsNearEqual( right, left, 0.00001f ) && !Math::IsNearEqual( top, bottom, 0.00001f ) && !Math::IsNearEqual( farPlane, nearPlane, 0.00001f ) );

        float const fReciprocalWidth = 1.0f / ( right - left );
        float const fReciprocalHeight = 1.0f / ( top - bottom );
        float const fRange = 1.0f / ( nearPlane - farPlane );

        Vector vValues = { fReciprocalWidth, fReciprocalHeight, fRange, 1.0f };
        Vector rMem2 = { -( left + right ), -( top + bottom ), nearPlane, 1.0f };
        Vector vTemp = _mm_setzero_ps();

        // Copy x only
        vTemp = _mm_move_ss( vTemp, vValues );
        // fReciprocalWidth*2,0,0,0
        vTemp = _mm_add_ss( vTemp, vTemp );

        Matrix M;
        M.m_rows[0] = vTemp;
        // 0,fReciprocalHeight*2,0,0
        vTemp = vValues;
        vTemp = _mm_and_ps( vTemp, SIMD::g_mask0Y00 );
        vTemp = _mm_add_ps( vTemp, vTemp );
        M.m_rows[1] = vTemp;
        // 0,0,fRange,0.0f
        vTemp = vValues;
        vTemp = _mm_and_ps( vTemp, SIMD::g_mask00Z0 );
        M.m_rows[2] = vTemp;
        // -(left + right)*fReciprocalWidth,-(top + bottom)*fReciprocalHeight,fRange*-nearPlane,1.0f
        vValues = _mm_mul_ps( vValues, rMem2 );
        M.m_rows[3] = vValues;
        return M;
    }

    // Adapted from DirectXMath: XMMatrixLookAtLH (but changed to be right handed)
    // View matrix is -Z forward, +Y up and +X right
    static Matrix CreateLookToMatrix( Vector const& viewPosition, Vector const& viewDirection, Vector const& upDirection )
    {
        EE_ASSERT( viewPosition.IsW1() && !viewDirection.IsZero3() && !viewDirection.IsInfinite3() && !upDirection.IsZero3() && !upDirection.IsInfinite3() );

        Vector const R2 = viewDirection.GetNegated().GetNormalized3();
        Vector const R0 = Vector::Cross3( upDirection, R2 ).GetNormalized3();
        Vector const R1 = Vector::Cross3( R2, R0 );

        Vector const NegViewPosition = viewPosition.GetNegated();

        Vector const D0 = Vector::Dot3( R0, NegViewPosition );
        Vector const D1 = Vector::Dot3( R1, NegViewPosition );
        Vector const D2 = Vector::Dot3( R2, NegViewPosition );

        Matrix M;
        M[0] = Vector::Select( D0, R0, Vector::Select1110 );
        M[1] = Vector::Select( D1, R1, Vector::Select1110 );
        M[2] = Vector::Select( D2, R2, Vector::Select1110 );
        M[3] = Vector::UnitW;

        M.Transpose();
        return M;
    }

    EE_FORCE_INLINE static Matrix CreateLookAtMatrix( Vector const& viewPosition, Vector const& focusPosition, Vector const& upDirection )
    {
        Vector const viewDirection = focusPosition - viewPosition;
        return CreateLookToMatrix( viewPosition, viewDirection, upDirection );
    }

    // Adapted from DirectXMath: XMMatrixLookAtLH (but changed to be right handed)
    // View matrix is -Z forward, +Y up and +X right
    static Matrix CreateLookToMatrix( Vector const& viewPosition, Vector const& viewForward, Vector const& viewRight, Vector const& viewUp )
    {
        Vector const R2 = viewForward.GetNegated();
        Vector const R0 = viewRight;
        Vector const R1 = viewUp;

        Vector const NegViewPosition = viewPosition.GetNegated();

        Vector const D0 = Vector::Dot3( R0, NegViewPosition );
        Vector const D1 = Vector::Dot3( R1, NegViewPosition );
        Vector const D2 = Vector::Dot3( R2, NegViewPosition );

        Matrix M;
        M[0] = Vector::Select( D0, R0, Vector::Select1110 );
        M[1] = Vector::Select( D1, R1, Vector::Select1110 );
        M[2] = Vector::Select( D2, R2, Vector::Select1110 );
        M[3] = Vector::UnitW;

        M.Transpose();
        return M;
    }

    // Calculate the 6 planes enclosing the volume
    static void CalculateViewPlanes( Matrix const& viewProjectionMatrix, Plane viewPlanes[6] )
    {
        // Left clipping plane
        viewPlanes[0].a = viewProjectionMatrix[0][3] + viewProjectionMatrix[0][0];
        viewPlanes[0].b = viewProjectionMatrix[1][3] + viewProjectionMatrix[1][0];
        viewPlanes[0].c = viewProjectionMatrix[2][3] + viewProjectionMatrix[2][0];
        viewPlanes[0].d = viewProjectionMatrix[3][3] + viewProjectionMatrix[3][0];

        // Right clipping plane
        viewPlanes[1].a = viewProjectionMatrix[0][3] - viewProjectionMatrix[0][0];
        viewPlanes[1].b = viewProjectionMatrix[1][3] - viewProjectionMatrix[1][0];
        viewPlanes[1].c = viewProjectionMatrix[2][3] - viewProjectionMatrix[2][0];
        viewPlanes[1].d = viewProjectionMatrix[3][3] - viewProjectionMatrix[3][0];

        // Bottom clipping plane
        viewPlanes[3].a = viewProjectionMatrix[0][3] + viewProjectionMatrix[0][1];
        viewPlanes[3].b = viewProjectionMatrix[1][3] + viewProjectionMatrix[1][1];
        viewPlanes[3].c = viewProjectionMatrix[2][3] + viewProjectionMatrix[2][1];
        viewPlanes[3].d = viewProjectionMatrix[3][3] + viewProjectionMatrix[3][1];

        // Top clipping plane
        viewPlanes[2].a = viewProjectionMatrix[0][3] - viewProjectionMatrix[0][1];
        viewPlanes[2].b = viewProjectionMatrix[1][3] - viewProjectionMatrix[1][1];
        viewPlanes[2].c = viewProjectionMatrix[2][3] - viewProjectionMatrix[2][1];
        viewPlanes[2].d = viewProjectionMatrix[3][3] - viewProjectionMatrix[3][1];

        // Near clipping plane
        viewPlanes[4].a = viewProjectionMatrix[0][3] + viewProjectionMatrix[0][2];
        viewPlanes[4].b = viewProjectionMatrix[1][3] + viewProjectionMatrix[1][2];
        viewPlanes[4].c = viewProjectionMatrix[2][3] + viewProjectionMatrix[2][2];
        viewPlanes[4].d = viewProjectionMatrix[3][3] + viewProjectionMatrix[3][2];

        // Far clipping plane
        viewPlanes[5].a = viewProjectionMatrix[0][3] - viewProjectionMatrix[0][2];
        viewPlanes[5].b = viewProjectionMatrix[1][3] - viewProjectionMatrix[1][2];
        viewPlanes[5].c = viewProjectionMatrix[2][3] - viewProjectionMatrix[2][2];
        viewPlanes[5].d = viewProjectionMatrix[3][3] - viewProjectionMatrix[3][2];

        // Normalize planes
        viewPlanes[0].Normalize();
        viewPlanes[1].Normalize();
        viewPlanes[2].Normalize();
        viewPlanes[3].Normalize();
        viewPlanes[4].Normalize();
        viewPlanes[5].Normalize();
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void ViewVolume::VolumeCorners::DrawDebug( Drawing::DrawContext& drawingContext ) const
    {
        // Near Plane
        //-------------------------------------------------------------------------

        drawingContext.DrawPoint( m_points[0], Colors::Cyan, 10.0f );
        drawingContext.DrawPoint( m_points[1], Colors::Cyan, 10.0f );
        drawingContext.DrawPoint( m_points[2], Colors::Cyan, 10.0f );
        drawingContext.DrawPoint( m_points[3], Colors::Cyan, 10.0f );

        drawingContext.DrawLine( m_points[0], m_points[1], Colors::Cyan, 3.0f );
        drawingContext.DrawLine( m_points[1], m_points[2], Colors::Cyan, 3.0f );
        drawingContext.DrawLine( m_points[2], m_points[3], Colors::Cyan, 3.0f );
        drawingContext.DrawLine( m_points[3], m_points[0], Colors::Cyan, 3.0f );

        // Far Plane
        //-------------------------------------------------------------------------

        drawingContext.DrawPoint( m_points[4], Colors::Yellow, 10.0f );
        drawingContext.DrawPoint( m_points[5], Colors::Yellow, 10.0f );
        drawingContext.DrawPoint( m_points[6], Colors::Yellow, 10.0f );
        drawingContext.DrawPoint( m_points[7], Colors::Yellow, 10.0f );

        drawingContext.DrawLine( m_points[4], m_points[5], Colors::Yellow, 3.0f );
        drawingContext.DrawLine( m_points[5], m_points[6], Colors::Yellow, 3.0f );
        drawingContext.DrawLine( m_points[6], m_points[7], Colors::Yellow, 3.0f );
        drawingContext.DrawLine( m_points[7], m_points[4], Colors::Yellow, 3.0f );

        // Connecting Lines
        //-------------------------------------------------------------------------

        drawingContext.DrawLine( m_points[0], m_points[4], Colors::Cyan, 3.0f );
        drawingContext.DrawLine( m_points[1], m_points[5], Colors::Cyan, 3.0f );
        drawingContext.DrawLine( m_points[2], m_points[6], Colors::Cyan, 3.0f );
        drawingContext.DrawLine( m_points[3], m_points[7], Colors::Cyan, 3.0f );
    }
    #endif

    //-------------------------------------------------------------------------

    ViewVolume::ViewVolume( Float2 const& viewDimensions, FloatRange depthRange, Matrix const& worldMatrix )
        : m_viewDimensions( viewDimensions )
        , m_depthRange( depthRange )
        , m_type( ProjectionType::Orthographic )
    {
        EE_ASSERT( IsValid() );
        CalculateProjectionMatrix();
        SetView( worldMatrix.GetTranslation(), worldMatrix.GetForwardVector(), worldMatrix.GetUpVector() ); // Updates all internals

    }

    ViewVolume::ViewVolume( Float2 const& viewDimensions, FloatRange depthRange, Radians FOV, Matrix const& worldMatrix )
        : m_viewDimensions( viewDimensions )
        , m_FOV( FOV )
        , m_depthRange( depthRange )
        , m_type( ProjectionType::Perspective )
    {
        EE_ASSERT( IsValid() );
        CalculateProjectionMatrix();
        SetView( worldMatrix.GetTranslation(), worldMatrix.GetForwardVector(), worldMatrix.GetUpVector() ); // Updates all internals
    }

    bool ViewVolume::IsValid() const
    {
        if ( m_viewDimensions.m_x <= 0.0f || m_viewDimensions.m_y <= 0.0f || m_depthRange.m_begin < 0.0f || !m_depthRange.IsValid() )
        {
            return false;
        }

        if ( m_type == ProjectionType::Perspective )
        {
            if ( Math::IsNearZero( (float) m_FOV ) )
            {
                return false;
            }
        }

        return true;
    }

    //-------------------------------------------------------------------------

    void ViewVolume::SetView( Vector const& position, Vector const& viewDir, Vector const& upDir )
    {
        EE_ASSERT( viewDir.IsNormalized3() && upDir.IsNormalized3() );

        m_viewPosition = position;
        m_viewForwardDirection = viewDir;
        m_viewRightDirection = Vector::Cross3( viewDir, upDir );
        m_viewUpDirection = Vector::Cross3( m_viewRightDirection, viewDir );

        m_viewMatrix = CreateLookToMatrix( position.GetWithW1(), m_viewForwardDirection, m_viewRightDirection, m_viewUpDirection );
        UpdateInternals();
    }

    void ViewVolume::SetViewDimensions( Float2 dimensions )
    {
        EE_ASSERT( dimensions.m_x > 0.0f && dimensions.m_y > 0.0f );
        m_viewDimensions = dimensions;
        CalculateProjectionMatrix();
        UpdateInternals();
    }

    void ViewVolume::SetHorizontalFOV( Radians FOV )
    {
        EE_ASSERT( IsPerspective() && FOV > 0.0f );
        m_FOV = FOV;
        CalculateProjectionMatrix();
        UpdateInternals();
    }

    //-------------------------------------------------------------------------

    void ViewVolume::CalculateProjectionMatrix()
    {
        if ( IsOrthographic() )
        {
            m_projectionMatrix = CreateOrthographicProjectionMatrix( m_viewDimensions.m_x, m_viewDimensions.m_y, m_depthRange.m_begin, m_depthRange.m_end );
        }
        else
        {
            Radians const verticalFOV = ConvertHorizontalToVerticalFOV( m_viewDimensions.m_x, m_viewDimensions.m_y, m_FOV );
            m_projectionMatrix = CreatePerspectiveProjectionMatrix( (float) verticalFOV, GetAspectRatio(), m_depthRange.m_begin, m_depthRange.m_end );
        }
    }

    OBB ViewVolume::GetOBB() const
    {
        Radians const halfFOV = ( m_FOV / 2 );
        float const volumeDepth = m_depthRange.GetLength();

        float const halfX = Math::Tan( halfFOV.ToFloat() ) * m_depthRange.m_end;
        float const halfY = volumeDepth / 2;
        float const halfZ = halfX / GetAspectRatio();
        Vector const extents( halfX, halfY, halfZ );

        float const forwardOffset = m_depthRange.m_begin + halfY;
        Vector const center = m_viewPosition + ( m_viewForwardDirection * forwardOffset );

        Quaternion const orientation = Matrix( m_viewRightDirection.GetNegated(), m_viewForwardDirection.GetNegated(), m_viewUpDirection ).GetRotation();

        return OBB( center, extents, orientation );
    }

    void ViewVolume::SetDepthRange( FloatRange depthRange )
    {
        EE_ASSERT( depthRange.m_begin > 0 && depthRange.m_end > 0 && depthRange.IsValid() );
        m_depthRange = depthRange;
        CalculateProjectionMatrix();
        UpdateInternals();
    }

    void ViewVolume::UpdateInternals()
    {
        m_viewProjectionMatrix = m_viewMatrix * m_projectionMatrix;
        m_inverseViewProjectionMatrix = m_viewProjectionMatrix.GetInverse();
        CalculateViewPlanes( m_viewProjectionMatrix, m_viewPlanes );
    }

    //-------------------------------------------------------------------------

    Float2 ViewVolume::GetNearPlaneWorldSpaceDimensions() const
    {
        Vector const viewPosition = GetViewPosition();
        Vector const fwdDir = GetViewForwardVector();
        Vector const centerNear = Vector::MultiplyAdd( fwdDir, Vector( m_depthRange.m_begin ), viewPosition );

        Radians const verticalFOV = ConvertHorizontalToVerticalFOV( m_viewDimensions.m_x, m_viewDimensions.m_y, m_FOV );
        float const aspectRatio = GetAspectRatio();
        float e = Math::Tan( (float) verticalFOV * 0.5f );

        Float2 dimensions;
        dimensions.m_y = e * m_depthRange.m_begin * 2;
        dimensions.m_x = dimensions.m_y * aspectRatio;

        return dimensions;
    }

    Float2 ViewVolume::GetFarPlaneWorldSpaceDimensions() const
    {
        Vector const viewPosition = GetViewPosition();
        Vector const fwdDir = GetViewForwardVector();
        Vector const centerFar = Vector::MultiplyAdd( fwdDir, Vector( m_depthRange.m_end ), viewPosition );

        Radians const verticalFOV = ConvertHorizontalToVerticalFOV( m_viewDimensions.m_x, m_viewDimensions.m_y, m_FOV );
        float const aspectRatio = GetAspectRatio();
        float e = Math::Tan( (float) verticalFOV * 0.5f );

        Float2 dimensions;
        dimensions.m_y = e * m_depthRange.m_end * 2;
        dimensions.m_x = dimensions.m_y * aspectRatio;

        return dimensions;
    }

    ViewVolume::VolumeCorners ViewVolume::GetCorners() const
    {
        VolumeCorners vc;

        if ( IsOrthographic() )
        {
            Vector const viewPosition = GetViewPosition();
            Vector const fwdDir = GetViewForwardVector();
            Vector const upDir = GetViewUpVector();
            Vector const rightDir = GetViewRightVector();

            Vector const nearPlaneDepthCenterPoint = Vector::MultiplyAdd( fwdDir, Vector( m_depthRange.m_begin ), viewPosition );
            Vector const halfWidth( m_viewDimensions.m_x / 2, m_viewDimensions.m_y / 2, 0.0f, 0.0f );

            vc.m_points[0] = nearPlaneDepthCenterPoint - ( rightDir * ( m_viewDimensions.m_x / 2 ) ) - ( upDir * ( m_viewDimensions.m_y / 2 ) );
            vc.m_points[1] = nearPlaneDepthCenterPoint - ( rightDir * ( m_viewDimensions.m_x / 2 ) ) + ( upDir * ( m_viewDimensions.m_y / 2 ) );
            vc.m_points[2] = nearPlaneDepthCenterPoint + ( rightDir * ( m_viewDimensions.m_x / 2 ) ) + ( upDir * ( m_viewDimensions.m_y / 2 ) );
            vc.m_points[3] = nearPlaneDepthCenterPoint + ( rightDir * ( m_viewDimensions.m_x / 2 ) ) - ( upDir * ( m_viewDimensions.m_y / 2 ) );

            Vector const farPlaneDepthOffset = fwdDir * ( m_depthRange.m_end - m_depthRange.m_begin );
            vc.m_points[4] = vc.m_points[0] + farPlaneDepthOffset;
            vc.m_points[5] = vc.m_points[1] + farPlaneDepthOffset;
            vc.m_points[6] = vc.m_points[2] + farPlaneDepthOffset;
            vc.m_points[7] = vc.m_points[3] + farPlaneDepthOffset;
        }
        else
        {
            Vector const viewPosition = GetViewPosition();
            Vector const fwdDir = GetViewForwardVector();
            Vector const rightDir = GetViewRightVector();
            Vector const upDir = GetViewUpVector();

            // Near/far plane center points
            Vector const centerNear = Vector::MultiplyAdd( fwdDir, Vector( m_depthRange.m_begin ), viewPosition );
            Vector const centerFar = Vector::MultiplyAdd( fwdDir, Vector( m_depthRange.m_end ), viewPosition );

            // Get projected viewport extents on near/far planes
            Radians const verticalFOV = ConvertHorizontalToVerticalFOV( m_viewDimensions.m_x, m_viewDimensions.m_y, m_FOV );
            float const aspectRatio = GetAspectRatio();
            float e = Math::Tan( (float) verticalFOV * 0.5f );
            float extentUpNear = e * m_depthRange.m_begin;
            float extentRightNear = extentUpNear * aspectRatio;
            float extentUpFar = e * m_depthRange.m_end;
            float extentRightFar = extentUpFar * aspectRatio;

            // Points are just offset from the center points along camera basis
            vc.m_points[0] = centerNear - rightDir * extentRightNear - upDir * extentUpNear;
            vc.m_points[1] = centerNear - rightDir * extentRightNear + upDir * extentUpNear;
            vc.m_points[2] = centerNear + rightDir * extentRightNear + upDir * extentUpNear;
            vc.m_points[3] = centerNear + rightDir * extentRightNear - upDir * extentUpNear;
            vc.m_points[4] = centerFar - rightDir * extentRightFar - upDir * extentUpFar;
            vc.m_points[5] = centerFar - rightDir * extentRightFar + upDir * extentUpFar;
            vc.m_points[6] = centerFar + rightDir * extentRightFar + upDir * extentUpFar;
            vc.m_points[7] = centerFar + rightDir * extentRightFar - upDir * extentUpFar;
        }

        return vc;
    }

    //-------------------------------------------------------------------------

    Radians ViewVolume::GetVerticalFOV() const
    {
        EE_ASSERT( IsPerspective() );
        float const halfFOVX = GetFOV().ToFloat() * 0.5f;
        float const AR = GetAspectRatio();
        float const tanAR = Math::Tan( halfFOVX ) * AR;
        return 2.0f * Math::ATan( tanAR );
    }

    float ViewVolume::GetScalingFactorAtPosition( Vector const& position, float pixels ) const
    {
        float const percentageOfScreenY = pixels / m_viewDimensions.m_y;
        return percentageOfScreenY * GetViewForwardVector().GetDot3( position - GetViewPosition() );
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void ViewVolume::DrawDebug( Drawing::DrawContext& drawingContext ) const
    {
        VolumeCorners const corners = GetCorners();

        Vector const viewPosition = GetViewPosition();
        drawingContext.DrawPoint( viewPosition, Colors::LimeGreen, 10.0f );

        Vector const fwdDir = GetViewForwardVector();
        drawingContext.DrawArrow( viewPosition, viewPosition + ( fwdDir * m_depthRange.m_end ), Colors::Pink, 3.0f );

        //-------------------------------------------------------------------------

        drawingContext.DrawLine( corners.m_points[0], viewPosition, Colors::Red, 1.5f );
        drawingContext.DrawLine( corners.m_points[1], viewPosition, Colors::Red, 1.5f );
        drawingContext.DrawLine( corners.m_points[2], viewPosition, Colors::Red, 1.5f );
        drawingContext.DrawLine( corners.m_points[3], viewPosition, Colors::Red, 1.5f );

        //-------------------------------------------------------------------------

        drawingContext.Draw( corners );
    }
    #endif

    //-------------------------------------------------------------------------

    ViewVolume::IntersectionResult ViewVolume::Intersect( AABB const& aabb ) const
    {
        Vector const center( aabb.GetCenter() );
        Vector const extents( aabb.GetExtents() );

        for ( auto i = 0u; i < 6; i++ )
        {
            Plane plane( m_viewPlanes[i] );
            Vector const absPlane = m_viewPlanes[i].ToVector().Abs();

            auto const distance = plane.SignedDistanceToPoint( center );
            auto const radius = Vector::Dot3( extents, absPlane );

            // Is outside
            if ( ( distance + radius ).IsLessThan4( Vector::Zero ) )
            {
                return IntersectionResult::FullyOutside;
            }

            // Intersects
            if ( ( distance - radius ).IsLessThan4( Vector::Zero ) )
            {
                return IntersectionResult::Intersects;
            }
        }

        return IntersectionResult::FullyInside;
    }

    ViewVolume::IntersectionResult ViewVolume::Intersect( Vector const& point ) const
    {
        for ( auto i = 0u; i < 6; i++ )
        {
            Plane plane( m_viewPlanes[i] );

            Vector const distance = plane.SignedDistanceToPoint( point );

            // Is outside
            if ( distance.IsLessThan4( Vector::Zero ) )
            {
                return IntersectionResult::FullyOutside;
            }
        }

        return IntersectionResult::FullyInside;
    }
}