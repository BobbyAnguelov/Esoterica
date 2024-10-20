#pragma once
#include "RenderMesh.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class EE_ENGINE_API StaticMesh : public Mesh
    {
        EE_RESOURCE( 'msh', "Static Mesh", 2, false );
        EE_SERIALIZE( EE_SERIALIZE_BASE( Mesh ) );
    };
}