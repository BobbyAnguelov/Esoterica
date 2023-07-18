#pragma once

#include "Base/_Module/API.h"
#include "Base/Esoterica.h"

#if EE_DEVELOPMENT_TOOLS
#include "Base/Imgui/ImguiX.h"
#include "Base/Types/Arrays.h"

//-------------------------------------------------------------------------

namespace EE
{
    namespace Render{ class RenderDevice; }
}

//-------------------------------------------------------------------------

namespace EE::ImGuiX
{
    // Interface to allow various systems the ability to load/unload images without having access to the imgui system directly
    class EE_BASE_API ImageCache
    {
    public:

        ~ImageCache();

        inline bool IsInitialized() const { return m_pRenderDevice != nullptr; }
        void Initialize( Render::RenderDevice* pRenderDevice );
        void Shutdown();

        //-------------------------------------------------------------------------

        // Load an image from disk and get back the Imgui ID for it
        ImageInfo LoadImageFromFile( FileSystem::Path const& path );

        // Load an image from compressed memory and return the Imgui ID for it
        ImageInfo LoadImageFromMemoryBase64( uint8_t const* pData, size_t size );

        // Unload a previously loaded image
        void UnloadImage( ImageInfo& info );

    private:

        Render::RenderDevice*           m_pRenderDevice = nullptr;
        TVector<ImageInfo>              m_loadedImages;
    };
}

#endif