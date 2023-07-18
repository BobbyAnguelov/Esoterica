#pragma once

#include "DebugRenderStates.h"
#include "Engine/Render/IRenderer.h"
#include "Base/Render/RenderDevice.h"
#include "Base/Drawing/DebugDrawing.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Render
{
    class EE_ENGINE_API DebugRenderer final : public IRenderer
    {
    public:

        EE_RENDERER_ID( DebugRenderer, RendererPriorityLevel::Debug );

    public:

        bool IsInitialized() const { return m_initialized; }
        bool Initialize( RenderDevice* pRenderDevice );
        void Shutdown();
        void RenderWorld( Seconds const deltaTime, Viewport const& viewport, RenderTarget const& renderTarget, EntityWorld* pWorld ) override final;

    private:

        void DrawPoints( RenderContext const& renderContext, Viewport const& viewport, TVector<Drawing::PointCommand> const& commands );
        void DrawLines( RenderContext const& renderContext, Viewport const& viewport, TVector<Drawing::LineCommand> const& commands );
        void DrawTriangles( RenderContext const& renderContext, Viewport const& viewport, TVector<Drawing::TriangleCommand> const& commands );
        void DrawText( RenderContext const& renderContext, Viewport const& viewport, TVector<Drawing::TextCommand> const& commands, IntRange cmdRange );

    private:

        RenderDevice*                               m_pRenderDevice = nullptr;

        DebugLineRenderState                        m_lineRS;
        DebugPointRenderState                       m_pointRS;
        DebugPrimitiveRenderState                   m_primitiveRS;
        DebugTextRenderState                        m_textRS;

        Drawing::FrameCommandBuffer                 m_drawCommands;
        bool                                        m_initialized = false;

        // Text rendering
        TVector<DebugFontGlyphVertex>               m_intermediateGlyphVertexData;
        TVector<uint16_t>                             m_intermediateGlyphIndexData;
    };
}
#endif