#include "ResourceLoader.h"
#include "ResourceHeader.h"
#include "Base/FileSystem/FileSystem.h"
#include "Base/Serialization/BinarySerialization.h"
#include "Base/Time/Timers.h"
#include "Base/Profiling.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    ResourceLoader::LoadResult ResourceLoader::Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, ResourceRecord* pResourceRecord ) const
    {
        // Read file and create archive
        //-------------------------------------------------------------------------

        Blob rawResourceData;
        Serialization::BinaryInputArchive archive;

        {
            EE_PROFILE_SCOPE_IO( "Read File" );

            #if EE_DEVELOPMENT_TOOLS
            ScopedTimer<PlatformClock> timer( pResourceRecord->m_fileReadTime );
            #endif

            if ( !FileSystem::ReadBinaryFile( resourcePath, rawResourceData ) )
            {
                EE_LOG_ERROR( "Resource", "Resource Loader", "Failed to read resource file (%s)", resourceID.c_str() );
                return LoadResult::Failed;
            }

            archive.ReadFromBlob( rawResourceData );
        }

        // Load contents from raw file data
        //-------------------------------------------------------------------------

        {
            EE_PROFILE_SCOPE_IO( "Deserialize File" );

            #if EE_DEVELOPMENT_TOOLS
            ScopedTimer<PlatformClock> timer( pResourceRecord->m_loadTime );
            #endif

            Resource::ResourceHeader header;
            archive << header;

            #if EE_DEVELOPMENT_TOOLS
            pResourceRecord->m_sourceResourceHash = header.m_sourceResourceHash;
            #endif

            // Set all install dependencies
            pResourceRecord->m_installDependencyResourceIDs.reserve( header.m_installDependencies.size() );
            for ( auto const& depResourceID : header.m_installDependencies )
            {
                pResourceRecord->m_installDependencyResourceIDs.push_back( depResourceID );
            }

            // Perform resource load
            LoadResult loadResult = Load( resourceID, resourcePath, pResourceRecord, archive );
            if ( loadResult == LoadResult::Failed )
            {
                EE_LOG_ERROR( "Resource", "Resource Loader", "Failed to load resource: %s", resourceID.c_str() );
            }
            return loadResult;
        }

        // Loaders must always set a valid resource data ptr, even if the resource internally is invalid
        // This is enforced to prevent leaks from occurring when a loader allocates a resource, then tries to 
        // load it unsuccessfully and then forgets to release the allocated data.
        EE_ASSERT( pResourceRecord->GetResourceData() != nullptr );
        return LoadResult::Succeeded;
    }

    ResourceLoader::LoadResult ResourceLoader::UpdateLoad( ResourceID const& resourceID, FileSystem::Path const& resourcePath, ResourceRecord* pResourceRecord ) const
    {
        EE_UNREACHABLE_CODE();
        return LoadResult::Failed;
    }

    void ResourceLoader::Unload( ResourceID const& resourceID, ResourceRecord* pResourceRecord ) const
    {
        EE_ASSERT( pResourceRecord != nullptr );
        EE_ASSERT( pResourceRecord->IsUnloading() || pResourceRecord->HasLoadingFailed() );

        IResource* pData = pResourceRecord->GetResourceData();
        EE::Delete( pData );
        pResourceRecord->SetResourceData( nullptr );
        pResourceRecord->m_installDependencyResourceIDs.clear();
    }

    ResourceLoader::LoadResult ResourceLoader::Install( ResourceID const& resourceID, InstallDependencyList const& installDependencies, ResourceRecord* pResourceRecord ) const
    {
        EE_ASSERT( pResourceRecord != nullptr );
        pResourceRecord->m_pResource->m_resourceID = resourceID;

        #if EE_DEVELOPMENT_TOOLS
        pResourceRecord->m_pResource->m_sourceResourceHash = pResourceRecord->m_sourceResourceHash;
        #endif 

        return LoadResult::Succeeded;
    }

    ResourceLoader::LoadResult ResourceLoader::UpdateInstall( ResourceID const& resourceID, ResourceRecord* pResourceRecord ) const
    {
        EE_UNREACHABLE_CODE();
        return LoadResult::Failed;
    }
}