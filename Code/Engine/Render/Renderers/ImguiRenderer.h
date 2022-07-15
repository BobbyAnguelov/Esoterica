#pragma once

#include "Engine/_Module/API.h"
#include "imgui.h"
#include "Engine/Render/IRenderer.h"
#include "System/Render/RenderDevice.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Render
{
    class Viewport;

    //-------------------------------------------------------------------------\

    class EE_ENGINE_API ImguiRenderer final : public IRenderer
    {
    public:

        EE_RENDERER_ID( ImguiRenderer, RendererPriorityLevel::DevelopmentTools );

    private:

        struct RecordedCmdBuffer
        {
            TVector<ImDrawCmd>                  m_cmdBuffer;
            uint32_t                              m_numVertices;
        };

    public:

        bool IsInitialized() const { return m_initialized; }
        bool Initialize( RenderDevice* pRenderDevice );
        void Shutdown();
        void RenderViewport( Seconds const deltaTime, Viewport const& viewport, RenderTarget const& renderTarget ) override final;

    private:

        void RenderImguiData( RenderContext const& renderContext, ImDrawData const* pDrawData );

    private:

        RenderDevice*                   m_pRenderDevice = nullptr;

        VertexShader                    m_vertexShader;
        PixelShader                     m_pixelShader;
        ShaderInputBindingHandle        m_inputBinding;
        BlendState                      m_blendState;
        SamplerState                    m_samplerState;
        RasterizerState                 m_rasterizerState;
        TVector<ScissorRect>            m_scissorRects;
        RenderBuffer                    m_indexBuffer;
        VertexBuffer                    m_vertexBuffer;
        Texture                         m_fontTexture;

        PipelineState                   m_PSO;
        bool                            m_initialized = false;
    };
}
#endif