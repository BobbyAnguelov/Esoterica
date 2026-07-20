#include "ImguiImageCache.h"

#if EE_DEVELOPMENT_TOOLS
#include "Base/FileSystem/FileSystemPath.h"
#include "Base/Encoding/Encoding.h"
#include "Base/Render/RHI.h"
#include "Base/ThirdParty/stb/stb_image.h"
#include "Engine/Render/RenderSystem.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    static RHI::Texture* LoadTextureFromFile_Blocking( RenderSystem* pRenderSystem, FileSystem::Path const& path )
    {
        EE_ASSERT( pRenderSystem );
        EE_ASSERT( path.Exists() );

        //-------------------------------------------------------------------------

        int32_t  width, height, channels;
        uint8_t* pImage = stbi_load( path.c_str(), &width, &height, &channels, STBI_rgb_alpha );
        if ( pImage == nullptr )
        {
            return nullptr;
        }

        RHI::TextureParameters textureParameters = {};
        textureParameters.m_width = width;
        textureParameters.m_height = height;
        textureParameters.m_format = RHI::DataFormat::RGBA8_UNorm;
        textureParameters.m_debugName = path.c_str();

        auto CopyTextureMemory = [&] ( uint8_t* pDstMemory_WriteCombined, size_t srcOffset, uint32_t rowStride, uint32_t row )
        {
            Memory::CopyToWriteCombined( pDstMemory_WriteCombined, pImage + srcOffset, rowStride );
        };
        RHI::Texture* pTexture = pRenderSystem->QueueTextureCreate( CopyTextureMemory, textureParameters );

        stbi_image_free( pImage );

        return pTexture;
    }

    static RHI::Texture* CreateTextureFromBase64( RenderSystem* pRenderSystem, uint8_t const* pData, size_t size )
    {
        EE_ASSERT( pRenderSystem );
        EE_ASSERT( pData != nullptr && size > 0 );

        //-------------------------------------------------------------------------

        Blob const decodedData = Encoding::Base64::Decode( pData, size );

        //-------------------------------------------------------------------------

        int32_t  width, height, channels;
        uint8_t* pImage = stbi_load_from_memory( decodedData.data(), (int32_t) decodedData.size(), &width, &height, &channels, 4 );
        if ( pImage == nullptr )
        {
            return nullptr;
        }

        RHI::TextureParameters textureParameters = {};
        textureParameters.m_width = width;
        textureParameters.m_height = height;
        textureParameters.m_format = RHI::DataFormat::RGBA8_UNorm;
        textureParameters.m_debugName = "<<Texture From Memory>>";

        auto CopyTextureMemory = [&] ( uint8_t* pDstMemory_WriteCombined, size_t srcOffset, uint32_t rowStride, uint32_t row )
        {
            Memory::CopyToWriteCombined( pDstMemory_WriteCombined, pImage + srcOffset, rowStride );
        };
        RHI::Texture* pTexture = pRenderSystem->QueueTextureCreate( CopyTextureMemory, textureParameters );

        stbi_image_free( pImage );

        return pTexture;
    }
}

namespace EE::ImGuiX
{
    ImageCache::~ImageCache()
    {
        EE_ASSERT( !WasInitialized() );
        EE_ASSERT( m_loadedImages.empty() );
    }

    void ImageCache::Initialize( Render::RenderSystem* pRenderSystem )
    {
        EE_ASSERT( !WasInitialized() );
        m_pRenderSystem = pRenderSystem;
    }

    void ImageCache::Shutdown()
    {
        EE_ASSERT( WasInitialized() );

        m_loadedImages.clear();
        m_pRenderSystem = nullptr;
    }

    ImageInfo ImageCache::LoadImageFromFile( FileSystem::Path const& path )
    {
        EE_ASSERT( WasInitialized() );
        EE_ASSERT( path.Exists() );

        //-------------------------------------------------------------------------

        Render::RHI::Texture* pTexture = Render::LoadTextureFromFile_Blocking( m_pRenderSystem, path );
        EE_ASSERT( pTexture );

        //-------------------------------------------------------------------------

        ImageInfo info;
        info.m_ID = ImTextureID_Pack
        (
            0, // TODO: Provide correct sampler
            Render::RHI::GetTextureHandle( pTexture, Render::RHI::DescriptorTypeFlags::Texture, 0 )
        );
        info.m_size = Float2( float( pTexture->m_width ), float( pTexture->m_height ) );
        info.m_pTexture = pTexture;

        m_loadedImages.emplace_back( info );

        //-------------------------------------------------------------------------

        return info;
    }

    ImageInfo ImageCache::LoadImageFromMemoryBase64( uint8_t const* pData, size_t size )
    {
        EE_ASSERT( WasInitialized() );
        EE_ASSERT( pData != nullptr && size > 0 );

        //-------------------------------------------------------------------------

        Render::RHI::Texture* pTexture = Render::CreateTextureFromBase64( m_pRenderSystem, pData, size );
        EE_ASSERT( pTexture );

        //-------------------------------------------------------------------------

        ImageInfo info;
        info.m_ID = ImTextureID_Pack
        (
            0, // TODO: Provide correct sampler
            Render::RHI::GetTextureHandle( pTexture, Render::RHI::DescriptorTypeFlags::Texture, 0 )
        );
        info.m_size = Float2( float( pTexture->m_width ), float( pTexture->m_height ) );
        info.m_pTexture = pTexture;

        m_loadedImages.emplace_back( info );

        //-------------------------------------------------------------------------

        return info;
    }

    void ImageCache::UnloadImage( ImageInfo& info )
    {
        for ( auto i = 0; i < m_loadedImages.size(); i++ )
        {
            if ( m_loadedImages[i].m_ID == info.m_ID )
            {
                Render::RHI::DestroyTexture( m_pRenderSystem->GetContextRHI(), eastl::move( m_loadedImages[i].m_pTexture ) );

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
}
#endif
