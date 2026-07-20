#include "ResourceCompiler_Navmesh.h"
#include "EngineTools/Resource/ResourceCompilerContext.h"
#include "EngineTools/Entity/EntitySerializationTools.h"
#include "EngineTools/Entity/ResourceDescriptors/ResourceDescriptor_EntityMap.h"
#include "EngineTools/Physics/ResourceDescriptors/ResourceDescriptor_PhysicsCollisionMesh.h"
#include "EngineTools/Navmesh/NavmeshBuilder.h"
#include "EngineTools/Navmesh/NavmeshBuildData.h"
#include "EngineTools/Import/ImporterSource.h"
#include "Engine/Navmesh/NavmeshData.h"
#include "Engine/Entity/EntitySpatialComponent.h"
#include "Engine/Entity/EntityDescriptors.h"
#include "Engine/Navmesh/Components/Component_Navmesh.h"
#include "Base/FileSystem/FileSystem.h"
#include "Base/Serialization/BinarySerialization.h"
#include "Base/Time/Timers.h"
#include "Base/TypeSystem/TypeRegistry.h"

//-------------------------------------------------------------------------

namespace EE::Navmesh
{
    NavmeshCompiler::NavmeshCompiler()
        : Resource::Compiler( "NavmeshCompiler" )
    {
        RegisterOutput<NavmeshData>();
    }

    uint64_t NavmeshCompiler::GetAdditionalVersionForResourceType( ResourceTypeID resourceTypeID ) const
    {
        #if EE_ENABLE_NAVPOWER
        uint64_t const navpowerVersion = uint64_t( bfx::GetMajorVersion() ) + bfx::GetMinorVersion();
        return navpowerVersion;
        #else
        return 0;
        #endif
    }

    void NavmeshCompiler::GetAdditionalCompileDependencies( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& sourceResourceDirectoryPath, ResourceID const& resourceID, Resource::ResourceDescriptor const* pDescriptor, TVector<Resource::CompileDependency>& outDependencies ) const
    {
        if ( auto pMapDesc = TryCast<EntityModel::EntityMapResourceDescriptor>( pDescriptor ) )
        {
            // If we are using a pre-generated navmesh that is the only dependency
            FileSystem::Path const userGeneratedNavmeshPath = NavmeshData::GetUserGeneratedNavmeshFilePathForMap( resourceID.GetParentResourceID(), sourceResourceDirectoryPath );
            if ( userGeneratedNavmeshPath.Exists() )
            {
                outDependencies.emplace_back( DataPath( userGeneratedNavmeshPath, sourceResourceDirectoryPath ), false );
            }
            else // Add map and all nav relevant resources as dependencies
            {
                // Add map as a dependency
                DataPath const mapDataPath = resourceID.GetDataPath().GetPathWithoutSubFilename();
                outDependencies.emplace_back( mapDataPath, true );

                //-------------------------------------------------------------------------

                NavmeshBuildData buildData;
                buildData.ExtractBuildData( typeRegistry, pMapDesc->m_mapDescriptor );

                for ( TPair<ResourceID, TVector<NavmeshBuildData::MeshInstance>> const& meshInstancePair : buildData.m_collisionMeshInstances )
                {
                    outDependencies.emplace_back( meshInstancePair.first.GetDataPath(), true );
                }
            }
        }
    }

    Resource::CompilationResult NavmeshCompiler::Compile( Resource::CompileContext const& ctx ) const
    {
        NavmeshData navmeshData;

        FileSystem::Path const userGeneratedNavmeshPath = NavmeshData::GetUserGeneratedNavmeshFilePathForMap( ctx.GetResourceID().GetParentResourceID(), ctx.m_sourceResourceDirectoryPath );
        if ( userGeneratedNavmeshPath.Exists() )
        {
            if ( !LoadUserGeneratedNavmeshData( ctx, userGeneratedNavmeshPath, navmeshData ) )
            {
                return Resource::CompilationResult::Failure;
            }
        }
        else
        {
            if ( !GenerateNavmeshData( ctx, navmeshData ) )
            {
                return Resource::CompilationResult::Failure;
            }
        }

        // Save navmesh data
        //-------------------------------------------------------------------------

        EE_ASSERT( navmeshData.IsValid() );

        Resource::ResourceHeader hdr( NavmeshData::s_version, NavmeshData::GetStaticResourceTypeID(), 0 );
        Serialization::BinaryOutputArchive archive;
        archive << hdr << navmeshData;

        if ( archive.WriteToFile( ctx.GetOutputPath() ) )
        {
            return Resource::CompilationResult::Success;
        }
        else
        {
            return Resource::CompilationResult::Failure;
        }
    }

    bool NavmeshCompiler::LoadUserGeneratedNavmeshData( Resource::CompileContext const& ctx, FileSystem::Path const& path, NavmeshData& outData ) const
    {
        Serialization::BinaryInputArchive archive;
        if ( !archive.ReadFromFile( path ) )
        {
            return false;
        }

        archive << outData;
        return outData.IsValid();
    }

    bool NavmeshCompiler::GenerateNavmeshData( Resource::CompileContext const& ctx, NavmeshData& outData ) const
    {
        Milliseconds time = 0;

        // Setup Build Data
        //-------------------------------------------------------------------------

        NavmeshBuildData buildData;

        {
            ScopedTimer<PlatformClock> timer( time );

            // Extract Build data
            //-------------------------------------------------------------------------

            auto pMapDesc = Cast<EntityModel::EntityMapResourceDescriptor>( ctx.m_pResourceToCompile->m_pDescriptor );
            buildData.ExtractBuildData( ctx.m_typeRegistry, pMapDesc->m_mapDescriptor );
            EE_ASSERT( !buildData.HasErrors() );

            // Dump build data log
            for ( auto const& entry : buildData.GetLogEntries() )
            {
                if ( entry.m_severity == Severity::Warning )
                {
                    ctx.LogWarning( entry.m_message.c_str() );
                }
                else
                {
                    ctx.LogMessage( entry.m_message.c_str() );
                }
            }

            // Enable build logging if requested
            if ( buildData.m_buildSettings.m_enableBuildLogging )
            {
                buildData.m_optionalBuildLogPath = ctx.GetOutputPath();
            }

            // Load collision meshes
            //-------------------------------------------------------------------------

            auto GetMeshLoadInfo = [&ctx] ( ResourceID const& resourceID, Import::Source& outSource, TVector<String>& outMeshesToInclude )
            {
                auto pPhysCollisionMeshDesc = ctx.GetDescriptor<Physics::PhysicsCollisionMeshResourceDescriptor>( resourceID );
                outMeshesToInclude = pPhysCollisionMeshDesc->m_meshesToInclude;

                outSource.m_path = pPhysCollisionMeshDesc->m_sourcePath.GetFileSystemPath( ctx.m_sourceResourceDirectoryPath );
                outSource.m_pFileData = ctx.GetRawData( pPhysCollisionMeshDesc->m_sourcePath );
            };

            buildData.LoadCollisionMeshes( ctx.m_typeRegistry, ctx.m_sourceResourceDirectoryPath, GetMeshLoadInfo );
        }

        ctx.LogMessage( "Build Data Setup: %.2fms", time.ToFloat() );

        // Navpower Build
        //-------------------------------------------------------------------------

        #if EE_ENABLE_NAVPOWER
        {
            ScopedTimer<PlatformClock> timer( time );
            NavmeshBuilder builder;
            if ( !builder.Build( buildData, outData ) )
            {
                return false;
            }
        }
        ctx.LogMessage( "Navpower Build Completed: %.2fms", time.ToFloat() );
        #else
        ctx.LogError( "Navpower is disabled!" );
        return false;
        #endif

        if ( !outData.IsValid() )
        {
            return false;
        }

        return true;
    }
}