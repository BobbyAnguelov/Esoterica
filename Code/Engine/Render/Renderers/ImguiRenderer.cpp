#include "ImguiRenderer.h"
#include "Engine/Render/Shaders/ImguiShaders.h"
#include "System/Render/RenderViewport.h"
#include "System/Profiling.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Render
{
    namespace
    {
        static uint32_t const g_numInitialVertices = 2000;
        static uint32_t const g_numInitialIndices = 2000;
    }

    //-------------------------------------------------------------------------

    static void ImGui_CreateWindowContext( ImGuiViewport* pViewport )
    {
        ImGuiIO& io = ImGui::GetIO();
        RenderDevice* pRenderDevice = (RenderDevice*) io.BackendRendererUserData;
        EE_ASSERT( pRenderDevice != nullptr );

        //-------------------------------------------------------------------------

        HWND hwnd = pViewport->PlatformHandleRaw ? (HWND) pViewport->PlatformHandleRaw : (HWND) pViewport->PlatformHandle;
        EE_ASSERT( hwnd != 0 );

        auto pSecondaryWindow = EE::New<RenderWindow>();
        pRenderDevice->CreateSecondaryRenderWindow( *pSecondaryWindow, hwnd );
        pViewport->RendererUserData = pSecondaryWindow;
    }

    static void ImGui_DestroyWindowContext( ImGuiViewport* pViewport )
    {
        ImGuiIO& io = ImGui::GetIO();
        RenderDevice* pRenderDevice = (RenderDevice*) io.BackendRendererUserData;
        EE_ASSERT( pRenderDevice != nullptr );

        //-------------------------------------------------------------------------

        // The main viewport (owned by the application) will always have RendererUserData == NULL since we didn't create the data for it.
        if ( auto pSecondaryWindow = (RenderWindow*) pViewport->RendererUserData )
        {
            pRenderDevice->DestroySecondaryRenderWindow( *pSecondaryWindow );
            EE::Delete( pSecondaryWindow );
        }

        pViewport->RendererUserData = nullptr;
    }

    static void ImGui_SetWindowSize( ImGuiViewport* pViewport, ImVec2 size )
    {
        ImGuiIO& io = ImGui::GetIO();
        RenderDevice* pRenderDevice = (RenderDevice*) io.BackendRendererUserData;
        EE_ASSERT( pRenderDevice != nullptr );

        //-------------------------------------------------------------------------

        auto pSecondaryWindow = (RenderWindow*) pViewport->RendererUserData;
        pRenderDevice->ResizeWindow( *pSecondaryWindow, Int2( (int32_t) size.x, (int32_t) size.y ) );
    }

    //-------------------------------------------------------------------------

    bool ImguiRenderer::Initialize( RenderDevice* pRenderDevice )
    {
        EE_ASSERT( m_pRenderDevice == nullptr && pRenderDevice != nullptr );
        m_pRenderDevice = pRenderDevice;

        //-------------------------------------------------------------------------

        // Create vertex buffer for IMGUI
        m_vertexBuffer.m_byteStride = sizeof( ImDrawVert );
        m_vertexBuffer.m_byteSize = m_vertexBuffer.m_byteStride * g_numInitialVertices;
        m_vertexBuffer.m_type = RenderBuffer::Type::Vertex;
        m_vertexBuffer.m_usage = RenderBuffer::Usage::CPU_and_GPU;
        m_pRenderDevice->CreateBuffer( m_vertexBuffer, nullptr );

        if ( !m_vertexBuffer.IsValid() )
        {
            return false;
        }

        // Create index buffer for IMGUI
        m_indexBuffer.m_byteStride = sizeof( ImDrawIdx );
        m_indexBuffer.m_byteSize = m_indexBuffer.m_byteStride * g_numInitialIndices;
        m_indexBuffer.m_type = RenderBuffer::Type::Index;
        m_indexBuffer.m_usage = RenderBuffer::Usage::CPU_and_GPU;
        m_pRenderDevice->CreateBuffer( m_indexBuffer, nullptr );

        if ( !m_indexBuffer.IsValid() )
        {
            return false;
        }

        // Create blend state
        m_blendState.m_blendEnable = true;
        m_blendState.m_srcValue = BlendValue::SourceAlpha;
        m_blendState.m_dstValue = BlendValue::InverseSourceAlpha;
        m_blendState.m_blendOp = BlendOp::Add;
        m_blendState.m_srcAlphaValue = BlendValue::One;
        m_blendState.m_dstAlphaValue = BlendValue::InverseSourceAlpha;
        m_blendState.m_blendOpAlpha = BlendOp::Add;
        m_pRenderDevice->CreateBlendState( m_blendState );

        if ( !m_blendState.IsValid() )
        {
            return false;
        }

        // Set Rasterizer state for drawing
        m_rasterizerState.m_cullMode = CullMode::None;
        m_rasterizerState.m_windingMode = WindingMode::CounterClockwise;
        m_rasterizerState.m_fillMode = FillMode::Solid;
        m_rasterizerState.m_scissorCulling = true;
        m_pRenderDevice->CreateRasterizerState( m_rasterizerState );

        // Create sampler state
        m_pRenderDevice->CreateSamplerState( m_samplerState );

        // Create Shaders
        TVector<RenderBuffer> cbuffers;

        // World transform const buffer
        RenderBuffer buffer;
        buffer.m_byteSize = sizeof( Matrix );
        buffer.m_byteStride = 16; // Vector4 aligned
        buffer.m_usage = RenderBuffer::Usage::CPU_and_GPU;
        buffer.m_type = RenderBuffer::Type::Constant;
        cbuffers.push_back( buffer );

        // Vertex Shader
        VertexLayoutDescriptor vertexLayoutDesc;
        vertexLayoutDesc.m_elementDescriptors.push_back( VertexLayoutDescriptor::ElementDescriptor( DataSemantic::Position, DataFormat::Float_R32G32, 0, offsetof( ImDrawVert, pos ) ) );
        vertexLayoutDesc.m_elementDescriptors.push_back( VertexLayoutDescriptor::ElementDescriptor( DataSemantic::TexCoord, DataFormat::Float_R32G32, 0, offsetof( ImDrawVert, uv ) ) );
        vertexLayoutDesc.m_elementDescriptors.push_back( VertexLayoutDescriptor::ElementDescriptor( DataSemantic::Color, DataFormat::UNorm_R8G8B8A8, 0, offsetof( ImDrawVert, col ) ) );
        vertexLayoutDesc.CalculateByteSize();

        m_vertexShader = VertexShader( g_byteCode_VS_imgui, sizeof( g_byteCode_VS_imgui ), cbuffers, vertexLayoutDesc );
        m_pRenderDevice->CreateShader( m_vertexShader );

        // Create input binding
        m_pRenderDevice->CreateShaderInputBinding( m_vertexShader, vertexLayoutDesc, m_inputBinding );
        EE_ASSERT( m_inputBinding.IsValid() );

        // Pixel Shader
        cbuffers.clear();
        m_pixelShader = PixelShader( g_byteCode_PS_imgui, sizeof( g_byteCode_PS_imgui ), cbuffers );
        m_pRenderDevice->CreateShader( m_pixelShader );

        // Set up PSO
        m_PSO.m_pVertexShader = &m_vertexShader;
        m_PSO.m_pPixelShader = &m_pixelShader;
        m_PSO.m_pBlendState = &m_blendState;
        m_PSO.m_pRasterizerState = &m_rasterizerState;

        //-------------------------------------------------------------------------
        // IMGUI Setup
        //-------------------------------------------------------------------------

        // Imgui needs to be initialized before this
        EE_ASSERT( ::ImGui::GetCurrentContext() != nullptr );

        ImGuiIO& io = ::ImGui::GetIO();
        io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;

        // Multiple Viewport Support
        if ( io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable )
        {
            ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
            platformIO.Renderer_CreateWindow = ImGui_CreateWindowContext;
            platformIO.Renderer_DestroyWindow = ImGui_DestroyWindowContext;
            platformIO.Renderer_SetWindowSize = ImGui_SetWindowSize;
        }

        //-------------------------------------------------------------------------

        uint8_t* pPixels = nullptr;
        Int2 dimensions;
        io.Fonts->GetTexDataAsRGBA32( &pPixels, &dimensions.m_x, &dimensions.m_y );
        size_t const textureDataSize = dimensions.m_x * dimensions.m_y * 4;

        m_fontTexture = Texture( dimensions );
        EE_ASSERT( pPixels != nullptr );
        m_pRenderDevice->CreateDataTexture( m_fontTexture, TextureFormat::Raw, pPixels, textureDataSize );

        io.Fonts->TexID = const_cast<ViewSRVHandle*>( &m_fontTexture.GetShaderResourceView() );

        //-------------------------------------------------------------------------

        m_initialized = m_fontTexture.IsValid();
        return m_initialized;
    }

    void ImguiRenderer::Shutdown()
    {
        ImGuiIO& io = ::ImGui::GetIO();
        io.Fonts->TexID = nullptr;

        //-------------------------------------------------------------------------

        ImGui::DestroyPlatformWindows();

        //-------------------------------------------------------------------------

        m_PSO.Clear();

        if( m_inputBinding.IsValid() )
        {
            m_pRenderDevice->DestroyShaderInputBinding( m_inputBinding );
        }

        if( m_vertexShader.IsValid() )
        {
            m_pRenderDevice->DestroyShader( m_vertexShader );
        }

        if( m_pixelShader.IsValid() )
        {
            m_pRenderDevice->DestroyShader( m_pixelShader );
        }

        if ( m_samplerState.IsValid() )
        {
            m_pRenderDevice->DestroySamplerState( m_samplerState );
        }

        if ( m_vertexBuffer.IsValid() )
        {
            m_pRenderDevice->DestroyBuffer( m_vertexBuffer );
        }

        if ( m_indexBuffer.IsValid() )
        {
            m_pRenderDevice->DestroyBuffer( m_indexBuffer );
        }

        if ( m_fontTexture.IsValid() )
        {
            m_pRenderDevice->DestroyTexture( m_fontTexture );
        }

        if ( m_rasterizerState.IsValid() )
        {
            m_pRenderDevice->DestroyRasterizerState( m_rasterizerState );
        }

        if ( m_blendState.IsValid() )
        {
            m_pRenderDevice->DestroyBlendState( m_blendState );
        }

        m_pRenderDevice = nullptr;
        m_initialized = false;
    }

    void ImguiRenderer::RenderViewport( Seconds const deltaTime, Viewport const& viewport, RenderTarget const& renderTarget )
    {
        EE_ASSERT( IsInitialized() && Threading::IsMainThread() );
        EE_PROFILE_FUNCTION_RENDER();

        ImGuiIO& io = ImGui::GetIO();

        // Render main imgui viewport
        //-------------------------------------------------------------------------

        ImGui::Render();

        auto const& renderContext = m_pRenderDevice->GetImmediateContext();
        renderContext.SetRenderTarget( renderTarget );

        ImDrawData const* pData = ImGui::GetDrawData();
        if ( pData != nullptr )
        {
            RenderImguiData( renderContext, pData );
        }

        // Viewport Support
        //-------------------------------------------------------------------------

        if ( io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable )
        {
            ImGui::UpdatePlatformWindows();

            //-------------------------------------------------------------------------

            ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();

            for ( int i = 1; i < platformIO.Viewports.Size; i++ )
            {
                ImGuiViewport* pViewport = platformIO.Viewports[i];
                if ( pViewport->Flags & ImGuiViewportFlags_IsMinimized )
                {
                    continue;
                }

                auto pSecondarySwapChain = (RenderWindow*) pViewport->RendererUserData;
                EE_ASSERT( pSecondarySwapChain != nullptr );

                renderContext.SetRenderTarget( *pSecondarySwapChain->GetRenderTarget() );
                if ( !( pViewport->Flags & ImGuiViewportFlags_NoRendererClear ) )
                {
                    renderContext.ClearRenderTargetViews( *pSecondarySwapChain->GetRenderTarget() );
                }
                RenderImguiData( renderContext, pViewport->DrawData );
                renderContext.Present( *pSecondarySwapChain );
            }
        }
    }

    void ImguiRenderer::RenderImguiData( RenderContext const& renderContext, ImDrawData const* pDrawData )
    {
        if ( pDrawData->DisplaySize.x <= 0.0f || pDrawData->DisplaySize.y <= 0.0f )
        {
            return;
        }

        // Buffer management
        //-------------------------------------------------------------------------

        // Check if our vertex and index buffers are large enough, if not then grow them
        if ( (int32_t) m_vertexBuffer.GetNumElements() < pDrawData->TotalVtxCount )
        {
            m_pRenderDevice->ResizeBuffer( m_vertexBuffer, sizeof( ImDrawVert ) * pDrawData->TotalVtxCount );
        }

        if ( (int32_t) m_indexBuffer.GetNumElements() < pDrawData->TotalIdxCount )
        {
            m_pRenderDevice->ResizeBuffer( m_indexBuffer, sizeof( ImDrawIdx ) * pDrawData->TotalIdxCount );
        }

        // Transfer buffer data
        //-------------------------------------------------------------------------

        // Copy vertices into our vertex and index buffers and record the command lists
        ImDrawVert* pVB = (ImDrawVert*) renderContext.MapBuffer( m_vertexBuffer );
        int32_t VBWriteIdx = 0;

        ImDrawIdx* pIB = (ImDrawIdx*) renderContext.MapBuffer( m_indexBuffer );
        int32_t IBWriteIdx = 0;

        for ( int32_t n = 0; n < pDrawData->CmdListsCount; n++ )
        {
            ImDrawList const* pCmdList = pDrawData->CmdLists[n];

            // Copy vertex / index data
            EE_ASSERT( VBWriteIdx + pCmdList->VtxBuffer.Size <= (int32_t) m_vertexBuffer.GetNumElements() );
            memcpy( &pVB[VBWriteIdx], pCmdList->VtxBuffer.Data, pCmdList->VtxBuffer.Size * sizeof( ImDrawVert ) );
            VBWriteIdx += pCmdList->VtxBuffer.Size;

            EE_ASSERT( IBWriteIdx + pCmdList->IdxBuffer.Size <= (int32_t) m_indexBuffer.GetNumElements() );
            memcpy( &pIB[IBWriteIdx], pCmdList->IdxBuffer.Data, pCmdList->IdxBuffer.Size * sizeof( ImDrawIdx ) );
            IBWriteIdx += pCmdList->IdxBuffer.Size;
        }

        renderContext.UnmapBuffer( m_vertexBuffer );
        renderContext.UnmapBuffer( m_indexBuffer );

        // Set pipeline and render state
        //-------------------------------------------------------------------------

        renderContext.SetViewport( Float2( pDrawData->DisplaySize.x, pDrawData->DisplaySize.y ), Float2( 0, 0 ) );
        renderContext.SetPipelineState( m_PSO );
        renderContext.SetShaderInputBinding( m_inputBinding );
        renderContext.SetPrimitiveTopology( Topology::TriangleList );
        renderContext.SetVertexBuffer( m_vertexBuffer );
        renderContext.SetIndexBuffer( m_indexBuffer );
        renderContext.SetSampler( PipelineStage::Pixel, 0, m_samplerState );
        renderContext.SetDepthTestMode( DepthTestMode::Off );

        // Set MVP matrix
        //-------------------------------------------------------------------------

        float const L = pDrawData->DisplayPos.x;
        float const R = pDrawData->DisplayPos.x + pDrawData->DisplaySize.x;
        float const T = pDrawData->DisplayPos.y;
        float const B = pDrawData->DisplayPos.y + pDrawData->DisplaySize.y;
        float mvp[4][4] =
        {
            { 2.0f / ( R - L ),   0.0f,           0.0f,       0.0f },
            { 0.0f,         2.0f / ( T - B ),     0.0f,       0.0f },
            { 0.0f,         0.0f,           0.5f,       0.0f },
            { ( R + L ) / ( L - R ),  ( T + B ) / ( B - T ),    0.5f,       1.0f },
        };
        renderContext.WriteToBuffer( m_vertexShader.GetConstBuffer( 0 ), &mvp, sizeof( float ) * 16 );

        // Render command lists
        //-------------------------------------------------------------------------

        int32_t vertexOffset = 0;
        int32_t indexOffset = 0;
        for ( int32_t n = 0; n < pDrawData->CmdListsCount; n++ )
        {
            ImDrawList const* pCmdList = pDrawData->CmdLists[n];

            for ( int32_t cmdIdx = 0; cmdIdx < pCmdList->CmdBuffer.Size; cmdIdx++ )
            {
                ImDrawCmd const* pCmd = &pCmdList->CmdBuffer[cmdIdx];
                if ( pCmd->UserCallback != nullptr )
                {
                    EE_UNIMPLEMENTED_FUNCTION();
                }
                else
                {
                    // Project scissor/clipping rectangles into frame buffer space
                    ImVec2 const& clipOffset = pDrawData->DisplayPos;
                    ImVec2 clip_min( pCmd->ClipRect.x - clipOffset.x, pCmd->ClipRect.y - clipOffset.y );
                    ImVec2 clip_max( pCmd->ClipRect.z - clipOffset.x, pCmd->ClipRect.w - clipOffset.y );
                    if ( clip_max.x < clip_min.x || clip_max.y < clip_min.y )
                    {
                        continue;
                    }

                    // Apply scissor/clipping rectangle
                    ScissorRect scissorRect = { (int32_t) clip_min.x, (int32_t) clip_min.y, (int32_t) clip_max.x, (int32_t) clip_max.y };
                    renderContext.SetRasterizerScissorRectangles( &scissorRect, 1 );

                    // Bind texture
                    ViewSRVHandle const* pSRV = reinterpret_cast<ViewSRVHandle const*>( pCmd->TextureId );
                    renderContext.SetShaderResource( PipelineStage::Pixel, 0, *pSRV );

                    // Draw
                    renderContext.DrawIndexed( pCmd->ElemCount, pCmd->IdxOffset + indexOffset, pCmd->VtxOffset + vertexOffset );
                }
            }

            indexOffset += pCmdList->IdxBuffer.Size;
            vertexOffset += pCmdList->VtxBuffer.Size;
        }

        // Clear texture binding
        //-------------------------------------------------------------------------

        renderContext.ClearShaderResource( Render::PipelineStage::Pixel, 0 );
    }
}
#endif