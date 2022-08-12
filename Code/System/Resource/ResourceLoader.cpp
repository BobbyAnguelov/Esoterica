#include "ResourceLoader.h"
#include "ResourceHeader.h"
#include "System/Serialization/BinarySerialization.h"
#include "System/Log.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    bool ResourceLoader::Load( ResourceID const& resourceID, Blob& rawData, ResourceRecord* pResourceRecord ) const
    {
        Serialization::BinaryInputArchive archive;
        archive.ReadFromBlob( rawData );

        // Read resource header
        Resource::ResourceHeader header;
        archive << header;

        // Set all install dependencies
        pResourceRecord->m_installDependencyResourceIDs.reserve( header.m_installDependencies.size() );
        for ( auto const& depResourceID : header.m_installDependencies )
        {
            pResourceRecord->m_installDependencyResourceIDs.push_back( depResourceID );
        }

        // Perform resource load
        if ( !LoadInternal( resourceID, pResourceRecord, archive ) )
        {
            EE_LOG_ERROR( "Resource", "Resource Loader", "Failed to load resource: %s", resourceID.c_str() );
            return false;
        }

        // Loaders must always set a valid resource data ptr, even if the resource internally is invalid
        // This is enforced to prevent leaks from occurring when a loader allocates a resource, then tries to 
        // load it unsuccessfully and then forgets to release the allocated data.
        EE_ASSERT( pResourceRecord->GetResourceData() != nullptr );
        return true;
    }

    InstallResult ResourceLoader::Install( ResourceID const& resourceID, ResourceRecord* pResourceRecord, InstallDependencyList const& installDependencies ) const
    {
        EE_ASSERT( pResourceRecord != nullptr );
        pResourceRecord->m_pResource->m_resourceID = resourceID;
        return InstallResult::Succeeded;
    }

    InstallResult ResourceLoader::UpdateInstall( ResourceID const& resourceID, ResourceRecord* pResourceRecord ) const
    {
        // This function should never be called directly!!
        // If your resource requires multi-frame installation, you need to override this function in your loader and return InstallResult::InProgress from the install function!
        EE_UNREACHABLE_CODE();
        return InstallResult::Succeeded;
    }

    void ResourceLoader::Unload( ResourceID const& resourceID, ResourceRecord* pResourceRecord ) const
    {
        EE_ASSERT( pResourceRecord != nullptr );
        EE_ASSERT( pResourceRecord->IsUnloading() || pResourceRecord->HasLoadingFailed() );
        UnloadInternal( resourceID, pResourceRecord );
        pResourceRecord->m_installDependencyResourceIDs.clear();
    }

    void ResourceLoader::UnloadInternal( ResourceID const& resourceID, ResourceRecord* pResourceRecord ) const
    {
        IResource* pData = pResourceRecord->GetResourceData();
        EE::Delete( pData );
        pResourceRecord->SetResourceData( nullptr );
    }
}