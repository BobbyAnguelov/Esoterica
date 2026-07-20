#pragma  once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceCompiler.h"

//-------------------------------------------------------------------------

namespace EE::Navmesh
{
    class NavmeshData;

    //-------------------------------------------------------------------------

    class NavmeshCompiler : public Resource::Compiler
    {
        EE_REFLECT_TYPE( NavmeshCompiler );

    public:

        NavmeshCompiler();

        virtual Resource::CompilationResult Compile( Resource::CompileContext const& ctx ) const override;
        virtual void GetAdditionalCompileDependencies( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& sourceResourceDirectoryPath, ResourceID const& resourceID, Resource::ResourceDescriptor const* pDescriptor, TVector<Resource::CompileDependency>& outDependencies ) const override;
        virtual uint64_t GetAdditionalVersionForResourceType( ResourceTypeID resourceTypeID ) const override;

        bool LoadUserGeneratedNavmeshData( Resource::CompileContext const& ctx, FileSystem::Path const& path, NavmeshData& outData ) const;
        bool GenerateNavmeshData( Resource::CompileContext const& ctx, NavmeshData& outData ) const;
    };
}