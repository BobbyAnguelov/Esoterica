#pragma once

#include "Base/Esoterica.h"
#include "Engine/_Module/API.h"
#include "imgui.h"

#if EE_DEVELOPMENT_TOOLS
#include "Base/Types/Arrays.h"
#include "Base/Render/RHI.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class RenderSystem;
}

namespace EE::ImGuiX
{
    struct ImageInfo
    {
        inline bool IsValid() const { return m_ID != 0; }

    public:

        ImTextureID             m_ID = 0;
        Render::RHI::Texture*   m_pTexture = nullptr;
        ImVec2                  m_size = ImVec2( 0, 0 );
    };

    //-------------------------------------------------------------------------

    // Interface to allow various systems the ability to load/unload images without having access to the imgui system directly
    class EE_ENGINE_API ImageCache
    {
    public:

        ImageCache() = default;
        ImageCache( ImageCache const& ) = default;

        ~ImageCache();

        ImageCache& operator=( ImageCache const& ) = default;

        inline bool WasInitialized() const { return m_pRenderSystem != nullptr; }

        void Initialize( Render::RenderSystem* pRenderSystem );
        void Shutdown();

        //-------------------------------------------------------------------------

        // Load an image from disk and get back the Imgui ID for it
        ImageInfo LoadImageFromFile( FileSystem::Path const& path );

        // Load an image from compressed memory and return the Imgui ID for it
        ImageInfo LoadImageFromMemoryBase64( uint8_t const* pData, size_t size );

        // Unload a previously loaded image
        void UnloadImage( ImageInfo& info );

    private:

        Render::RenderSystem*   m_pRenderSystem = nullptr;
        TVector<ImageInfo>      m_loadedImages;
    };
}
#endif