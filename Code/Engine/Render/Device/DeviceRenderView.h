
#pragma once

#include "Engine/Render/Shaders/EngineShader.h"
#include "Engine/Render/Device/DeviceResourceState.h"
#include "Engine/Render/Device/DeviceResizeBuffer.h"
#include "Base/Render/PageAllocator.h"
#include "Base/Math/Vector.h"

namespace EE::Render
{
    class DeviceRenderWorld;
    class RenderSystem;

    //-------------------------------------------------------------------------

    enum class DeviceRenderViewType
    {
        Main,
        GlobalEnvironmentMap,
        LocalEnvironmentMap,
        CascadedShadowMap,
        PointShadowMap,
        SpotShadowMap,

        #if EE_DEVELOPMENT_TOOLS
        EditorOutline,
        #endif

        NumTypes,
        Invalid = NumTypes,
    };

    static constexpr uint32_t g_NumGlobalEnvironmentMapViews = 6;   // One per cubemap face
    static constexpr uint32_t g_NumCascadedShadowViews = 4;         // One per cascade
    static constexpr uint32_t g_NumPointShadowViews = 1;            // One per light, octahedral faces are not views
    static constexpr uint32_t g_NumSpotShadowViews = 1;             // A single shadow map

    static constexpr uint32_t g_NumPointShadowFaces = 8;

    inline uint32_t GetRenderViewGroupSize( DeviceRenderViewType type )
    {
        if ( type == DeviceRenderViewType::GlobalEnvironmentMap )
        {
            return g_NumGlobalEnvironmentMapViews;
        }

        if ( type == DeviceRenderViewType::CascadedShadowMap )
        {
            return g_NumCascadedShadowViews;
        }

        if ( type == DeviceRenderViewType::PointShadowMap )
        {
            return g_NumPointShadowViews;
        }

        return 1;
    }

    //-------------------------------------------------------------------------

    struct DeviceRenderView final
    {
        DeviceRenderViewType                m_deviceRenderViewType = DeviceRenderViewType::Invalid;

        mutable DeviceTextureState          m_colorTexture = {};                            // Plain texture, cubemap or texture array. Only valid for color passes
        mutable DeviceTextureState          m_depthTexture = {};                            // Depth target, or the cascade array for shadow views

        RHI::Texture*                       m_pEnvironmentMapRadianceTexture = nullptr;    // Environment map views only
        RHI::Buffer*                        m_pSHCoefficientsBuffer = nullptr;             // Environment map views only

        //-------------------------------------------------------------------------

        inline bool IsValid() const { return m_deviceRenderViewType != DeviceRenderViewType::Invalid; }

        void Initialize( RenderSystem* pRenderSystem );
        void Shutdown( RenderSystem* pRenderSystem );
    };
}
