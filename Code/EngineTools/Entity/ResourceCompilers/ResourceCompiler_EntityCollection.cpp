#include "ResourceCompiler_EntityCollection.h"
#include "Engine/Entity/EntityDescriptors.h"
#include "Engine/Entity/EntitySerialization.h"
#include "System/TypeSystem/TypeRegistry.h"
#include "System/Serialization/BinarySerialization.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Time/Timers.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    EntityCollectionCompiler::EntityCollectionCompiler()
        : Resource::Compiler( "EntityCollectionCompiler", s_version )
    {
        m_outputTypes.push_back( EntityCollectionDescriptor::GetStaticResourceTypeID() );
    }

    Resource::CompilationResult EntityCollectionCompiler::Compile( Resource::CompileContext const& ctx ) const
    {
        EntityCollectionDescriptor collectionDesc;

        //-------------------------------------------------------------------------
        // Read collection
        //-------------------------------------------------------------------------

        Milliseconds elapsedTime = 0.0f;
        {
            ScopedTimer<PlatformClock> timer( elapsedTime );

            if ( !Serializer::ReadEntityCollectionFromFile( *m_pTypeRegistry, ctx.m_inputFilePath, collectionDesc ) )
            {
                return Resource::CompilationResult::Failure;
            }
        }
        Message( "Entity collection read in: %.2fms", elapsedTime.ToFloat() );

        //-------------------------------------------------------------------------
        // Serialize
        //-------------------------------------------------------------------------

        Serialization::BinaryOutputArchive archive;
        archive << Resource::ResourceHeader( s_version, EntityCollectionDescriptor::GetStaticResourceTypeID() ) << collectionDesc;
        
        if ( archive.WriteToFile( ctx.m_outputFilePath ) )
        {
            return CompilationSucceeded( ctx );
        }
        else
        {
            return CompilationFailed( ctx );
        }
    }

    bool EntityCollectionCompiler::GetReferencedResources( ResourceID const& resourceID, TVector<ResourceID>& outReferencedResources ) const
    {
        EE_ASSERT( resourceID.GetResourceTypeID() == EntityCollectionDescriptor::GetStaticResourceTypeID() );

        EntityModel::EntityCollectionDescriptor collectionDesc;

        // Read map descriptor
        FileSystem::Path const collectionFilePath = resourceID.GetResourcePath().ToFileSystemPath( m_rawResourceDirectoryPath );
        if ( !EntityModel::Serializer::ReadEntityCollectionFromFile( *m_pTypeRegistry, collectionFilePath, collectionDesc ) )
        {
            return false;
        }

        // Get all referenced resources
        TVector<ResourceID> referencedResources;
        collectionDesc.GetAllReferencedResources( referencedResources );

        // Enqueue resources for compilation
        for ( auto const& referencedResourceID : referencedResources )
        {
            VectorEmplaceBackUnique( outReferencedResources, referencedResourceID );
        }

        return true;
    }
}