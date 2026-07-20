#include "ResourceCompiler.h"
#include "Base/FileSystem/FileSystem.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "Base/Time/Timers.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    uint64_t Compiler::GetVersionForResourceType( ResourceTypeID resourceTypeID ) const
    {
        for ( auto const& outputType : m_outputs )
        {
            if ( outputType.m_typeID == resourceTypeID )
            {
                uint64_t const additionalVersion = GetAdditionalVersionForResourceType( resourceTypeID );
                return s_binarySerializationVersion + outputType.m_version + additionalVersion;
                break;
            }
        }

        EE_UNREACHABLE_CODE();
        return UINT64_MAX;
    }

    bool Compiler::WillGenerateAdditionalDataFile( ResourceTypeID resourceTypeID ) const
    {
        for ( auto const& outputType : m_outputs )
        {
            if ( outputType.m_typeID == resourceTypeID )
            {
                return outputType.m_requiresAdditionalDataFile;
            }
        }

        EE_UNREACHABLE_CODE();
        return false;
    }

    //-------------------------------------------------------------------------

    void Compiler::PerformUpToDateCheck( CompileContext& ctx ) const
    {
        EE_ASSERT( ctx.HasValidArguments() );
        ctx.m_compilerName = m_name;

        // Prepare for compilation
        //-------------------------------------------------------------------------

        EE_ASSERT( ctx.m_sourceResourceDirectoryPath.IsValid() );
        EE_ASSERT( ctx.m_compiledResourceDirectoryPath.IsValid() );

        //-------------------------------------------------------------------------

        ResourceTypeID const resourceTypeID = ctx.m_resourceID.GetResourceTypeID();
        if ( !ctx.m_typeRegistry.IsRegisteredResourceType( resourceTypeID ) )
        {
            ctx.m_result = ctx.LogError( "Invalid resource type ID - %s", resourceTypeID.ToString().c_str() );
            return;
        }

        {
            ScopedTimer<PlatformClock> dependencyTreeTimer( ctx.m_createDependencyTreeTime );

            ctx.m_pResourceToCompile = ctx.CreateResourceInfo( nullptr, ctx.m_resourceID );
            EE_ASSERT( ctx.m_pResourceToCompile != nullptr );

            if ( ctx.m_pResourceToCompile->HasError() )
            {
                ctx.m_result = ctx.LogError( ctx.m_pResourceToCompile->m_error.c_str() );
                return;
            }
        }
        ctx.LogMessage( "Dependency tree creation took: %.2fms", ctx.m_createDependencyTreeTime.ToFloat() );

        // Ensure we can create the output files
        //-------------------------------------------------------------------------

        if ( !ctx.m_pResourceToCompile->m_outputFileExists )
        {
            if ( !ctx.m_pResourceToCompile->m_outputPath.EnsureDirectoryExists() )
            {
                ctx.m_result = ctx.LogError( "Failed to create output directory - %s", ctx.m_pResourceToCompile->m_outputPath.GetParentDirectory().c_str() );
                return;
            }
        }

        if ( ctx.m_pResourceToCompile->m_outputFileExists && FileSystem::IsFileReadOnly( ctx.m_pResourceToCompile->m_outputPath ) )
        {
            ctx.m_result = ctx.LogError( "Output file(% s) is read-only!", ctx.m_pResourceToCompile->m_outputPath.GetFullPath().c_str() );
            return;
        }

        if ( ctx.m_pResourceToCompile->m_additionalOutputPath.IsValid() && !ctx.m_pResourceToCompile->m_additionalOutputFileExists )
        {
            if ( !ctx.m_pResourceToCompile->m_additionalOutputPath.EnsureDirectoryExists() )
            {
                ctx.m_result = ctx.LogError( "Failed to create additional output directory - %s", ctx.m_pResourceToCompile->m_additionalOutputPath.GetParentDirectory().c_str() );
                return;
            }
        }

        if ( ctx.m_pResourceToCompile->m_additionalOutputPath.IsValid() && ctx.m_pResourceToCompile->m_additionalOutputFileExists && FileSystem::IsFileReadOnly( ctx.m_pResourceToCompile->m_additionalOutputPath ) )
        {
            ctx.m_result = ctx.LogError( "Additional output file(% s) is read-only!", ctx.m_pResourceToCompile->m_additionalOutputPath.GetFullPath().c_str() );
            return;
        }

        // Update to date check
        //-------------------------------------------------------------------------

        ctx.m_requiresCompilation = true;
        {
            ScopedTimer<PlatformClock> upToDateCheckTimer( ctx.m_upToDateCheckTime );

            // Calculate the source hash for the dependency chain
            //-------------------------------------------------------------------------

            GenerateCustomDependencyHashes( ctx, ctx.m_pResourceToCompile );
            ctx.CalculateSourceResourceHash();

            // Check if we should be compiling this resource?
            //-------------------------------------------------------------------------

            if ( !ctx.m_forceCompilation )
            {
                if ( ctx.m_pResourceToCompile->m_outputFileExists )
                {
                    // If we have a valid compiled resource record (i.e. we have compiled this resource before) check the version and hashes
                    if ( ctx.m_pResourceToCompile->m_compiledResourceRecord.IsValid() )
                    {
                        // Is the compiler version up to date?
                        uint64_t const compiledResourceVersion = GetVersionForResourceType( resourceTypeID );
                        if ( ctx.m_pResourceToCompile->m_compiledResourceRecord.m_compilerVersion == compiledResourceVersion )
                        {
                            ctx.m_requiresCompilation = ctx.m_pResourceToCompile->m_compiledResourceRecord.m_sourceResourceHash != ctx.m_sourceResourceHash;
                        }
                    }
                }
            }
        }
        ctx.LogMessage( "Up to Date Check took: %.2fms", ctx.m_upToDateCheckTime.ToFloat() );

        // Should we proceed with the compilation?
        //-------------------------------------------------------------------------

        if ( !ctx.m_requiresCompilation )
        {
            ctx.LogMessage( "Resource is up to date, nothing to do!" );
            ctx.m_result = CompilationResult::SuccessUpToDate;
        }
        else
        {
            ctx.m_result = ctx.m_log.HasWarnings() ? CompilationResult::SuccessWithWarnings : CompilationResult::Success;
        }
    }

    void Compiler::CompileResource( CompileContext& ctx ) const
    {
        EE_ASSERT( ctx.HasValidArguments() );
        EE_ASSERT( ctx.m_pResourceToCompile != nullptr ); // Did you run the up-to-date check?
        EE_ASSERT( ctx.m_requiresCompilation );

        // Compile
        //-------------------------------------------------------------------------

        EE_ASSERT( ctx.m_sourceResourceHash != 0 );

        {
            ScopedTimer<PlatformClock> compileTimer( ctx.m_compilationTime );

            ctx.m_result = Compile( ctx );

            // Update database
            if ( ctx.m_result == CompilationResult::Success || ctx.m_result == CompilationResult::SuccessWithWarnings )
            {
                CompiledResourceRecord record;
                record.m_resourceID = ctx.m_pResourceToCompile->m_ID;
                record.m_compilerVersion = ctx.m_pResourceToCompile->m_compiledResourceVersion;
                record.m_sourceResourceHash = ctx.m_sourceResourceHash;
                bool dbResult = ctx.m_compiledResourceDB.WriteRecord( record );
                EE_ASSERT( dbResult );

                TVector<DataPath> compileDependencies;
                ctx.m_pResourceToCompile->GetAllCompileDependencyPaths( compileDependencies );
                dbResult = ctx.m_compiledResourceDB.WriteCompileDependencies( ctx.m_pResourceToCompile->m_ID, compileDependencies );
                EE_ASSERT( dbResult );
            }
            else // Remove any compilation record for this resource, but keep any dependencies to attempt to automatically trigger a recompile if they change
            {
                EE_ASSERT( ctx.m_result == CompilationResult::Failure );
                bool dbResult = ctx.m_compiledResourceDB.DeleteRecord( ctx.m_resourceID );
                EE_ASSERT( dbResult );
            }
        }

        ctx.LogMessage( "Compilation took: %.2fms", ctx.m_compilationTime.ToFloat() );

        //-------------------------------------------------------------------------

        if ( ctx.m_result == CompilationResult::SuccessWithWarnings || ( ctx.m_result == CompilationResult::Success && ctx.m_log.HasWarnings() ) )
        {
            ctx.LogWarning( "Compiled with warnings: %s", ctx.m_resourceID.c_str() );
            ctx.LogMessage( "Total time: %.2fms", ( ctx.m_upToDateCheckTime + ctx.m_compilationTime ).ToFloat() );
            ctx.LogMessage( "Output File: %s", ctx.m_pResourceToCompile->m_outputPath.c_str() );
            ctx.m_result = CompilationResult::SuccessWithWarnings;
        }

        if ( ctx.m_result == CompilationResult::Success )
        {
            ctx.LogMessage( "Compiled successfully: %s", ctx.m_resourceID.c_str() );
            ctx.LogMessage( "Total time: %.2fms", ( ctx.m_upToDateCheckTime + ctx.m_compilationTime ).ToFloat() );
            ctx.LogMessage( "Output File: %s", ctx.m_pResourceToCompile->m_outputPath.c_str() );
            ctx.m_result = CompilationResult::Success;
        }

        if ( ctx.m_result == CompilationResult::Failure )
        {
            ctx.LogError( "Failed to compile resource: %s", ctx.m_resourceID.c_str() );
        }
    }
}