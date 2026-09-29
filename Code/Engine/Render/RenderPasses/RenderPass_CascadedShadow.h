#pragma once

#include "Base/Render/RHI.h"
#include "Base/Types/Arrays.h"
#include "Engine/Render/ActiveRenderView.h"
#include "Engine/Render/Device/DeviceRenderView.h"
#include "Engine/Render/RenderPasses/RenderPass.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class DeviceRenderWorld;
    class RenderSettings;
    class RenderViewport;

    struct ForwardShadingMaterialShaderPipelineBucket; // TODO: Decouple forward shading

    //-------------------------------------------------------------------------

    struct CascadedShadowPass
    {
    public:

        void Initialize( RenderPassContext const& context );
        void Shutdown( RenderSystem* pRenderSystem );

        void BarrierWriteable( DeviceRenderWorld const& deviceRenderWorld, ActiveRenderViewList const& activeRenderViewList, DeviceResourceStates& resourceStates ) const;
        void BarrierReadOnly( DeviceRenderWorld const& deviceRenderWorld, ActiveRenderViewList const& activeRenderViewList, DeviceResourceStates& resourceStates ) const;

        void DrawShadowCascades
        (
            TArrayView<ForwardShadingMaterialShaderPipelineBucket const>    materialShaderPipelineBuckets,
            ActiveRenderViewList const&                                     activeRenderViewList,
            DeviceRenderWorld const&                                        deviceRenderWorld,
            DeviceResourceStates&                                           resourceStates,
            RHI::CommandBuffer*                                             pCommandBuffer
        ) const;

        //-------------------------------------------------------------------------

        RenderSettings const*                               m_pRenderSettings = nullptr;
    };
}
