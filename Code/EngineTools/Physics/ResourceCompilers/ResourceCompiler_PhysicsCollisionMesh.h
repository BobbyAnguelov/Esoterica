#pragma  once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceCompiler.h"

//-------------------------------------------------------------------------

namespace EE::RawAssets { class RawMesh; }

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class CollisionMeshCompiler : public Resource::Compiler
    {
        EE_REFLECT_TYPE( CollisionMeshCompiler );
        static const int32_t s_version = 15;

    public:

        CollisionMeshCompiler();
        virtual Resource::CompilationResult Compile( Resource::CompileContext const& ctx ) const override;

    private:

        bool CookTriangleMeshData( RawAssets::RawMesh const& rawMesh, Blob& outCookedData ) const;
        bool CookConvexMeshData( RawAssets::RawMesh const& rawMesh, Blob& outCookedData ) const;
    };
}