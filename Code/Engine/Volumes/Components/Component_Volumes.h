#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Entity/EntitySpatialComponent.h"
#include "Base/Types/Color.h"

//-------------------------------------------------------------------------

namespace EE
{
    namespace Drawing{ class DrawContext; }

    //-------------------------------------------------------------------------

    class EE_ENGINE_API VolumeComponent : public SpatialEntityComponent
    {
        EE_ENTITY_COMPONENT( VolumeComponent );

    public:

        inline VolumeComponent() = default;
        inline VolumeComponent( StringID name ) : SpatialEntityComponent( name ) {}

        #if EE_DEVELOPMENT_TOOLS
        virtual Color GetVolumeColor() const { return Colors::Gray; }
        virtual void Draw( Drawing::DrawContext& drawingCtx ) const {}
        #endif
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API BoxVolumeComponent : public VolumeComponent
    {
        EE_ENTITY_COMPONENT( BoxVolumeComponent );

    public:

        inline BoxVolumeComponent() = default;
        inline BoxVolumeComponent( StringID name ) : VolumeComponent( name ) {}

        // Get the half-size of the volume!
        inline Float3 GetVolumeLocalExtents() const { return m_extents; }

        #if EE_DEVELOPMENT_TOOLS
        virtual void Draw( Drawing::DrawContext& drawingCtx ) const override;
        #endif

    protected:

        virtual OBB CalculateLocalBounds() const override;

    protected:

        Float3 m_extents = Float3::One;
    };
}