#pragma once

#include "EngineTools/Resource/ResourceCompiler.h"

//-------------------------------------------------------------------------

namespace EE::Import
{
    class Mesh;
}

//-------------------------------------------------------------------------

namespace EE::Render
{
    class Mesh;
    class SkeletalMesh;

    struct MeshGroup;
    struct MeshResourceDescriptor;

    struct ConvertedMesh;

    //-------------------------------------------------------------------------

    class MeshCompiler : public Resource::Compiler
    {
        EE_REFLECT_TYPE( MeshCompiler );

    protected:

        using Compiler::Compiler;

    protected:

        Resource::CompilationResult CompileMesh( Resource::CompileContext const& ctx, Mesh& mesh, MeshResourceDescriptor const& resourceDescriptor, MeshGroup const& meshGroup ) const;

        static void SetResourceHeaderInstallDependencies( Mesh const& Mesh, Resource::ResourceHeader& hdr );
    };

    //-------------------------------------------------------------------------

    class StaticMeshCompiler final : public MeshCompiler
    {
        EE_REFLECT_TYPE( StaticMeshCompiler );

    public:

        StaticMeshCompiler();

        virtual Resource::CompilationResult Compile( Resource::CompileContext const& ctx ) const override;
    };

    //-------------------------------------------------------------------------

    class SkeletalMeshCompiler final : public MeshCompiler
    {
        EE_REFLECT_TYPE( SkeletalMeshCompiler );

    public:

        SkeletalMeshCompiler();

        virtual Resource::CompilationResult Compile( Resource::CompileContext const& ctx ) const override;
    };
}
