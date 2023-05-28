#include "RenderUtils.h"
#include "RenderDevice.h"
#include "System/Types/Arrays.h"
#include "System/Encoding/Encoding.h"
#include "System/ThirdParty/stb/stb_image.h"

//-------------------------------------------------------------------------

namespace EE::Render::Utils
{
    bool CreateTextureFromFile( RenderDevice* pRenderDevice, FileSystem::Path const& path, Texture& texture )
    {
        EE_ASSERT( pRenderDevice != nullptr );
        EE_ASSERT( path.Exists() );
        EE_ASSERT( !texture.IsValid() );

        //-------------------------------------------------------------------------

        int32_t width, height, channels;
        uint8_t* pImage = stbi_load( path.c_str(), &width, &height, &channels, 4 );
        if ( pImage == nullptr )
        {
            return false;
        }

        texture = Texture( Int2( width, height ) );
        size_t const imageSize = size_t( width ) * height * channels; // 8 bits per channel
        pRenderDevice->CreateDataTexture( texture, TextureFormat::Raw, pImage, imageSize );
        stbi_image_free( pImage );

        return true;
    }

    bool CreateTextureFromBase64( RenderDevice* pRenderDevice, uint8_t const* pData, size_t size, Texture& texture )
    {
        EE_ASSERT( pRenderDevice != nullptr );
        EE_ASSERT( pData != nullptr && size > 0 );
        EE_ASSERT( !texture.IsValid() );

        //-------------------------------------------------------------------------

        Blob const decodedData = Encoding::Base64::Decode( pData, size );

        //-------------------------------------------------------------------------

        int32_t width, height, channels;
        uint8_t* pImage = stbi_load_from_memory( decodedData.data(), (int32_t) decodedData.size(), &width, &height, &channels, 4 );
        if ( pImage == nullptr )
        {
            return false;
        }

        texture = Texture( Int2( width, height ) );
        size_t const imageSize = size_t( width ) * height * channels; // 8 bits per channel
        pRenderDevice->CreateDataTexture( texture, TextureFormat::Raw, pImage, imageSize );
        stbi_image_free( pImage );

        return true;
    }
}