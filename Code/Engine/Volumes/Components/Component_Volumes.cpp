#include "Component_Volumes.h"
#include "System/Drawing/DebugDrawing.h"

//-------------------------------------------------------------------------

namespace EE
{
    void BoxVolumeComponent::Initialize()
    {
        VolumeComponent::Initialize();

        OBB const newBounds( Vector::Zero, Vector::One, Quaternion::Identity );
        SetLocalBounds( newBounds );
    }

    #if EE_DEVELOPMENT_TOOLS
    void BoxVolumeComponent::Draw( Drawing::DrawContext& drawingCtx ) const
    {
        Color const volumeBorderColor = GetVolumeColor();
        Color const volumeColor = volumeBorderColor.GetAlphaVersion( 0.35f );
        auto const& worldBounds = GetWorldBounds();

        drawingCtx.DrawBox( worldBounds, volumeColor, Drawing::EnableDepthTest );
        drawingCtx.DrawWireBox( worldBounds, volumeBorderColor, 2.0f, Drawing::EnableDepthTest );
    }
    #endif
}