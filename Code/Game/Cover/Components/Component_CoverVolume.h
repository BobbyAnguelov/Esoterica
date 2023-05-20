#pragma once

#include "Game/_Module/API.h"
#include "Engine/Volumes/Components/Component_Volumes.h"

//-------------------------------------------------------------------------

namespace EE
{
    namespace Drawing{ class DrawContext; }

    //-------------------------------------------------------------------------

    enum class CoverType
    {
        EE_REFLECT_ENUM

        Low = 0,
        HighShootLeft,
        HighShootRight,
        HighHidden,
    };

    //-------------------------------------------------------------------------

    class EE_GAME_API CoverVolumeComponent : public BoxVolumeComponent
    {
        EE_ENTITY_COMPONENT( CoverVolumeComponent );

    public:

        inline CoverVolumeComponent() = default;
        inline CoverVolumeComponent( StringID name ) : BoxVolumeComponent( name ) {}

        #if EE_DEVELOPMENT_TOOLS
        virtual Color GetVolumeColor() const override { return Colors::GreenYellow; }
        virtual void Draw( Drawing::DrawContext& drawingCtx ) const override;
        #endif

    private:

        EE_REFLECT() CoverType    m_coverType = CoverType::HighHidden;
    };
}