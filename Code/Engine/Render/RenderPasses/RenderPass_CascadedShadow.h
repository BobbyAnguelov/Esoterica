#pragma once

#include "Base/Render/RHI.h"
#include "Base/Types/Arrays.h"
#include "Engine/Render/Device/DeviceRenderView.h"
#include "Engine/Render/RenderPasses/RenderPass.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    namespace ShaderTypes
    {
        struct RenderView;
        struct CascadedShadow;
    }

    class DeviceRenderWorld;
    class RenderSettings;
    class RenderViewport;

    struct ForwardShadingMaterialShaderPipelineBucket; // TODO: Decouple forward shading

    //-------------------------------------------------------------------------

    struct CascadedShadowPass
    {
        static constexpr uint32_t                           NumShadowCascades = 4;

    public:

        void Initialize( RenderPassContext const& context );
        void Shutdown( RenderSystem* pRenderSystem );

        void UpdateDeviceResources( RenderSystem* pRenderSystem, TArrayView<uint32_t const> clusterCapacity );

        void DrawShadowCascades( TArrayView<ForwardShadingMaterialShaderPipelineBucket const>   materialShaderPipelineBuckets,
                                 TArrayView<uint32_t const>                                     clusterCapacity,
                                 RenderViewport const*                                          pRenderViewport,
                                 Vector                                                         lightDirection,
                                 DeviceResourceStates&                                          resourceStates,
                                 RHI::CommandBuffer*                                            pCommandBuffer,
                                 ShaderTypes::CascadedShadow*                                   pOutCascadedShadow_WriteCombined,
                                 TArrayView<ShaderTypes::RenderView>                            dstRenderViews_WriteCombined );

    public:

        TArray<DeviceRenderView, NumShadowCascades>         m_renderViews;
        DeviceTextureState                                  m_depthTargetArray = {};
        RenderSettings const*                               m_pRenderSettings = nullptr;
    };
}
