#pragma once
#include "RenderMesh.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class EE_ENGINE_API StaticMesh : public Mesh
    {
        EE_RESOURCE( 'msh', "Static Mesh" );
        EE_SERIALIZE( EE_SERIALIZE_BASE( Mesh ) );
    };
}