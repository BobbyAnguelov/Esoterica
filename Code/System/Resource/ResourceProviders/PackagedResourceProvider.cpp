#include "PackagedResourceProvider.h"
#include "System/Resource/ResourceRequest.h"
#include "System/Resource/ResourceSettings.h"


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
        FileSystem::Path const resourceFilePath = pRequest->GetResourceID().GetResourcePath().ToFileSystemPath( m_settings.m_compiledResourcePath );
        pRequest->OnRawResourceRequestComplete( resourceFilePath.c_str() );
    }

    void PackagedResourceProvider::CancelRequest( ResourceRequest* pRequest )
    {
         // Do Nothing
    }
}