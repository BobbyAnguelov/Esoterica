#pragma once

#include "Base/_Module/API.h"
#include "ResourcePtr.h"
#include "IResource.h"

//-------------------------------------------------------------------------

namespace EE::Serialization { class BinaryInputArchive; }

// Resource Loader
//-------------------------------------------------------------------------
/*
    Resource loading has several stages:

    1) LOAD (first stage): read the compiled data from disk, request load of any install dependencies
    2) LOAD (subsequent stages): perform additional load operations, interface with RHI, load additional data from disk, decompress, etc...
    3) WAIT FOR INSTALL DEPS: wait for any dependencies to finish loading
    4) INSTALL: Get the list of loaded dependencies and perform any final operations (caching, generate LUTs, etc...) required before the resource is ready to use
*/

//-------------------------------------------------------------------------

namespace EE::Resource
{
    using InstallDependencyList = TInlineVector<ResourcePtr, 4>;

    //-------------------------------------------------------------------------

    class EE_BASE_API ResourceLoader
    {

    public:

        enum class LoadResult
        {
            Succeeded,
            InProgress,
            Failed,
        };

    protected:

        inline static ResourcePtr GetInstallDependency( InstallDependencyList const& installDependencies, ResourceID const& resourceID )
        {
            for ( ResourcePtr const& resourcePtr : installDependencies )
            {
                if ( resourcePtr.GetResourceID() == resourceID )
                {
                    return resourcePtr;
                }
            }

            EE_HALT();
            return nullptr;
        }

    public:

        virtual ~ResourceLoader() = default;

        TVector<ResourceTypeID> const& GetLoadableTypes() const { return m_loadableTypes; }

        // Can this loader proceed when an install dependency fails to load? Certain resource should still be loaded if some of their dependencies fail (i.e. still load a mesh if a material fails to load)
        virtual bool CanProceedWithFailedInstallDependency() const { return false; }

        // This function loads is responsible to deserialize the compiled resource data, read the resource header for install dependencies and to create the new runtime resource object
        LoadResult Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, ResourceRecord* pResourceRecord ) const;

        // This function is only called if the initial load return "LoadResult::InProgress". This is necessary for any multi-stage loads i.e. anything that requires RHI alloc/transfer/etc...
        // This function will continue being called until either "LoadResult::Succeeded" or "LoadResult::Failed" is returned
        virtual LoadResult UpdateLoad( ResourceID const& resourceID, FileSystem::Path const& resourcePath, ResourceRecord* pResourceRecord ) const;

        // This function will destroy the created resource object
        // (Optional) Override this function to implement any custom object destruction logic if needed, by default this will just delete the created resource
        virtual void Unload( ResourceID const& resourceID, ResourceRecord* pResourceRecord ) const;

        // This function is called once all the install dependencies have been loaded, it allows us to update any internal resource ptrs the resource might hold
        // Return "LoadResult::InProgress" if you require a multi-stage load
        virtual LoadResult Install( ResourceID const& resourceID, InstallDependencyList const& installDependencies, ResourceRecord* pResourceRecord ) const;

        // This function is only called if the initial install return "LoadResult::InProgress". This is necessary for any multi-stage installs i.e. anything that requires RHI alloc/transfer/etc...
        // This function will continue being called until either "LoadResult::Succeeded" or "LoadResult::Failed" is returned
        virtual LoadResult UpdateInstall( ResourceID const& resourceID, ResourceRecord* pResourceRecord ) const;

        // This function is called as a first step when we are about to unload a resource, it allows us to clean up anything that might require an install dependency to be available
        virtual void Uninstall( ResourceID const& resourceID, ResourceRecord* pResourceRecord ) const {}

    protected:

        // (Required) Override this function to implement you custom deserialization and creation logic, resource header has already been read at this point
        // Return "LoadResult::InProgress" if you require a multi-stage load
        virtual LoadResult Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const = 0;

    protected:

        TVector<ResourceTypeID>          m_loadableTypes;
    };
}