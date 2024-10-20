#include "ResourceCompiler_Navmesh.h"
#include "EngineTools/Entity/EntitySerializationTools.h"
#include "EngineTools/Navmesh/NavmeshGenerator.h"
#include "Engine/Navmesh/NavmeshData.h"
#include "Engine/Entity/EntityDescriptors.h"
#include "Engine/Navmesh/Components/Component_Navmesh.h"
#include "Base/FileSystem/FileSystem.h"
#include "Base/Serialization/BinarySerialization.h"
#include "Base/Time/Timers.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include <filesystem>

//-------------------------------------------------------------------------

namespace EE::Navmesh
{
    NavmeshCompiler::NavmeshCompiler()
        : Resource::Compiler( "NavmeshCompiler" )
    {
        AddOutputType<NavmeshData>();
    }

    uint64_t NavmeshCompiler::CalculateAdvancedUpToDateHash( ResourceID resourceID ) const
    {
        return 0;
    }

    Resource::CompilationResult NavmeshCompiler::Compile( Resource::CompileContext const& ctx ) const
    {
        if ( ctx.m_inputFilePath.Exists() )
        {
            Serialization::BinaryInputArchive archive;
            if ( archive.ReadFromFile( ctx.m_inputFilePath ) )
            {
                Resource::ResourceHeader header;
                archive << header;

                // Is we have a mismatched version, rebuild the source file
                #if EE_ENABLE_NAVPOWER
                if ( header.m_version != NavmeshData::s_version )
                {
                    if ( GenerateNavmesh( ctx, true ) == Resource::CompilationResult::Failure )
                    {
                        return Resource::CompilationResult::Failure;
                    }
                }
                #endif
            }
            else // Corrupted source file
            {
                Error( "Failed to read pre-generated navmesh file (%s)!", ctx.m_inputFilePath.c_str() );
                return Resource::CompilationResult::Failure;
            }

            //-------------------------------------------------------------------------

            std::error_code ec;
            std::filesystem::copy_options opts = std::filesystem::copy_options::overwrite_existing;
            std::filesystem::copy( ctx.m_inputFilePath.c_str(), ctx.m_outputFilePath.c_str(), opts, ec );
            return ( ec.value() == 0 ) ? CompilationSucceeded(ctx) : CompilationFailed(ctx);
        }
        else
        {
            return GenerateNavmesh( ctx, false );
        }
    }

    Resource::CompilationResult NavmeshCompiler::GenerateNavmesh( Resource::CompileContext const& ctx, bool updatePregeneratedNavmesh ) const
    {
        if ( updatePregeneratedNavmesh )
        {
            Warning( "Pre-generated navmesh file has an old version - rebuilding (%s)!", ctx.m_inputFilePath.c_str() );
        }
        else
        {
            Message( "No pre-generated navmesh found, generating navmesh!" );
        }

        // Get map data
        //-------------------------------------------------------------------------

        FileSystem::Path mapPath = ctx.m_inputFilePath;
        mapPath.ReplaceExtension( "map" );

        if ( !mapPath.Exists() )
        {
            Error( "Entity map file (%s) doesnt exist!", mapPath.c_str() );
            return Resource::CompilationResult::Failure;
        }

        EntityModel::EntityMapDescriptor serializedMap;

        Milliseconds elapsedTime = 0.0f;
        {
            ScopedTimer<PlatformClock> timer( elapsedTime );

            if ( !EntityModel::ReadMapDescriptorFromFile( *m_pTypeRegistry, mapPath, serializedMap ) )
            {
                Error( "Entity map file (%s) is malformed!", mapPath.c_str() );
                return Resource::CompilationResult::Failure;
            }
        }

        Message( "Entity map read in: %.2fms", elapsedTime.ToFloat() );

        // Get navmesh component
        //-------------------------------------------------------------------------

        auto const navmeshComponents = serializedMap.GetComponentsOfType<NavmeshComponent>( *m_pTypeRegistry, false );
        if ( navmeshComponents.empty() )
        {
            Error( "Requesting navmesh for a map without a navmesh component! This is an invalid operation!" );
            return CompilationFailed( ctx );
        }

        if ( navmeshComponents.size() > 1 )
        {
            Warning( "More than one navmesh component found in this map, this is not supported... Ignoring all components apart from the first found!" );
        }

        auto pNavmeshComponent = navmeshComponents[0].m_pComponent->CreateType<Navmesh::NavmeshComponent>( *m_pTypeRegistry );
        EE_ASSERT( pNavmeshComponent != nullptr );
        Navmesh::NavmeshBuildSettings const buildSettings = pNavmeshComponent->GetBuildSettings();
        EE::Delete( pNavmeshComponent );

        // Generate navmesh
        //-------------------------------------------------------------------------

        #if EE_ENABLE_NAVPOWER
        Navmesh::NavmeshGenerator generator( *m_pTypeRegistry, m_sourceDataDirectoryPath, updatePregeneratedNavmesh ? ctx.m_inputFilePath : ctx.m_outputFilePath, serializedMap, buildSettings );

        {
            ScopedTimer<PlatformClock> timer( elapsedTime );
            generator.GenerateSync();
        }

        Message( "Navmesh built in: %.2fms", elapsedTime.ToFloat() );

        return Resource::CompilationResult::Success;
        #else
        return Error( "No navmesh middleware present!" );
        #endif
    }
}