#pragma once
#include "Engine/_Module/API.h"
#include "Engine/Volumes/Components/Component_Volumes.h"

//-------------------------------------------------------------------------

namespace EE::Navmesh
{
    class EE_ENGINE_API NavmeshInclusionVolumeComponent : public BoxVolumeComponent
    {
        EE_REGISTER_ENTITY_COMPONENT( NavmeshInclusionVolumeComponent );

    public:

        inline NavmeshInclusionVolumeComponent() = default;
        inline NavmeshInclusionVolumeComponent( StringID name ) : BoxVolumeComponent( name ) {}
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API NavmeshExclusionVolumeComponent : public BoxVolumeComponent
    {
        EE_REGISTER_ENTITY_COMPONENT( NavmeshExclusionVolumeComponent );

    public:

        inline NavmeshExclusionVolumeComponent() = default;
        inline NavmeshExclusionVolumeComponent( StringID name ) : BoxVolumeComponent( name ) {}
    };
}