#pragma once

#include "Engine/_Module/API.h"
#include "System/Render/RenderTarget.h"
#include "System/Math/ViewVolume.h"
#include "System/Math/Rectangle.h"

//-------------------------------------------------------------------------
// EE viewport
//-------------------------------------------------------------------------
// Defines an area of the 2D window that we can render to as well as the 3D volume to be rendered within it
// Provides space conversion functions between the three main spaces:
//  * World Space - Engine 3D space, units = meters
//  * Clip Space - The 3d world space projected to the unit cube for clipping, units = Normalized device coordinates (NDC)
//  * Screen Space - The 2D screen rectangle we render in, units = pixels

namespace EE::Render
{
    class Texture;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API Viewport
    {
    public:

        Viewport() = default;
        Viewport( Int2 const& topLeft, Int2 const& size, Math::ViewVolume const& viewVolume = Math::ViewVolume() );

        inline bool IsValid() const { return m_ID.IsValid() && ( m_size.m_x > 0 && m_size.m_y > 0 ) && ( m_topLeftPosition.m_x >= 0 && m_topLeftPosition.m_y >= 0 ); }
        inline UUID const& GetID() const { return m_ID; }

        //-------------------------------------------------------------------------

        // Resize the viewport - Note: this will not resize the render target immediately
        void Resize( Int2 const& topLeftPosition, Int2 const& dimensions );

        // Resize the viewport - Note: this will not resize the render target immediately
        void Resize( Math::Rectangle const& rect );

        // Get the viewport position and dimensions - All in screen space
        //-------------------------------------------------------------------------

        inline Float2 const& GetDimensions() const { return m_size; }
        inline Float2 const& GetTopLeftPosition() const { return m_topLeftPosition; }
        inline Float2 GetBottomRightPosition() const { return m_topLeftPosition + m_size; }
        inline float GetAspectRatio() const { return m_size.m_x / m_size.m_y; }

        // View Volume
        //-------------------------------------------------------------------------

        void SetViewVolume( Math::ViewVolume const& viewVolume );
        inline Math::ViewVolume const& GetViewVolume() const { return m_viewVolume; }

        inline bool IsOrthographic() const { return m_viewVolume.IsOrthographic(); }
        inline bool IsPerspective() const { return m_viewVolume.IsOrthographic(); }

        inline Vector GetViewPosition() const { return m_viewVolume.GetViewPosition(); }
        inline Vector GetViewForwardDirection() const { return m_viewVolume.GetViewForwardVector(); }
        inline Vector GetViewRightDirection() const { return m_viewVolume.GetViewRightVector(); }
        inline Vector GetViewUpDirection() const { return m_viewVolume.GetViewUpVector(); }

        // Queries
        //-------------------------------------------------------------------------

        inline bool ContainsPointScreenSpace( Float2 const& point ) const 
        {
            Float2 const bottomRightPosition = GetBottomRightPosition();
            return Math::IsInRangeInclusive( point.m_x, m_topLeftPosition.m_x, bottomRightPosition.m_x ) && Math::IsInRangeInclusive( point.m_y, m_topLeftPosition.m_y, bottomRightPosition.m_y );
        }

        inline bool IsWorldSpacePointVisible( Vector const& point ) const
        {
            return m_viewVolume.Contains( point );
        }

        // Space conversions
        //-------------------------------------------------------------------------

        Vector WorldSpaceToClipSpace( Vector const& pointWS ) const;
        inline Float2 WorldSpaceToScreenSpace( Vector const& pointWS ) const { return ClipSpaceToScreenSpace( WorldSpaceToClipSpace( pointWS ) ); }

        Float2 ClipSpaceToScreenSpace( Vector const& pointCS ) const;

        // Get the line segment from the near plane to the far plane for a given clip space point
        LineSegment ClipSpaceToWorldSpace( Vector const& pointCS ) const;

        Vector ScreenSpaceToClipSpace( Float2 const& pointSS ) const;
        Vector ScreenSpaceToWorldSpaceNearPlane( Vector const& pointSS ) const;
        Vector ScreenSpaceToWorldSpaceFarPlane( Vector const& pointSS ) const;

        // Get the line segment from the near plane to the far plane for a given screen space point
        inline LineSegment ScreenSpaceToWorldSpace( Float2 const& pointSS ) const { return ClipSpaceToWorldSpace( ScreenSpaceToClipSpace( pointSS ) ); }

        // Get the size of a vector at a specific position if we want to it be a specific pixel height
        inline float GetScalingFactorAtPosition( Vector const& position, float pixels ) const
        {
            float const distanceToPoint = m_viewVolume.IsOrthographic() ? 1.0f : ( position - GetViewPosition() ).GetLength3();
            return m_viewVolume.GetProjectionScaleY() * distanceToPoint * ( pixels / GetDimensions().m_y );
        }

    private:

        UUID                            m_ID = UUID::GenerateID();
        Math::ViewVolume                m_viewVolume = Math::ViewVolume( Float2( 1, 1 ), FloatRange( 0.1f, 1000.0f ) );
        Float2                          m_topLeftPosition = Float2( 0.0f );
        Float2                          m_size = Float2( 0.0f );
    };
}