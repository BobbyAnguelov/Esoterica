#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceCompiler.h"

//-------------------------------------------------------------------------

namespace EE::Import { class ImportedMesh; }

//-------------------------------------------------------------------------

namespace EE::Render
{
    class Mesh;
    class SkeletalMesh;
    struct MeshResourceDescriptor;

    //-------------------------------------------------------------------------

    class MeshCompiler : public Resource::Compiler
    {
        EE_REFLECT_TYPE( MeshCompiler );

    protected:

        using Resource::Compiler::Compiler;

    protected:

        void TransferMeshGeometry( Import::ImportedMesh const& ImportedMesh, Mesh& mesh, int32_t maxBoneInfluences ) const;
        void OptimizeMeshGeometry( Mesh& mesh ) const;
        void SetMeshDefaultMaterials( MeshResourceDescriptor const& descriptor, Mesh& mesh ) const;
        void SetMeshInstallDependencies( Mesh const& mesh, Resource::ResourceHeader& hdr ) const;
        virtual bool GetInstallDependencies( ResourceID const& resourceID, TVector<ResourceID>& outReferencedResources ) const override;
    };

    //-------------------------------------------------------------------------

    class StaticMeshCompiler : public MeshCompiler
    {
        EE_REFLECT_TYPE( StaticMeshCompiler );
        static const int32_t s_version = 2;

    public:

        StaticMeshCompiler();
        virtual Resource::CompilationResult Compile( Resource::CompileContext const& ctx ) const override;
    };

    //-------------------------------------------------------------------------

    class SkeletalMeshCompiler : public MeshCompiler
    {
        EE_REFLECT_TYPE( SkeletalMeshCompiler );
        static const int32_t s_version = 5;

    public:

        SkeletalMeshCompiler();
        virtual Resource::CompilationResult Compile( Resource::CompileContext const& ctx ) const override;

    private:

        void TransferSkeletalMeshData( Import::ImportedMesh const& ImportedMesh, SkeletalMesh& mesh ) const;
    };
}