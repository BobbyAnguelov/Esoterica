#include "ResourceCompiler_EntityCollection.h"
#include "EngineTools/Entity/EntitySerializationTools.h"
#include "Engine/Entity/EntityDescriptors.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "Base/Serialization/BinarySerialization.h"
#include "Base/FileSystem/FileSystem.h"
#include "Base/Time/Timers.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    EntityCollectionCompiler::EntityCollectionCompiler()
        : Resource::Compiler( "EntityCollectionCompiler", s_version )
    {
        m_outputTypes.push_back( SerializedEntityCollection::GetStaticResourceTypeID() );
    }

    Resource::CompilationResult EntityCollectionCompiler::Compile( Resource::CompileContext const& ctx ) const
    {
        SerializedEntityCollection serializedCollection;

        //-------------------------------------------------------------------------
        // Read collection
        //-------------------------------------------------------------------------

        Milliseconds elapsedTime = 0.0f;
        {
            ScopedTimer<PlatformClock> timer( elapsedTime );

            if ( !ReadSerializedEntityCollectionFromFile( *m_pTypeRegistry, ctx.m_inputFilePath, serializedCollection ) )
            {
                return Resource::CompilationResult::Failure;
            }
        }
        Message( "Entity collection read in: %.2fms", elapsedTime.ToFloat() );

        //-------------------------------------------------------------------------
        // Serialize
        //-------------------------------------------------------------------------

        Serialization::BinaryOutputArchive archive;
        archive << Resource::ResourceHeader( s_version, SerializedEntityCollection::GetStaticResourceTypeID(), ctx.m_sourceResourceHash ) << serializedCollection;
        
        if ( archive.WriteToFile( ctx.m_outputFilePath ) )
        {
            return CompilationSucceeded( ctx );
        }
        else
        {
            return CompilationFailed( ctx );
        }
    }

    bool EntityCollectionCompiler::GetInstallDependencies( ResourceID const& resourceID, TVector<ResourceID>& outReferencedResources ) const
    {
        EE_ASSERT( resourceID.GetResourceTypeID() == SerializedEntityCollection::GetStaticResourceTypeID() );

        EntityModel::SerializedEntityCollection serializedCollection;

        // Read map descriptor
        FileSystem::Path const collectionFilePath = resourceID.GetResourcePath().ToFileSystemPath( m_rawResourceDirectoryPath );
        if ( !ReadSerializedEntityCollectionFromFile( *m_pTypeRegistry, collectionFilePath, serializedCollection ) )
        {
            return false;
        }

        // Get all referenced resources
        TVector<ResourceID> referencedResources;
        serializedCollection.GetAllReferencedResources( referencedResources );

        // Enqueue resources for compilation
        for ( auto const& referencedResourceID : referencedResources )
        {
            VectorEmplaceBackUnique( outReferencedResources, referencedResourceID );
        }

        return true;
    }
}