#include "OrientationGuide.h"
#include "Engine/Render/RenderViewport.h"
#include "System/Types/Arrays.h"
#include <EASTL/sort.h>

//-------------------------------------------------------------------------

namespace EE::ImGuiX::OrientationGuide
{
    constexpr static float const g_windowPadding = 4.0f;
    constexpr static float const g_windowRounding = 2.0f;
    constexpr static float const g_guideDimension = 55.0f;
    constexpr static float const g_axisHeadRadius = 3.0f;
    constexpr static float const g_axisHalfLength = ( g_guideDimension / 2 ) - g_axisHeadRadius - 4.0f;
    constexpr static float const g_worldRenderDistanceZ = 5.0f;
    constexpr static float const g_axisThickness = 2.0f;

    //-------------------------------------------------------------------------

    void Draw( Float2 const& guideOrigin, Render::Viewport const& viewport )
    {
        // Project world space axis positions to screen space
        //-------------------------------------------------------------------------

        Vector const& originWS = viewport.GetViewPosition() + viewport.GetViewForwardDirection() * g_worldRenderDistanceZ;
        Vector const& worldAxisX = ( Vector::UnitX );
        Vector const& worldAxisY = ( Vector::UnitY );
        Vector const& worldAxisZ = ( Vector::UnitZ );

        Vector const& worldAxisForwardPointX = ( originWS + worldAxisX );
        Vector const& worldAxisForwardPointY = ( originWS + worldAxisY );
        Vector const& worldAxisForwardPointZ = ( originWS + worldAxisZ );
        Vector const& worldAxisBackwardPointX = ( originWS - worldAxisX );
        Vector const& worldAxisBackwardPointY = ( originWS - worldAxisY );
        Vector const& worldAxisBackwardPointZ = ( originWS - worldAxisZ );

        Vector const axisStartPointX = Vector( viewport.WorldSpaceToScreenSpace( worldAxisBackwardPointX ) );
        Vector const axisStartPointY = Vector( viewport.WorldSpaceToScreenSpace( worldAxisBackwardPointY ) );
        Vector const axisStartPointZ = Vector( viewport.WorldSpaceToScreenSpace( worldAxisBackwardPointZ ) );
        Vector const axisEndPointX = Vector( viewport.WorldSpaceToScreenSpace( worldAxisForwardPointX ) );
        Vector const axisEndPointY = Vector( viewport.WorldSpaceToScreenSpace( worldAxisForwardPointY ) );
        Vector const axisEndPointZ = Vector( viewport.WorldSpaceToScreenSpace( worldAxisForwardPointZ ) );

        // Calculate screen space axis lengths
        //-------------------------------------------------------------------------

        float const axisLengthX = axisStartPointX.GetDistance2( axisEndPointX );
        float const axisLengthY = axisStartPointY.GetDistance2( axisEndPointY );
        float const axisLengthZ = axisStartPointZ.GetDistance2( axisEndPointZ );
        float const maxAxisLength = Math::Max( axisLengthX, Math::Max( axisLengthY, axisLengthZ ) );

        static float const axisHalfLengthSS = g_axisHalfLength;
        float const axisScaleX = ( axisLengthX / maxAxisLength ) * axisHalfLengthSS;
        float const axisScaleY = ( axisLengthY / maxAxisLength ) * axisHalfLengthSS;
        float const axisScaleZ = ( axisLengthZ / maxAxisLength ) * axisHalfLengthSS;

        // Calculate screen space axis directions
        Vector const origin = viewport.WorldSpaceToScreenSpace( originWS );
        Vector const axisDirX = ( axisEndPointX - origin ).GetNormalized2();
        Vector const axisDirY = ( axisEndPointY - origin ).GetNormalized2();
        Vector const axisDirZ = ( axisEndPointZ - origin ).GetNormalized2();

        // Sort SS axis and draw them in the correct order
        //-------------------------------------------------------------------------

        struct AxisDrawRequest { Axis m_axis; bool m_isInForwardDirection; float m_distance; };
        TInlineVector<AxisDrawRequest, 3> drawRequests;

        Plane const NearPlane = viewport.GetViewVolume().GetViewPlane( Math::ViewVolume::PlaneID::Near );

        drawRequests.push_back( { Axis::X, true, NearPlane.GetAbsoluteDistanceToPoint( worldAxisForwardPointX ) } );
        drawRequests.push_back( { Axis::Y, true, NearPlane.GetAbsoluteDistanceToPoint( worldAxisForwardPointY ) } );
        drawRequests.push_back( { Axis::Z, true, NearPlane.GetAbsoluteDistanceToPoint( worldAxisForwardPointZ ) } );
        drawRequests.push_back( { Axis::X, false, NearPlane.GetAbsoluteDistanceToPoint( worldAxisBackwardPointX ) } );
        drawRequests.push_back( { Axis::Y, false, NearPlane.GetAbsoluteDistanceToPoint( worldAxisBackwardPointY ) } );
        drawRequests.push_back( { Axis::Z, false, NearPlane.GetAbsoluteDistanceToPoint( worldAxisBackwardPointZ ) } );

        eastl::sort( drawRequests.begin(), drawRequests.end(), [] ( AxisDrawRequest const& lhs, AxisDrawRequest const& rhs ) { return lhs.m_distance > rhs.m_distance; } );

        //-------------------------------------------------------------------------

        auto pDrawList = ImGui::GetWindowDrawList();
        for ( auto const& request : drawRequests )
        {
            // X
            if ( request.m_axis == Axis::X && request.m_isInForwardDirection )
            {
                pDrawList->AddLine( guideOrigin, guideOrigin + axisDirX * ( axisScaleX - g_axisHeadRadius + 1.0f ), 0xBB0000FF, g_axisThickness );
                pDrawList->AddCircleFilled( guideOrigin + axisDirX * axisScaleX, g_axisHeadRadius, 0xBB0000FF );
            }
            else if ( request.m_axis == Axis::X && !request.m_isInForwardDirection )
            {
                pDrawList->AddCircleFilled( guideOrigin - axisDirX * axisScaleX, g_axisHeadRadius, 0x660000FF );
            }
            //Y
            else if ( request.m_axis == Axis::Y && request.m_isInForwardDirection )
            {
                pDrawList->AddLine( guideOrigin, guideOrigin + axisDirY * ( axisScaleY - g_axisHeadRadius + 1.0f ), 0xBB00FF00, g_axisThickness );
                pDrawList->AddCircleFilled( guideOrigin + axisDirY * axisScaleY, g_axisHeadRadius, 0xBB00FF00 );
            }
            else if ( request.m_axis == Axis::Y && !request.m_isInForwardDirection )
            {
                pDrawList->AddCircleFilled( guideOrigin - axisDirY * axisScaleY, g_axisHeadRadius, 0x6600FF00 );
            }
            // Z
            else if ( request.m_axis == Axis::Z && request.m_isInForwardDirection )
            {
                pDrawList->AddLine( guideOrigin, guideOrigin + axisDirZ * ( axisScaleZ - g_axisHeadRadius + 1.0f ), 0xBBFF0000, g_axisThickness );
                pDrawList->AddCircleFilled( guideOrigin + axisDirZ * axisScaleZ, g_axisHeadRadius, 0xBBFF0000 );
            }
            else if ( request.m_axis == Axis::Z && !request.m_isInForwardDirection )
            {
                pDrawList->AddCircleFilled( guideOrigin - axisDirZ * axisScaleZ, g_axisHeadRadius, 0x66FF0000 );
            }
        }
    }

    //-------------------------------------------------------------------------

    Float2 GetSize()
    {
        return Float2( g_guideDimension, g_guideDimension );
    }

    float GetWidth()
    {
        return g_guideDimension / 2;
    }
}