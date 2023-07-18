#pragma once

#include "Plane.h"
#include "Line.h"
#include "NumericRange.h"
#include "BoundingVolumes.h"

//-------------------------------------------------------------------------

namespace EE::Drawing { class DrawContext; }

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

        enum class PlaneID { Left = 0, Right, Top, Bottom, Near, Far };
        enum class ProjectionType { Orthographic, Perspective };
        enum class IntersectionResult { FullyOutside = 0, FullyInside, Intersects };

        inline static Radians ConvertVerticalToHorizontalFOV( float width, float height, Radians VerticalAngle )
        {
            EE_ASSERT( !Math::IsNearZero( height ) );
            Radians const horizontalFOV( 2.0f * Math::ATan( width / height * Math::Tan( (float) VerticalAngle / 2.0f ) ) );
            return horizontalFOV;
        }

        inline static Radians ConvertHorizontalToVerticalFOV( float width, float height, Radians HorizontalAngle )
        {
            EE_ASSERT( !Math::IsNearZero( width ) );
            Radians const verticalFOV( 2.0f * Math::ATan( height / width * Math::Tan( (float) HorizontalAngle / 2.0f ) ) );
            return verticalFOV;
        }

        // Set of world space corners for the view volume
        // Order: Near BL, Near TL, Near TR, Near BR, Far BL, Far TL, Far TR, Far BR
        union VolumeCorners
        {
            #if EE_DEVELOPMENT_TOOLS
            void DrawDebug( Drawing::DrawContext& drawingContext ) const;
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

    public:

        ViewVolume() {} // Warning: Leaves most members uninitialized!

        // Warning: This ctor will use the position, forward and up of the world matrix to set the view (this is assuming world coordinates (Z-up))
        ViewVolume( Float2 const& viewDimensions, FloatRange depthRange, Matrix const& worldMatrix = Matrix::Identity );

        // Warning: This ctor will use the position, forward and up of the world matrix to set the view (this is assuming world coordinates (Z-up))
        ViewVolume( Float2 const& viewDimensions, FloatRange depthRange, Radians FOV, Matrix const& worldMatrix = Matrix::Identity );

        bool IsValid() const;

        // Set the current view, using EE world conventions (-Y forward, Z-up)
        void SetView( Vector const& position, Vector const& viewDir, Vector const& upDir );
        void SetDepthRange( FloatRange depthRange );
        void SetViewDimensions( Float2 dimensions );
        void SetHorizontalFOV( Radians FOV );

        // Projection Info
        //-------------------------------------------------------------------------

        inline bool IsPerspective() const { return m_type == ProjectionType::Perspective; }
        inline bool IsOrthographic() const { return m_type == ProjectionType::Orthographic; }

        inline FloatRange GetDepthRange() const { return m_depthRange; }
        inline Float2 GetViewDimensions() const { return m_viewDimensions; }
        inline float GetAspectRatio() const { return m_viewDimensions.m_x / m_viewDimensions.m_y; }

        // Get the horizontal FOV for this view volume
        inline Radians GetFOV() const { EE_ASSERT( IsPerspective() ); return m_FOV; }

        // Get the vertical FOV for this view volume
        Radians GetVerticalFOV() const;

        // Get the size of a vector at a specific position if we want to it be a specific pixel height
        float GetScalingFactorAtPosition( Vector const& position, float pixels ) const;

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
        inline Plane const& GetViewPlane( uint32_t p ) const { EE_ASSERT( p < 6 ); return m_viewPlanes[p]; }
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
        void DrawDebug( Drawing::DrawContext& drawingContext ) const;
        #endif

    private:

        void CalculateProjectionMatrix();
        void UpdateInternals();

    private:

        Vector                  m_viewPosition = Vector::Zero;
        Vector                  m_viewForwardDirection = Vector::WorldForward;
        Vector                  m_viewRightDirection = Vector::WorldRight;
        Vector                  m_viewUpDirection = Vector::WorldUp;
        Matrix                  m_viewMatrix;                           // The composed view matrix ( -Z forward, Y-up, RH )
        Matrix                  m_projectionMatrix;                     // The projection conversion for this volume // TODO: infinite perspective, inverse Z
        Matrix                  m_viewProjectionMatrix;                 // Cached view projection matrix
        Matrix                  m_inverseViewProjectionMatrix;          // Inverse of the cached view projection matrix
        Plane                   m_viewPlanes[6];                        // Cached view planes for this volume

        Float2                  m_viewDimensions = Float2::Zero;        // The dimensions of the view volume
        Radians                 m_FOV = Radians( 0.0f );                // The horizontal field of view angle (only for perspective projection)
        FloatRange              m_depthRange = FloatRange( 0 );         // The distance from the volume origin of the near/far planes ( X = near plane, Y = far plane )
        ProjectionType          m_type = ProjectionType::Perspective;   // The projection type
    };

    //-------------------------------------------------------------------------

    // Creates a right-handed perspective projection matrix
    EE_BASE_API Matrix CreatePerspectiveProjectionMatrix( float verticalFOV, float aspectRatio, float nearPlaneZ, float farPlaneZ );

    // Creates a right handed orthographic projection matrix
    EE_BASE_API Matrix CreateOrthographicProjectionMatrix( float width, float height, float nearPlaneZ, float farPlaneZ );
}