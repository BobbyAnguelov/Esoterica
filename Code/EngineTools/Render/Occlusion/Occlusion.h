#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceCompilerContext.h"
#include "Base/Esoterica.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    namespace Occlusion
    {
        struct HeightField
        {
            uint8_t const*  m_pData = nullptr;
            int32_t         m_width = 0;
            int32_t         m_height = 0;
            int32_t         m_strideInBytes = 0;
            int32_t         m_pixelSizeInBytes = 0;
            int32_t         m_channel = 0;
        };

        struct Settings
        {
            float       m_strength = 1.0F;
            int32_t     m_blurRadius = 1;
            float       m_contrast = 1.0F;
        };

        EE_ENGINETOOLS_API void Generate( Resource::CompileContext const& ctx, Settings const& settings, uint8_t* pSurfaceData, int32_t width, int32_t height, int32_t strideInBytes, int32_t targetChannel, HeightField const& field );
    }
}
