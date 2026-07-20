#include "ResourceProvider_Package.h"
#include "Base/Resource/ResourceRequest.h"
#include "Base/Resource/Settings/Settings_Resource.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    bool PackagedResourceProvider::IsReady() const
    {
        return true;
    }

    bool PackagedResourceProvider::Initialize()
    {
        return true;
    }

    void PackagedResourceProvider::RequestRawResource( ResourceRequest* pRequest )
    {
        ResourceID const& resourceID = pRequest->GetResourceID();
        FileSystem::Path const resourceFilePath = resourceID.GetCompiledFileSystemPath( m_settings.m_compiledResourceDirectoryPath );
        pRequest->OnRawResourceRequestComplete( resourceFilePath.c_str(), String() );
    }

    void PackagedResourceProvider::CancelRequest( ResourceRequest* pRequest )
    {
         // Do Nothing
    }
}