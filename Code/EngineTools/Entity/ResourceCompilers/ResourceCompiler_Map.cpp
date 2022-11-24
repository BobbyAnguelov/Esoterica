#include "ResourceCompiler_Map.h"
#include "EngineTools/Entity/EntitySerializationTools.h"
#include "Engine/Entity/EntityDescriptors.h"
#include "Engine/Navmesh/Components/Component_Navmesh.h"
#include "System/TypeSystem/TypeRegistry.h"
#include "System/Serialization/BinarySerialization.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Time/Timers.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    EntityMapCompiler::EntityMapCompiler()
        : Resource::Compiler( "EntityMapCompiler", s_version )
    {
        m_outputTypes.push_back( SerializedEntityMap::GetStaticResourceTypeID() );
    }

    Resource::CompilationResult EntityMapCompiler::Compile( Resource::CompileContext const& ctx ) const
    {
        SerializedEntityMap map;

        //-------------------------------------------------------------------------
        // Read collection
        //-------------------------------------------------------------------------

        Milliseconds elapsedTime = 0.0f;
        {
            ScopedTimer<PlatformClock> timer( elapsedTime );

            if ( !ReadSerializedEntityMapFromFile( *m_pTypeRegistry, ctx.m_inputFilePath, map ) )
            {
                return Resource::CompilationResult::Failure;
            }
        }
        Message( "Entity map read in: %.2fms", elapsedTime.ToFloat() );

        //-------------------------------------------------------------------------
        // Component Modifications
        //-------------------------------------------------------------------------

        // We need to set the navmesh component resource ptr to the appropriate resource ID
        auto const navmeshComponents = map.GetComponentsOfType<Navmesh::NavmeshComponent>( *m_pTypeRegistry, false );
        if ( !navmeshComponents.empty() )
        {
            if ( navmeshComponents.size() > 1 )
            {
                Warning( "More than one navmesh component found in this map, this is not supported... Ignoring all components apart from the first found!" );
            }

            // TODO: see if there is a smart way to avoid using strings for property access
            TypeSystem::PropertyPath const navmeshResourcePropertyPath( "m_pNavmeshData" );

            // Remove any values for the navmesh resource property
            auto pNavmeshComponentDesc = navmeshComponents[0].m_pComponent;
            pNavmeshComponentDesc->RemovePropertyValue( navmeshResourcePropertyPath );

            // Set navmesh resource ptr
            ResourcePath navmeshResourcePath = ctx.m_resourceID.GetResourcePath();
            navmeshResourcePath.ReplaceExtension( Navmesh::NavmeshData::GetStaticResourceTypeID().ToString() );
            pNavmeshComponentDesc->m_properties.emplace_back( TypeSystem::PropertyDescriptor( *m_pTypeRegistry, navmeshResourcePropertyPath, GetCoreTypeID( TypeSystem::CoreTypeID::TResourcePtr ), TypeSystem::TypeID(), navmeshResourcePath.GetString() ) );
        }

        //-------------------------------------------------------------------------
        // Serialize
        //-------------------------------------------------------------------------

        Serialization::BinaryOutputArchive archive;
        archive << Resource::ResourceHeader( s_version, SerializedEntityMap::GetStaticResourceTypeID() ) << map;

        if ( archive.WriteToFile( ctx.m_outputFilePath ) )
        {
            return CompilationSucceeded( ctx );
        }
        else
        {
            return CompilationFailed( ctx );
        }
    }

    bool EntityMapCompiler::GetInstallDependencies( ResourceID const& resourceID, TVector<ResourceID>& outReferencedResources ) const
    {
        EE_ASSERT( resourceID.GetResourceTypeID() == SerializedEntityMap::GetStaticResourceTypeID() );

        SerializedEntityMap map;

        // Read map descriptor
        FileSystem::Path const collectionFilePath = resourceID.GetResourcePath().ToFileSystemPath( m_rawResourceDirectoryPath );
        if ( !ReadSerializedEntityMapFromFile( *m_pTypeRegistry, collectionFilePath, map ) )
        {
            return false;
        }

        // Get all referenced resources
        TVector<ResourceID> referencedResources;
        map.GetAllReferencedResources( referencedResources );

        // Enqueue resources for compilation
        for ( auto const& referencedResourceID : referencedResources )
        {
            VectorEmplaceBackUnique( outReferencedResources, referencedResourceID );
        }

        return true;
    }
}