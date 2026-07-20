#include "Component_Volumes.h"
#include "Base/Drawing/DebugDrawing.h"

//-------------------------------------------------------------------------

namespace EE
{
    OBB BoxVolumeComponent::CalculateLocalBounds() const
    {
        return OBB( Vector::Zero, m_extents );
    }

    #if EE_DEVELOPMENT_TOOLS
    void BoxVolumeComponent::Draw( DebugDrawContext& drawingCtx ) const
    {
        Color const volumeBorderColor = GetVolumeColor();
        Color const volumeColor = volumeBorderColor.GetAlphaVersion( 0.35f );
        auto const& worldBounds = GetWorldBounds();

        drawingCtx.DrawBox( worldBounds, volumeColor );
        drawingCtx.DrawWireBox( worldBounds, volumeBorderColor, 2.0f );
    }
    #endif
}