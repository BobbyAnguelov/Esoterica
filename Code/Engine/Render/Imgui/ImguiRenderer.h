#pragma once

#include "Base/Render/RHI.h"
#include "Base/Render/RenderWindow.h"
#include "Base/Systems.h"
#include "Engine/_Module/API.h"
#include "Engine/Render/Imgui/ImguiImageCache.h"
#include "Engine/Render/RenderViewport.h"
#include "Engine/Render/Device/DeviceResizeBuffer.h"

//-------------------------------------------------------------------------

struct ImDrawData;

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Render
{
    class RenderSystem;
    class Window;

    //-------------------------------------------------------------------------

    struct ImGui_BackendUserData
    {
        RenderSystem*                                       m_pRenderSystem = nullptr;
        Window*                                             m_pRenderWindow = nullptr;
    };

    struct ImGui_RendererUserData
    {
        Window                                              m_renderWindow;
        TArray<RHI::Buffer*, RHI::MaxPendingFrames>         m_constantBuffers = {};
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API ImguiRenderer : public ISystem
    {
    public:

        EE_SYSTEM( ImguiRenderer );

    public:

        void Initialize( Window* pPrimaryRenderWindow, RenderSystem* pRenderSystem );
        void Shutdown();

        #if EE_DEVELOPMENT_TOOLS
        void PreShaderHotReload();
        void PostShaderHotReload();
        #endif

        void UpdateDeviceResources();

        void StartFrame();
        void SubmitFrame( bool clear );
        void EndFrame();

        inline ImGuiX::ImageCache* GetImageCache() { return &m_imageCache; }

    private:

        struct ImguiGeometryState
        {
            uint32_t vertexOffset = 0;
            uint32_t indexOffset = 0;
        };

        static void RenderImguiData( ImDrawData const*      pDrawData,
                                     RenderSystem*          pRenderSystem,
                                     RHI::CommandBuffer*    pCommandBuffer,
                                     RHI::Texture*          pRenderTarget,
                                     RHI::Pipeline*         pPipeline,
                                     RHI::Buffer*           pConstantBuffer,
                                     RHI::Buffer*           pVertexBuffer,
                                     RHI::Buffer*           pIndexBuffer,
                                     uint32_t               frameIndex,
                                     bool                   clear,
                                     ImguiGeometryState&    state );

        static void UpdateTextureData( ImDrawData const* pDrawData, RenderSystem* pRenderSystem );

    private:

        RenderSystem*                                       m_pRenderSystem = nullptr;
        Window*                                             m_pPrimaryRenderWindow = nullptr;

        ImGuiX::ImageCache                                  m_imageCache;

        RHI::Pipeline*                                      m_pPipeline = nullptr;

        TArray<DeviceResizeBuffer, RHI::MaxPendingFrames>   m_vertexBuffers = {};
        TArray<DeviceResizeBuffer, RHI::MaxPendingFrames>   m_indexBuffers = {};
        TArray<RHI::Buffer*, RHI::MaxPendingFrames>         m_constantBuffers = {};
    };
}
#endif