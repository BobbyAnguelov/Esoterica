#pragma  once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceCompiler.h"

//-------------------------------------------------------------------------

namespace EE::Import { class ImportedMesh; }

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

        bool CookTriangleMeshData( Import::ImportedMesh const& ImportedMesh, Blob& outCookedData ) const;
        bool CookConvexMeshData( Import::ImportedMesh const& ImportedMesh, Blob& outCookedData ) const;
    };
}