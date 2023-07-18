#include "Component_CoverVolume.h"
#include "Base/Drawing/DebugDrawing.h"

//-------------------------------------------------------------------------

namespace EE
{
    #if EE_DEVELOPMENT_TOOLS
    void CoverVolumeComponent::Draw( Drawing::DrawContext& drawingCtx ) const
    {
        BoxVolumeComponent::Draw( drawingCtx );

        Transform const& WT = GetWorldTransform();
        Float3 const volumeExtents = GetVolumeLocalExtents();
        Vector const forward = WT.GetForwardVector();
        Vector const right = WT.GetRightVector();
        Vector const volumeOffset = forward * volumeExtents.m_y;
        Vector const& coverFront = WT.GetTranslation() + volumeOffset;
        Vector const arrowOffset = ( forward * 0.5f );

        // Draw central arrow
        drawingCtx.DrawArrow( coverFront, coverFront + arrowOffset, GetVolumeColor(), 3.0f );

        // Draw type
        char const* coverTypeStr[] = { "Low", "HighL", "HighR", "Hidden" };
        drawingCtx.DrawText3D( WT.GetTranslation(), coverTypeStr[(int8_t) m_coverType], Colors::Yellow, Drawing::FontNormal, Drawing::TextAlignment::AlignMiddleCenter );

        // Draw additional arrows
        for ( float horizontalOffset = 1.0f; horizontalOffset <= volumeExtents.m_x; horizontalOffset++ )
        {
            Vector const rightOffset = right * horizontalOffset;
            Vector const leftOffset = -rightOffset;
            drawingCtx.DrawArrow( coverFront + rightOffset, coverFront + rightOffset + arrowOffset, GetVolumeColor(), 3.0f );
            drawingCtx.DrawArrow( coverFront + leftOffset, coverFront + leftOffset + arrowOffset, GetVolumeColor(), 3.0f );
        }
    }
    #endif
}