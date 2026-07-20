#pragma once

#include "ResourceDescriptor.h"
#include "ResourceCompilationDatabase.h"
#include "ResourceCompilerContext.h"
#include "Base/Resource/ResourceHeader.h"
#include "Base/Resource/IResource.h"
#include "Base/TypeSystem/ReflectedType.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    class CompilerRegistry;
    struct CompileDependencyResourceInfo;

    // Resource Compiler
    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API Compiler : public IReflectedType
    {
        EE_REFLECT_TYPE( Compiler );

    public:

        constexpr static char const * const s_compileArg = "-compile";
        constexpr static char const * const s_forceArg = "-force";
        constexpr static char const * const s_packageArg = "-package";
        constexpr static char const* const s_logDelimiter = "Esoterica Resource Compiler\n-------------------------------------------------------------------------\n\n";
        constexpr static uint64_t const s_binarySerializationVersion = 14;

        struct Output final
        {
            ResourceTypeID                                  m_typeID;
            uint64_t                                        m_version;
            bool                                            m_requiresAdditionalDataFile;
        };

    public:

        Compiler( String const& name ) : m_name( name ) {}

        // Get the friendly name for this compiler
        String const& GetName() const { return m_name; }

        // Get the version for the resource
        uint64_t GetVersionForResourceType( ResourceTypeID resourceTypeID ) const;

        // Get the list of resource types that we can compile
        virtual TVector<Output> const& GetCompilableResourceTypes() const { return m_outputs; }

        // Will this compiler generate an additional data file for this resource type?
        bool WillGenerateAdditionalDataFile( ResourceTypeID resourceTypeID ) const;

        // Override this function to provide additional dependencies that cannot be trivially found via the descriptor
        // Generally needed for things like navmesh generation where the map descriptor doesnt have the necessary information to extract exact dependencies
        virtual void GetAdditionalCompileDependencies( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& sourceResourceDirectoryPath, ResourceID const& resourceID, ResourceDescriptor const* pDescriptor, TVector<CompileDependency>& outDependencies ) const {}

        // Compile a resource
        void PerformUpToDateCheck( CompileContext& ctx ) const;

        // Compile a resource
        void CompileResource( CompileContext& ctx ) const;

    protected:

        virtual CompilationResult Compile( CompileContext const& ctx ) const = 0;

        // Override this function to generate custom compile dependency hashes to provide more info to the up-to-date check
        // This is useful in scenarios where you only dependent on some subset of data in a data file and want to avoid recompiling when irrelevant data changes
        virtual void GenerateCustomDependencyHashes( CompileContext const& ctx, CompileDependencyResourceInfo* pResourceToCompile ) const {}

        // Register a type that this compiler creates
        template<typename T>
        void RegisterOutput()
        {
            static_assert( std::is_base_of<EE::Resource::IResource, T>::value, "T is not derived from IResource" );
            m_outputs.emplace_back( T::GetStaticResourceTypeID(), T::s_version, T::s_requiresAdditionalDataFile );
        }

        // Override this to check any third-party versions for a given resource type
        virtual uint64_t GetAdditionalVersionForResourceType( ResourceTypeID resourceTypeID ) const { return 0; }

        // Logging
        //-------------------------------------------------------------------------

        CompilationResult Error( CompileContext const& ctx, char const* pFormat, ... ) const;
        CompilationResult Warning( CompileContext const& ctx, char const* pFormat, ... ) const;
        CompilationResult Message( CompileContext const& ctx, char const* pFormat, ... ) const;

        // Combines two results together and keeps the most severe one
        EE_FORCE_INLINE CompilationResult KeepHighestSeverityCompilationResult( CompilationResult a, CompilationResult b ) const
        {
            if ( a == Resource::CompilationResult::Failure || b == Resource::CompilationResult::Failure ) { return Resource::CompilationResult::Failure; }
            if ( a == Resource::CompilationResult::SuccessWithWarnings || b == Resource::CompilationResult::SuccessWithWarnings ) { return Resource::CompilationResult::SuccessWithWarnings; }
            if ( a == Resource::CompilationResult::SuccessUpToDate || b == Resource::CompilationResult::SuccessUpToDate ) { return Resource::CompilationResult::SuccessUpToDate; }
            return Resource::CompilationResult::Success;
        }

    private:

        Compiler& operator=( Compiler const& ) = delete;

    private:

        String const                                m_name;
        TVector<Output>                             m_outputs;
    };
}