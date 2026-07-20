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

    enum class LoadResult
    {
        Complete,
        InProgress,
        Failed,
    };

    enum class UnloadResult
    {
        Complete,
        InProgress,
    };

    //-------------------------------------------------------------------------

    class EE_BASE_API ResourceLoader
    {
        friend class ResourceRequest;

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

            // Cant find a requested install dependency, are you sure you correctly registered it with the resource header when compiling?
            EE_UNREACHABLE_CODE();
            return nullptr;
        }

        inline static void LogError( ResourceRecord* pResourceRecord, char const* pFormat, ... )
        {
            #if EE_DEVELOPMENT_TOOLS
            va_list args;
            va_start( args, pFormat );

            InlineString msg;
            msg.sprintf_va_list( pFormat, args );

            pResourceRecord->m_errorLog.append( msg.c_str() );
            EE_LOG_ERROR( LogCategory::Resource, "Resource Loader", msg.c_str() );

            va_end( args );
            #endif
        }

    public:

        virtual ~ResourceLoader() = default;

        TVector<ResourceTypeID> const& GetLoadableTypes() const { return m_loadableTypes; }

        // Can this loader proceed when an install dependency fails to load? Certain resource should still be loaded if some of their dependencies fail (i.e. still load a mesh if a material fails to load)
        virtual bool CanProceedWithFailedInstallDependency() const { return false; }

        // This function loads is responsible to deserialize the compiled resource data, read the resource header for install dependencies and to create the new runtime resource object
        // This function will continue being called until either "LoadResult::Succeeded" or "LoadResult::Failed" is returned
        LoadResult Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, ResourceRecord* pResourceRecord ) const;

    protected:

        // (Required) Override this function to implement you custom deserialization and creation logic, resource header has already been read at this point
        // Return "LoadResult::InProgress" if you require a multi-stage load
        virtual LoadResult Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive* pArchive ) const = 0;

        // This function is called once all the install dependencies have been loaded, it allows us to update any internal resource ptrs the resource might hold
        // This function will continue being called until either "LoadResult::Succeeded" or "LoadResult::Failed" is returned
        virtual LoadResult Install( ResourceID const& resourceID, InstallDependencyList const& installDependencies, ResourceRecord* pResourceRecord ) const
        {
            return LoadResult::Complete;
        }

        // Called once we complete the install stage
        void OnInstallComplete( ResourceID const& resourceID, ResourceRecord* pResourceRecord );

        // This function is called as a first step when we are about to uninstall a resource, it allows us to clean up anything that might require an install dependency to be available
        // This function will continue being called until "UnloadResult::Complete" is returned
        virtual UnloadResult Uninstall( ResourceID const& resourceID, ResourceRecord* pResourceRecord ) const
        {
            return UnloadResult::Complete;
        }

        // This function will destroy the created resource object
        // This function will continue being called until "UnloadResult::Complete" is returned
        // (Optional) Override this function to implement any custom object destruction logic if needed, by default this will just delete the created resource
        virtual UnloadResult Unload( ResourceID const& resourceID, ResourceRecord* pResourceRecord ) const
        {
            return UnloadResult::Complete;
        }

        // Called once we complete the unload stage
        void OnUnloadComplete( ResourceID const& resourceID, ResourceRecord* pResourceRecord );

    protected:

        TVector<ResourceTypeID>          m_loadableTypes;
    };
}