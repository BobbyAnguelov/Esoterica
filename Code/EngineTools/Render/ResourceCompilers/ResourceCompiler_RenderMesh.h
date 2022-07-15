#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceCompiler.h"

//-------------------------------------------------------------------------

namespace EE::RawAssets { class RawMesh; }

//-------------------------------------------------------------------------

namespace EE::Render
{
    class Mesh;
    class SkeletalMesh;
    struct MeshResourceDescriptor;

    //-------------------------------------------------------------------------

    class MeshCompiler : public Resource::Compiler
    {
        EE_REGISTER_TYPE( MeshCompiler );

    protected:

        using Resource::Compiler::Compiler;

    protected:

        void TransferMeshGeometry( RawAssets::RawMesh const& rawMesh, Mesh& mesh, int32_t maxBoneInfluences ) const;
        void OptimizeMeshGeometry( Mesh& mesh ) const;
        void SetMeshDefaultMaterials( MeshResourceDescriptor const& descriptor, Mesh& mesh ) const;
        void SetMeshInstallDependencies( Mesh const& mesh, Resource::ResourceHeader& hdr ) const;
        virtual bool GetReferencedResources( ResourceID const& resourceID, TVector<ResourceID>& outReferencedResources ) const override;
    };

    //-------------------------------------------------------------------------

    class StaticMeshCompiler : public MeshCompiler
    {
        EE_REGISTER_TYPE( StaticMeshCompiler );
        static const int32_t s_version = 1;

    public:

        StaticMeshCompiler();
        virtual Resource::CompilationResult Compile( Resource::CompileContext const& ctx ) const override;
    };

    //-------------------------------------------------------------------------

    class SkeletalMeshCompiler : public MeshCompiler
    {
        EE_REGISTER_TYPE( SkeletalMeshCompiler );
        static const int32_t s_version = 4;

    public:

        SkeletalMeshCompiler();
        virtual Resource::CompilationResult Compile( Resource::CompileContext const& ctx ) const override;

    private:

        void TransferSkeletalMeshData( RawAssets::RawMesh const& rawMesh, SkeletalMesh& mesh ) const;
    };
}