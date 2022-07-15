#include "RenderDefaultResources.h"
#include "RenderDevice.h"
#include "System/Algorithm/Encoding.h"
#include "System/ThirdParty/stb/stb_image.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    namespace DefaultResources
    {
        // 320x240 Default Texture
        static unsigned char const g_defaultTexturePngBase64Encoded[] = "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";

        //-------------------------------------------------------------------------

        struct DefaultResourceContainer
        {
            Texture m_defaultTexture;
        };

        static DefaultResourceContainer* g_pDefaultResources = nullptr;

        //-------------------------------------------------------------------------

        void Initialize( RenderDevice* pRenderDevice )
        {
            EE_ASSERT( pRenderDevice != nullptr );

            g_pDefaultResources = EE::New<DefaultResourceContainer>();

            // Create default texture
            //-------------------------------------------------------------------------

            Blob decodedData = Encoding::Base64::Decode( g_defaultTexturePngBase64Encoded, sizeof( g_defaultTexturePngBase64Encoded ) );

            int32_t width, height, channels;
            uint8_t* pImage = stbi_load_from_memory( decodedData.data(), (int32_t) decodedData.size(), &width, &height, &channels, 4 );
            size_t const imageSize = size_t( width ) * height * channels; // 8 bits per channel

            g_pDefaultResources->m_defaultTexture = Texture( Int2( 320, 240 ) );
            pRenderDevice->CreateDataTexture( g_pDefaultResources->m_defaultTexture, TextureFormat::Raw, pImage, imageSize );

            stbi_image_free( pImage );
        }

        void Shutdown( RenderDevice* pRenderDevice )
        {
            EE_ASSERT( pRenderDevice != nullptr );

            pRenderDevice->DestroyTexture( g_pDefaultResources->m_defaultTexture );

            //-------------------------------------------------------------------------

            EE::Delete( g_pDefaultResources );
        }

        Texture const* GetDefaultTexture()
        {
            EE_ASSERT( g_pDefaultResources != nullptr );
            return &g_pDefaultResources->m_defaultTexture;
        }
    }
}