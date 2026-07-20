#pragma once

#include "Plane.h"
#include "Line.h"
#include "NumericRange.h"
#include "BoundingVolumes.h"

//-------------------------------------------------------------------------

namespace EE
{
    class DebugDrawContext;
}

//-------------------------------------------------------------------------

namespace EE::Math
{
    //-------------------------------------------------------------------------
    // A camera view volume
    //-------------------------------------------------------------------------
    // Assumes the camera is using the standard convention for RH systems
    // * View Direction = -Z
    // * Up Direction = Y
    // * Right Direction = X
    //
    // Assumes clip space Z limit: [0 : 1]
    //
    // Note: FOV is horizontal

    class EE_BASE_API ViewVolume
    {

    public:

        constexpr static float const AspectRatio_4_3 = 4.0f / 3.0f;

        enum class PlaneID
        {
            Left = 0,
            Right,
            Top,
            Bottom,
            Near,
            Far
        };

        enum class ProjectionType
        {
            Orthographic,
            Perspective
        };

        enum class IntersectionResult
        {
            FullyOutside = 0,
            FullyInside,
            Intersects
        };

        inline static Radians ConvertVerticalToHorizontalFOV( float aspectRatio, Radians verticalAngle )
        {
            EE_ASSERT( aspectRatio > 0.0f && !Math::IsNearZero( aspectRatio ) );
            Radians const horizontalFOV( 2.0f * Math::ATan( aspectRatio * Math::Tan( (float) verticalAngle / 2.0f ) ) );
            return horizontalFOV;
        }

        inline static Radians ConvertHorizontalToVerticalFOV( float aspectRatio, Radians horizontalAngle )
        {
            EE_ASSERT( aspectRatio > 0.0f && !Math::IsNearZero( aspectRatio ) );
            Radians const verticalFOV( 2.0f * Math::ATan( Math::Tan( (float) horizontalAngle / 2.0f ) / aspectRatio ) );
            return verticalFOV;
        }

        // Set of world space corners for the view volume
        // Order: Near BL, Near TL, Near TR, Near BR, Far BL, Far TL, Far TR, Far BR
        union VolumeCorners
        {
            #if EE_DEVELOPMENT_TOOLS
            void DrawDebug( DebugDrawContext& drawingContext ) const;
            #endif

            struct
            {
                Vector m_nearBL;
                Vector m_nearTL;
                Vector m_nearTR;
                Vector m_nearBR;
                Vector m_farBL;
                Vector m_farTL;
                Vector m_farTR;
                Vector m_farBR;
            };

            Vector m_points[8] = {};
        };

        static inline ViewVolume CreatePerspective( float aspectRatio, FloatRange depthRange, Radians horizontalFOV, Matrix const& worldMatrix = Matrix::Identity )
        {
            return ViewVolume( aspectRatio, depthRange, horizontalFOV, worldMatrix );
        }

        static inline ViewVolume CreatePerspective( float aspectRatio, FloatRange depthRange, Radians horizontalFOV, Transform const& worldMatrix = Transform::Identity )
        {
            return ViewVolume( aspectRatio, depthRange, horizontalFOV, worldMatrix );
        }

        static inline ViewVolume CreateOrthographic( float viewWidth, float aspectRatio, FloatRange depthRange, Matrix const& worldMatrix = Matrix::Identity )
        {
            return ViewVolume( viewWidth, aspectRatio, depthRange, worldMatrix );
        }

        static inline ViewVolume CreateOrthographic( float viewWidth, float aspectRatio, FloatRange depthRange, Transform const& worldMatrix = Transform::Identity )
        {
            return ViewVolume( viewWidth, aspectRatio, depthRange, worldMatrix );
        }

    public:

        ViewVolume() {} // Warning: Leaves most members uninitialized!

        bool IsValid() const;

        // Set the current view, using EE world conventions (-Y forward, Z-up)
        void SetView( Vector const& position, Vector const& viewDir, Vector const& upDir );

        // Set the view width for a orthographic projection
        void SetViewWidth( float width );

        // Get the view width for a orthographic projection
        float GetViewWidth() const { EE_ASSERT( IsOrthographic() ); return m_viewWidth; }

        // Set the depth range in world units
        void SetDepthRange( FloatRange depthRange );

        // Get the aspect ratio (Horizontal/Vertical)
        void SetAspectRatio( float aspectRatio );

        // Set the horizontal FOV
        void SetHorizontalFOV( Radians FOV );

        // Projection Info
        //-------------------------------------------------------------------------

        inline bool IsPerspective() const { return m_type == ProjectionType::Perspective; }
        inline bool IsOrthographic() const { return m_type == ProjectionType::Orthographic; }

        inline FloatRange GetDepthRange() const { return m_depthRange; }
        inline float GetAspectRatio() const { return m_aspectRatio; }

        // Get the horizontal FOV for this view volume
        inline Radians GetHorizontalFOV() const { EE_ASSERT( IsPerspective() ); return m_horizontalFOV; }

        // Get the vertical FOV for this view volume
        Radians GetVerticalFOV() const;

        // Get the width and height of the near plane in world space units
        Float2 GetNearPlaneWorldSpaceDimensions() const;

        // Get the width and height of the far plane in world space units
        Float2 GetFarPlaneWorldSpaceDimensions() const;

        // Spatial Info
        //-------------------------------------------------------------------------

        EE_FORCE_INLINE Vector const& GetViewPosition() const { return m_viewPosition; }
        EE_FORCE_INLINE Vector const& GetViewForwardVector() const { return m_viewForwardDirection; }
        EE_FORCE_INLINE Vector const& GetViewRightVector() const { return m_viewRightDirection; }
        EE_FORCE_INLINE Vector const& GetViewUpVector() const { return m_viewUpDirection; }

        inline Matrix const& GetViewMatrix() const { return m_viewMatrix; }
        inline Matrix const& GetProjectionMatrix() const { return m_projectionMatrix; }
        inline Matrix const& GetViewProjectionMatrix() const { return m_viewProjectionMatrix; }
        inline Matrix const& GetInverseViewProjectionMatrix() const { return m_inverseViewProjectionMatrix; }

        // View Planes
        //-------------------------------------------------------------------------

        inline Plane const& GetViewPlane( PlaneID p ) const { return GetViewPlane( (uint32_t) p ); }
        inline Plane const& GetViewPlane( uint32_t p ) const
        {
            EE_ASSERT( p < 6 );
            return m_viewPlanes[p];
        }
        VolumeCorners GetCorners() const; // The first 4 points are the near plane corners, the last 4 are the far plane corners

        // Bounds and Intersection tests
        //-------------------------------------------------------------------------

        inline AABB GetAABB() const
        {
            VolumeCorners const corners = GetCorners();
            return AABB( corners.m_points, 8 );
        }

        OBB GetOBB() const;
        IntersectionResult Intersect( AABB const& aabb ) const;
        IntersectionResult Intersect( Vector const& point ) const;

        inline bool Contains( AABB const& aabb ) const { return Intersect( aabb ) != IntersectionResult::FullyOutside; }
        inline bool Contains( Vector const& point ) const { return Intersect( point ) != IntersectionResult::FullyOutside; }

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        void DrawDebug( DebugDrawContext& drawingContext ) const;
        #endif

    protected:

        // Orthographic view volume
        // Warning: This ctor will use the position, forward and up of the world matrix to set the view (this is assuming world coordinates (Z-up) )
        ViewVolume( float viewWidth, float aspectRatio, FloatRange depthRange, Matrix const& worldMatrix = Matrix::Identity );
        ViewVolume( float viewWidth, float aspectRatio, FloatRange depthRange, Transform const& worldMatrix = Transform::Identity );

        // Perspective view volume
        // Warning: This ctor will use the position, forward and up of the world matrix to set the view (this is assuming world coordinates (Z-up) )
        ViewVolume( float aspectRatio, FloatRange depthRange, Radians horizontalFOV, Matrix const& worldMatrix = Matrix::Identity );
        ViewVolume( float aspectRatio, FloatRange depthRange, Radians horizontalFOV, Transform const& worldMatrix = Transform::Identity );

    private:

        void CalculateProjectionMatrix();
        void UpdateInternals();

    private:

        Vector          m_viewPosition = Vector::Zero;
        Vector          m_viewForwardDirection = Vector::WorldForward;
        Vector          m_viewRightDirection = Vector::WorldRight;
        Vector          m_viewUpDirection = Vector::WorldUp;
        Matrix          m_viewMatrix;                           // The composed view matrix ( -Z forward, Y-up, RH )
        Matrix          m_projectionMatrix;                     // The projection conversion for this volume // TODO: infinite perspective, inverse Z
        Matrix          m_viewProjectionMatrix;                 // Cached view projection matrix
        Matrix          m_inverseViewProjectionMatrix;          // Inverse of the cached view projection matrix
        Plane           m_viewPlanes[6];                        // Cached view planes for this volume

        float           m_viewWidth = 1.0f;                     // Only relevant for orthographic projections
        float           m_aspectRatio = AspectRatio_4_3;
        Radians         m_horizontalFOV = Radians( 0.0f );      // The horizontal field of view angle (only for perspective projection)
        FloatRange      m_depthRange = FloatRange( 0 );         // The distance from the volume origin of the near/far planes ( X = near plane, Y = far plane )
        ProjectionType  m_type = ProjectionType::Perspective;   // The projection type
    };

    //-------------------------------------------------------------------------

    // Creates a right-handed perspective projection matrix
    EE_BASE_API Matrix CreatePerspectiveProjectionMatrix( float verticalFOV, float aspectRatio, float nearPlaneZ, float farPlaneZ );

    // Creates a right handed orthographic projection matrix
    EE_BASE_API Matrix CreateOrthographicProjectionMatrix( float viewWidth, float viewHeight, float nearPlaneZ, float farPlaneZ );
    EE_BASE_API Matrix CreateOrthographicProjectionMatrixOffCenter( float left, float right, float bottom, float top, float nearPlane, float farPlane );

    // Creates a lookat matrix
    EE_BASE_API Matrix CreateLookAtMatrix( Vector const& viewPosition, Vector const& focusPosition, Vector const& upDirection );

    // Calculate the 6 planes enclosing the volume
    EE_BASE_API void CalculateViewPlanes( Matrix const& viewProjectionMatrix, Plane viewPlanes[6] );
}
