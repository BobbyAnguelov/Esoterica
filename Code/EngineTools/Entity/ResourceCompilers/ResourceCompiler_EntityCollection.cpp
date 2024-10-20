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
        : Resource::Compiler( "EntityCollectionCompiler" )
    {
        AddOutputType<EntityCollection>();
    }

    Resource::CompilationResult EntityCollectionCompiler::Compile( Resource::CompileContext const& ctx ) const
    {
        EntityCollection entityCollection;

        //-------------------------------------------------------------------------
        // Read collection
        //-------------------------------------------------------------------------

        Milliseconds elapsedTime = 0.0f;
        {
            ScopedTimer<PlatformClock> timer( elapsedTime );

            if ( !ReadEntityCollectionFromFile( *m_pTypeRegistry, ctx.m_inputFilePath, entityCollection ) )
            {
                return Resource::CompilationResult::Failure;
            }
        }
        Message( "Entity collection read in: %.2fms", elapsedTime.ToFloat() );

        //-------------------------------------------------------------------------
        // Serialize
        //-------------------------------------------------------------------------

        Serialization::BinaryOutputArchive archive;
        archive << Resource::ResourceHeader( EntityCollection::s_version, EntityCollection::GetStaticResourceTypeID(), ctx.m_sourceResourceHash, ctx.m_advancedUpToDateHash ) << entityCollection;
        
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
        EE_ASSERT( resourceID.GetResourceTypeID() == EntityCollection::GetStaticResourceTypeID() );

        EntityModel::EntityCollection entityCollection;

        // Read map descriptor
        FileSystem::Path const collectionFilePath = resourceID.GetFileSystemPath( m_sourceDataDirectoryPath );
        if ( !ReadEntityCollectionFromFile( *m_pTypeRegistry, collectionFilePath, entityCollection ) )
        {
            return false;
        }

        // Get all referenced resources
        TVector<ResourceID> referencedResources;
        entityCollection.GetAllReferencedResources( referencedResources );

        // Enqueue resources for compilation
        for ( auto const& referencedResourceID : referencedResources )
        {
            VectorEmplaceBackUnique( outReferencedResources, referencedResourceID );
        }

        return true;
    }
}