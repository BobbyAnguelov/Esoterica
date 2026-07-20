#include "RenderGeometry.h"
#include "Engine/Render/Shaders/MeshData.esh"

//-------------------------------------------------------------------------

namespace EE::Render
{
    static_assert( sizeof( MeshCluster ) == sizeof( ShaderTypes::MeshCluster ) );
}
