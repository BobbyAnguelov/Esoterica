#include "ImguiImageCache.h"

#if EE_DEVELOPMENT_TOOLS
#include "Base/Render/RenderDevice.h"
#include "Base/Render/RenderUtils.h"

//-------------------------------------------------------------------------

namespace EE::ImGuiX
{
    ImageCache::~ImageCache()
    {
        EE_ASSERT( m_pRenderDevice == nullptr );
        EE_ASSERT( m_loadedImages.empty() );
    }

    void ImageCache::Initialize( Render::RenderDevice* pRenderDevice )
    {
        EE_ASSERT( m_pRenderDevice == nullptr );
        EE_ASSERT( pRenderDevice != nullptr );
        m_pRenderDevice = pRenderDevice;
    }

    void ImageCache::Shutdown()
    {
        EE_ASSERT( m_pRenderDevice != nullptr );

        for ( auto& imgInfo : m_loadedImages )
        {
            m_pRenderDevice->DestroyTexture( *imgInfo.m_pTexture );
            EE::Delete( imgInfo.m_pTexture );
        }

        m_loadedImages.clear();
        m_pRenderDevice = nullptr;
    }

    ImageInfo ImageCache::LoadImageFromFile( FileSystem::Path const& path )
    {
        EE_ASSERT( m_pRenderDevice != nullptr );
        EE_ASSERT( path.Exists() );

        //-------------------------------------------------------------------------

        auto pTexture = EE::New<Render::Texture>();
        if ( !Render::Utils::CreateTextureFromFile( m_pRenderDevice, path, *pTexture ) )
        {
            EE_HALT();
        }

        //-------------------------------------------------------------------------

        ImageInfo info;
        info.m_ID = (void*) &pTexture->GetShaderResourceView();
        info.m_pTexture = pTexture;
        info.m_size = Float2( pTexture->GetDimensions() );

        m_loadedImages.emplace_back( info );

        //-------------------------------------------------------------------------

        return info;
    }

    ImageInfo ImageCache::LoadImageFromMemoryBase64( uint8_t const* pData, size_t size )
    {
        EE_ASSERT( m_pRenderDevice != nullptr );
        EE_ASSERT( pData != nullptr && size > 0 );

        //-------------------------------------------------------------------------

        auto pTexture = EE::New<Render::Texture>();
        if ( !Render::Utils::CreateTextureFromBase64( m_pRenderDevice, pData, size, *pTexture ) )
        {
            EE_HALT();
        }

        //-------------------------------------------------------------------------

        ImageInfo info;
        info.m_ID = (void*) &pTexture->GetShaderResourceView();
        info.m_pTexture = pTexture;
        info.m_size = Float2( pTexture->GetDimensions() );

        m_loadedImages.emplace_back( info );

        //-------------------------------------------------------------------------

        return info;
    }

    void ImageCache::UnloadImage( ImageInfo& info )
    {
        EE_ASSERT( m_pRenderDevice != nullptr );

        //-------------------------------------------------------------------------

        for ( auto i = 0; i < m_loadedImages.size(); i++ )
        {
            if ( m_loadedImages[i].m_ID == info.m_ID )
            {
                m_pRenderDevice->DestroyTexture( *m_loadedImages[i].m_pTexture );
                EE::Delete( m_loadedImages[i].m_pTexture );
                m_loadedImages.erase_unsorted( m_loadedImages.begin() + i );

                //-------------------------------------------------------------------------

                info.m_ID = 0;
                info.m_pTexture = nullptr;
                info.m_size = ImVec2( 0, 0 );

                return;
            }
        }

        EE_UNREACHABLE_CODE();
    }

    ImTextureID GetImTextureID( Render::Texture const* pTexture )
    {
        return (void*) &pTexture->GetShaderResourceView();
    }
}
#endif