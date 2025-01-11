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

    Resource::ResourceLoader::LoadResult TextureLoader::Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const
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

        //-------------------------------------------------------------------------

        Blob textureData;
        FileSystem::Path const additionalDataFilePath = Resource::IResource::GetAdditionalDataFilePath( resourcePath );
        if ( !FileSystem::ReadBinaryFile( additionalDataFilePath, textureData ) )
        {
            EE_LOG_ERROR( "Render", "Texture Loader", "Failed to load texture resource: %s, failed to load binary data!", resourceID.ToString().c_str() );
            return Resource::ResourceLoader::LoadResult::Failed;
        }

        m_pRenderDevice->LockDevice();
        m_pRenderDevice->CreateDataTexture( *pTextureResource, pTextureResource->m_format, textureData );
        m_pRenderDevice->UnlockDevice();

        //-------------------------------------------------------------------------

        return Resource::ResourceLoader::LoadResult::Succeeded;
    }

    void TextureLoader::Unload( ResourceID const& resourceID, Resource::ResourceRecord* pResourceRecord ) const
    {
        auto pTextureResource = pResourceRecord->GetResourceData<Texture>();
        if ( pTextureResource != nullptr && pTextureResource->IsValid() )
        {
            m_pRenderDevice->LockDevice();
            m_pRenderDevice->DestroyTexture( *pTextureResource );
            m_pRenderDevice->UnlockDevice();
        }

        ResourceLoader::Unload( resourceID, pResourceRecord );
    }
}