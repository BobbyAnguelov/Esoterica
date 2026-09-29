#pragma once

#if EE_DEVELOPMENT_TOOLS

#include "Base/Render/RHI.h"
#include "Engine/Render/ActiveRenderView.h"
#include "Engine/Render/Device/DeviceRenderView.h"
#include "Engine/Render/RenderPasses/RenderPass.h"

namespace EE
{
    class EntityWorld;
}

namespace EE::Render
{
    namespace ShaderTypes
    {
        struct RenderView;
    }

    class DeviceRenderWorld;
    class RenderSystem;
    class RenderViewport;

    //-------------------------------------------------------------------------

    struct EditorOutlineRenderPass
    {
    public:

        void Initialize( RenderPassContext const& context );
        void Shutdown( RenderSystem* pRenderSystem );

        void UpdateWorldDeviceResources( EntityWorld* pWorld );
        void UpdateViewportDeviceResources( RenderSystem* pRenderSystem, RenderViewport* pRenderViewport );

        void DrawToViewport
        (
            TArrayView<ForwardShadingMaterialShaderPipelineBucket const>    materialShaderBuckets,
            ActiveRenderView const&                                         activeRenderView,
            RenderViewport const*                                           pRenderViewport,
            DeviceResourceStates&                                           resourceStates,
            RHI::CommandBuffer*                                             pCommandBuffer
        ) const;

        void ResolveToViewport( RenderViewport const* pRenderViewport, DeviceResourceStates& resourceStates, RHI::CommandBuffer* pCommandBuffer ) const;

        //-------------------------------------------------------------------------

        RHI::Pipeline*                              m_pInitializePipeline = nullptr;
        RHI::Pipeline*                              m_pJumpFloodPipeline = nullptr;
        RHI::Pipeline*                              m_pCompositePipeline = nullptr;

        RenderSettings const*                       m_pRenderSettings = nullptr;
    };
}

#endif
