#include "PackagedResourceProvider.h"
#include "Base/Resource/ResourceRequest.h"
#include "Base/Resource/Settings/GlobalSettings_Resource.h"

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
        pRequest->OnRawResourceRequestComplete( resourceFilePath.c_str(), String() );
    }

    void PackagedResourceProvider::CancelRequest( ResourceRequest* pRequest )
    {
         // Do Nothing
    }
}