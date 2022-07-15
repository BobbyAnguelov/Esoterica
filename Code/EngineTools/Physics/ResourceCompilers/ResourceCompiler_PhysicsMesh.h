#pragma  once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceCompiler.h"

//-------------------------------------------------------------------------

namespace EE::RawAssets { class RawMesh; }

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class PhysicsMeshCompiler : public Resource::Compiler
    {
        EE_REGISTER_TYPE( PhysicsMeshCompiler );
        static const int32_t s_version = 4;

    public:

        PhysicsMeshCompiler();
        virtual Resource::CompilationResult Compile( Resource::CompileContext const& ctx ) const override;

    private:

        bool CookTriangleMeshData( RawAssets::RawMesh const& rawMesh, Blob& outCookedData ) const;
        bool CookConvexMeshData( RawAssets::RawMesh const& rawMesh, Blob& outCookedData ) const;
    };
}