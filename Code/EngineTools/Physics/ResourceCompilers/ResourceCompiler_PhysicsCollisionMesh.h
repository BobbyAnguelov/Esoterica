#pragma  once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceCompiler.h"

//-------------------------------------------------------------------------

namespace EE::Import { class Mesh; }

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class CollisionMeshCompiler : public Resource::Compiler
    {
        EE_REFLECT_TYPE( CollisionMeshCompiler );

    public:

        CollisionMeshCompiler();
        virtual Resource::CompilationResult Compile( Resource::CompileContext const& ctx ) const override;
        virtual uint64_t GetAdditionalVersionForResourceType( ResourceTypeID resourceTypeID ) const override;

    private:

        bool CookTriangleMeshData( Import::Mesh const& ImportedMesh, Blob& outCookedData ) const;
        bool CookConvexMeshData( Import::Mesh const& ImportedMesh, Blob& outCookedData ) const;
    };
}