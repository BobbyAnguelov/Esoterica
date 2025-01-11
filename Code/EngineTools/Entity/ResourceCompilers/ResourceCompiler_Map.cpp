#include "ResourceCompiler_Map.h"
#include "EngineTools/Entity/EntitySerializationTools.h"
#include "Engine/Entity/EntityDescriptors.h"
#include "Engine/Navmesh/Components/Component_Navmesh.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "Base/Serialization/BinarySerialization.h"
#include "Base/FileSystem/FileSystem.h"
#include "Base/Time/Timers.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    EntityMapCompiler::EntityMapCompiler()
        : Resource::Compiler( "EntityMapCompiler" )
    {
        AddOutputType<EntityMapDescriptor>();
    }

    Resource::CompilationResult EntityMapCompiler::Compile( Resource::CompileContext const& ctx ) const
    {
        EntityMapDescriptor map;

        //-------------------------------------------------------------------------
        // Read collection
        //-------------------------------------------------------------------------

        Milliseconds elapsedTime = 0.0f;
        {
            ScopedTimer<PlatformClock> timer( elapsedTime );

            if ( !ReadMapDescriptorFromFile( *m_pTypeRegistry, ctx.m_inputFilePath, map ) )
            {
                return Resource::CompilationResult::Failure;
            }
        }
        Message( "Entity map read in: %.2fms", elapsedTime.ToFloat() );

        //-------------------------------------------------------------------------
        // Sanitize collection
        //-------------------------------------------------------------------------

        bool const isCompilingForPackagedBuild = ctx.IsCompilingForPackagedBuild();
        TVector<EntityDescriptor>& descriptors = map.GetMutableEntityDescriptors();
        for ( int32_t e = int32_t( descriptors.size() ) - 1; e >= 0; e-- )
        {
            // Remove invalid and dev-only components
            for ( int32_t c = int32_t( descriptors[e].m_components.size() ) - 1; c >= 0; c-- )
            {
                ComponentDescriptor& componentDesc = descriptors[e].m_components[c];
                TypeSystem::TypeInfo const* pComponentTypeInfo = m_pTypeRegistry->GetTypeInfo( componentDesc.m_typeID );

                if ( pComponentTypeInfo == nullptr || ( isCompilingForPackagedBuild && pComponentTypeInfo->m_isForDevelopmentUseOnly ) )
                {
                    descriptors[e].m_components.erase( descriptors[e].m_components.begin() + c );
                }
            }

            // Remove invalid and dev-only systems
            for ( int32_t s = int32_t( descriptors[e].m_systems.size() ) - 1; s >= 0; s-- )
            {
                SystemDescriptor& systemDesc = descriptors[e].m_systems[s];
                TypeSystem::TypeInfo const* pSystemTypeInfo = m_pTypeRegistry->GetTypeInfo( systemDesc.m_typeID );
                if ( pSystemTypeInfo == nullptr || ( isCompilingForPackagedBuild && pSystemTypeInfo->m_isForDevelopmentUseOnly ) )
                {
                    descriptors[e].m_systems.erase( descriptors[e].m_systems.begin() + s );
                }
            }

            // Remove any empty entities
            if ( ( descriptors[e].m_components.size() + descriptors[e].m_systems.size() ) == 0 )
            {
                descriptors.erase( descriptors.begin() + e );
            }
        }

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
            TypeSystem::PropertyPath const navmeshResourcePropertyPath( "m_navmeshData" );

            // Remove any values for the navmesh resource property
            ComponentDescriptor* pNavmeshComponentDesc = navmeshComponents[0].m_pComponent;
            pNavmeshComponentDesc->RemovePropertyValue( navmeshResourcePropertyPath );

            // Set navmesh resource ptr
            DataPath navmeshResourcePath = ctx.m_resourceID.GetDataPath();
            navmeshResourcePath.ReplaceExtension( Navmesh::NavmeshData::GetStaticResourceTypeID().ToString() );

            TypeSystem::PropertyDescriptor navmeshPtrPropertyDesc;
            navmeshPtrPropertyDesc.m_path = navmeshResourcePropertyPath;
            navmeshPtrPropertyDesc.m_stringValue = navmeshResourcePath.GetString();
            TypeSystem::Conversion::ConvertStringToBinary( *m_pTypeRegistry, GetCoreTypeID( TypeSystem::CoreTypeID::TResourcePtr ), TypeSystem::TypeID(), navmeshResourcePath.GetString(), navmeshPtrPropertyDesc.m_byteValue );

            pNavmeshComponentDesc->m_properties.emplace_back( navmeshPtrPropertyDesc );
        }

        //-------------------------------------------------------------------------
        // Serialize
        //-------------------------------------------------------------------------

        Resource::ResourceHeader hdr( EntityMapDescriptor::s_version, EntityMapDescriptor::GetStaticResourceTypeID(), ctx.m_sourceResourceHash, ctx.m_advancedUpToDateHash );
        Serialization::BinaryOutputArchive archive;
        archive << hdr << map;

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
        EE_ASSERT( resourceID.GetResourceTypeID() == EntityMapDescriptor::GetStaticResourceTypeID() );

        EntityMapDescriptor map;

        // Read map descriptor
        FileSystem::Path const collectionFilePath = resourceID.GetFileSystemPath( m_sourceDataDirectoryPath );
        if ( !ReadMapDescriptorFromFile( *m_pTypeRegistry, collectionFilePath, map ) )
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