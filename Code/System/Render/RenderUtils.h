#pragma once
#include "System/_Module/API.h"
#include "System/Esoterica.h"

//-------------------------------------------------------------------------

namespace EE::FileSystem { class Path; }

//-------------------------------------------------------------------------

namespace EE::Render
{
    class Texture;
    class RenderDevice;

    //-------------------------------------------------------------------------

    namespace Utils
    {
        EE_SYSTEM_API bool CreateTextureFromFile( RenderDevice* pRenderDevice, FileSystem::Path const& path, Texture& texture );
        EE_SYSTEM_API bool CreateTextureFromBase64( RenderDevice* pRenderDevice, uint8_t const* pData, size_t size, Texture& texture );
    }
}