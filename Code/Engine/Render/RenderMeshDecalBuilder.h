#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Render/RenderGeometryBuilder.h"
#include "Base/Math/Transform.h"
#include "Base/Math/Vector.h"
#include "Base/Types/Arrays.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    struct MeshDecalBakeData
    {
        Transform           m_localTransform;
        Float3              m_nonUniformScale = Float3::One;
    };

    //-------------------------------------------------------------------------

    struct EE_ENGINE_API MeshDecalBuilder final
    {
        enum class Error
        {
            None,
            NoTextureCoordinates,
            TooManyDecals,
            TooManyDecalsPerTriangle,
            TooManyProjectionBases,
            TooManyProjections,
        };

        static Error Build( GeometryBuilder& geometryBuilder, TArrayView<MeshDecalBakeData const> decals );
    };
}
