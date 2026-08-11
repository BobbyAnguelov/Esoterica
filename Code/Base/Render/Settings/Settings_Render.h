#pragma once

#include "Base/Settings/Settings.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class EE_BASE_API RenderSettings : public Settings
    {
        EE_REFLECT_TYPE( RenderSettings );

    public:

        virtual char const* GetSectionName() const override { return "Render"; }

    public:

        EE_REFLECT( Category = "Misc" );
        bool                    m_isFullscreen = false;

        EE_REFLECT( Category = "Misc" );
        uint32_t                m_stagingBufferSize = 32 << 20;

        EE_REFLECT( Category = "Light Culling" );
        float                   m_lightCullingMinCellSize = 4.0f;

        EE_REFLECT( Category = "Light Culling" );
        uint32_t                m_lightCullingInitialDispatchX = 8;

        EE_REFLECT( Category = "Light Culling" );
        uint32_t                m_lightCullingInitialDispatchY = 8;

        EE_REFLECT( Category = "Light Culling" );
        uint32_t                m_lightCullingInitialDispatchZ = 2;

        EE_REFLECT( Category = "Light Culling" );
        uint32_t                m_spatialHashTableSize = 100000; // rounded to next power of 2

        EE_REFLECT( Category = "Light Culling" );
        uint32_t                m_spatialHashBorderLOD1 = 3;

        EE_REFLECT( Category = "Light Culling" );
        uint32_t                m_spatialHashBorderLOD2 = 3;

        EE_REFLECT( Category = "Light Culling" );
        uint32_t                m_spatialHashBorderLOD3 = 2;

        EE_REFLECT( Category = "Light Culling" );
        uint32_t                m_spatialHashBorderLOD4 = 1;

        EE_REFLECT( Category = "Light Culling" );
        uint32_t                m_spatialHashBorderLOD5 = 2;

        EE_REFLECT( Category = "Light Culling" );
        uint32_t                m_spatialHashNumLODs = 5;

        EE_REFLECT( Category = "Render" );
        bool                    m_enableSMAA = true; // TODO: This should be world setting

        EE_REFLECT( Category = "Render" );
        bool                    m_enableSSAO = true;

        EE_REFLECT( Category = "Render" );
        bool                    m_enableSSAOLowResolution = false;

        EE_REFLECT( Category = "Render" );
        uint32_t                m_cascadedShadowResolution = 4096; // TODO: This should be world setting

        EE_REFLECT( Category = "Render" );
        bool                    m_enableAsyncCompute = true;

        EE_REFLECT( Category = "RHI" );
        bool                    m_enableHostValidation = false;

        EE_REFLECT( Category = "RHI" );
        bool                    m_enableDeviceValidation = false;

        EE_REFLECT( Category = "RHI" );
        bool                    m_enableDeviceBreadcrumbs = false;

        EE_REFLECT( Category = "RHI" );
        bool                    m_enableRenderDoc = false;

        EE_REFLECT( Category = "RHI" );
        bool                    m_enablePix = false;

        EE_REFLECT( Category = "RHI" );
        bool                    m_enableAmdAgs = false;

        EE_REFLECT( Category = "RHI" );
        int32_t                 m_deviceIndex = -1;
    };
}
