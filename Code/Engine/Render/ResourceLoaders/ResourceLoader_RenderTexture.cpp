#include "ResourceLoader_RenderTexture.h"
#include "Base/Serialization/BinarySerialization.h"
#include "Base/FileSystem/FileSystem.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    TextureLoader::TextureLoader() : m_pRenderDevice( nullptr )
    {
        m_loadableTypes.push_back( Texture::GetStaticResourceTypeID() );
        m_loadableTypes.push_back( CubemapTexture::GetStaticResourceTypeID() );
    }

    bool TextureLoader::Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const
    {
        EE_ASSERT( m_pRenderDevice != nullptr );

        Texture* pTextureResource = nullptr;

        if ( resourceID.GetResourceTypeID() == Texture::GetStaticResourceTypeID() )
        {
            pTextureResource = EE::New<Texture>();
            archive << *pTextureResource;
        }
        else if ( resourceID.GetResourceTypeID() == CubemapTexture::GetStaticResourceTypeID() )
        {
            auto pCubemapTextureResource = EE::New<CubemapTexture>();
            archive << *pCubemapTextureResource;
            EE_ASSERT( pCubemapTextureResource->m_format == TextureFormat::DDS );
            pTextureResource = pCubemapTextureResource;
        }
        else // Unknown type
        {
            EE_UNREACHABLE_CODE();
        }

        pResourceRecord->SetResourceData( pTextureResource );
        return true;
    }

    Resource::InstallResult TextureLoader::Install( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::InstallDependencyList const& installDependencies, Resource::ResourceRecord* pResourceRecord ) const
    {
        auto pTextureResource = pResourceRecord->GetResourceData<Texture>();

        EE_ASSERT( pTextureResource->RequiresAdditionalDataFile() );

        Blob textureData;
        FileSystem::Path const additionalDataFilePath = Resource::IResource::GetAdditionalDataFilePath( resourcePath );
        if ( !FileSystem::ReadBinaryFile( additionalDataFilePath, textureData ) )
        {
            EE_LOG_ERROR( "Render", "Texture Loader", "Failed to load texture resource: %s, failed to load binary data!", resourceID.ToString().c_str() );
            return Resource::InstallResult::Failed;
        }

        m_pRenderDevice->LockDevice();
        m_pRenderDevice->CreateDataTexture( *pTextureResource, pTextureResource->m_format, textureData );
        m_pRenderDevice->UnlockDevice();

        ResourceLoader::Install( resourceID, resourcePath, installDependencies, pResourceRecord );
        return Resource::InstallResult::Succeeded;
    }

    void TextureLoader::Uninstall( ResourceID const& resourceID, Resource::ResourceRecord* pResourceRecord ) const
    {
        auto pTextureResource = pResourceRecord->GetResourceData<Texture>();
        if ( pTextureResource != nullptr && pTextureResource->IsValid() )
        {
            m_pRenderDevice->LockDevice();
            m_pRenderDevice->DestroyTexture( *pTextureResource );
            m_pRenderDevice->UnlockDevice();
        }
    }

    Resource::InstallResult TextureLoader::UpdateInstall( ResourceID const& resourceID, Resource::ResourceRecord* pResourceRecord ) const
    {
        EE_UNIMPLEMENTED_FUNCTION();
        return Resource::InstallResult::Failed;
    }
}