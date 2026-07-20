#include "ImguiRenderer.h"
#include "Engine/Render/RenderSystem.h"
#include "Base/Render/RenderWindow.h"
#include "Base/Profiling.h"
#include "Base/Threading/Threading.h"
#include "imgui.h"

#if EE_DEVELOPMENT_TOOLS
#include "Engine/Render/Shaders/Imgui.esf"
#endif

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Render
{
    static void ImGui_CreateWindowContext( ImGuiViewport* pViewport )
    {
        ImGuiIO&               io = ImGui::GetIO();
        ImGui_BackendUserData* pBackendUserData = static_cast<ImGui_BackendUserData*>( io.BackendRendererUserData );
        EE_ASSERT( pBackendUserData != nullptr );

        //-------------------------------------------------------------------------

        void* hwnd = pViewport->PlatformHandleRaw ? pViewport->PlatformHandleRaw : pViewport->PlatformHandle;
        EE_ASSERT( hwnd );

        auto pUserData = EE::New<ImGui_RendererUserData>();

        pBackendUserData->m_pRenderSystem->WaitGraphicsQueueIdle();

        Int2 windowSize = Int2( int32_t( pViewport->Size.x ), int32_t( pViewport->Size.y ) );

        pUserData->m_renderWindow.SetNativeWindowHandle( hwnd );
        pUserData->m_renderWindow.ResizeSwapchain
        (
            pBackendUserData->m_pRenderSystem->GetContextRHI(),
            pBackendUserData->m_pRenderSystem->GetGraphicsQueue(),
            pBackendUserData->m_pRenderSystem->GetComputeQueue(),
            windowSize
        );

        pBackendUserData->m_pRenderSystem->RegisterRenderWindow( &pUserData->m_renderWindow );

        for ( uint32_t frameIndex = 0; frameIndex < RHI::MaxPendingFrames; ++frameIndex )
        {
            RHI::BufferParameters bufferParameters = {};
            bufferParameters.m_debugName.sprintf( "Imgui Viewport Constant Buffer %i", frameIndex );
            bufferParameters.m_bufferSize = sizeof( ShaderTypes::ImguiConstantBuffer );
            bufferParameters.m_memoryType = RHI::ResourceMemoryType::HostToDevice;
            bufferParameters.m_descriptorTypes = RHI::DescriptorTypeFlags::ConstantBuffer;
            bufferParameters.m_flags = RHI::BufferFlags::PersistentMap;

            pUserData->m_constantBuffers[frameIndex] = RHI::CreateBuffer( pBackendUserData->m_pRenderSystem->GetContextRHI(), bufferParameters );
        }

        pViewport->RendererUserData = pUserData;
    }

    static void ImGui_DestroyWindowContext( ImGuiViewport* pViewport )
    {
        // The main viewport (owned by the application) will always have RendererUserData == NULL since we didn't create the data for it.
        if ( auto pUserData = static_cast<ImGui_RendererUserData*>( pViewport->RendererUserData ) )
        {
            ImGuiIO&               io = ImGui::GetIO();
            ImGui_BackendUserData* pBackendUserData = static_cast<ImGui_BackendUserData*>( io.BackendRendererUserData );
            EE_ASSERT( pBackendUserData != nullptr );

            pBackendUserData->m_pRenderSystem->WaitGraphicsQueueIdle();
            pBackendUserData->m_pRenderSystem->UnregisterRenderWindow( &pUserData->m_renderWindow );
            pUserData->m_renderWindow.DestroySwapchain( pBackendUserData->m_pRenderSystem->GetContextRHI() );

            for ( uint32_t frameIndex = 0; frameIndex < RHI::MaxPendingFrames; ++frameIndex )
            {
                RHI::DestroyBuffer( pBackendUserData->m_pRenderSystem->GetContextRHI(), eastl::move( pUserData->m_constantBuffers[frameIndex] ) );
            }

            EE::Delete( pUserData );
        }

        pViewport->RendererUserData = nullptr;
    }

    static void ImGui_SetWindowSize( ImGuiViewport* pViewport, ImVec2 size )
    {
        ImGuiIO&               io = ImGui::GetIO();
        ImGui_BackendUserData* pBackendUserData = static_cast<ImGui_BackendUserData*>( io.BackendRendererUserData );
        EE_ASSERT( pBackendUserData != nullptr );

        //-------------------------------------------------------------------------

        auto pUserData = static_cast<ImGui_RendererUserData*>( pViewport->RendererUserData );

        Int2 newSize = Int2( int32_t( size.x ), int32_t( size.y ) );

        if ( pUserData->m_renderWindow.GetSwapchainSize() != newSize )
        {
            pBackendUserData->m_pRenderSystem->WaitGraphicsQueueIdle();

            pUserData->m_renderWindow.ResizeSwapchain(
                pBackendUserData->m_pRenderSystem->GetContextRHI(),
                pBackendUserData->m_pRenderSystem->GetGraphicsQueue(),
                pBackendUserData->m_pRenderSystem->GetComputeQueue(),
                newSize );
        }
    }

    void ImguiRenderer::Initialize( Window* pPrimaryRenderWindow, RenderSystem* pRenderSystem )
    {
        m_pRenderSystem = pRenderSystem;
        m_pPrimaryRenderWindow = pPrimaryRenderWindow;

        //---------------------------------------------------------------------------------------------------

        static StringID const            s_ImguiShaderID( "Imgui" );
        SurfaceShader const* pImguiShader = m_pRenderSystem->FindSurfaceShader( s_ImguiShaderID );

        RHI::DataFormat pipelineColorFormats[] = { RHI::DataFormat::RGBA8_sRGB };

        RHI::GraphicsPipelineParameters graphicsPipelineParameters = {};
        graphicsPipelineParameters.m_pRootSignature = pImguiShader->m_pRootSignature;
        graphicsPipelineParameters.m_pShader = pImguiShader->m_pShader;

        graphicsPipelineParameters.m_blendState.m_srcFactors[0] = RHI::BlendConstant::SrcAlpha;
        graphicsPipelineParameters.m_blendState.m_srcAlphaFactors[0] = RHI::BlendConstant::SrcAlpha;
        graphicsPipelineParameters.m_blendState.m_dstFactors[0] = RHI::BlendConstant::OneMinusSrcAlpha;
        graphicsPipelineParameters.m_blendState.m_dstAlphaFactors[0] = RHI::BlendConstant::OneMinusSrcAlpha;
        graphicsPipelineParameters.m_blendState.m_blendModes[0] = RHI::BlendMode::Add;
        graphicsPipelineParameters.m_blendState.m_blendModesAlpha[0] = RHI::BlendMode::Add;
        graphicsPipelineParameters.m_blendState.m_blendEnabled = true;

        graphicsPipelineParameters.m_rasterizerState.m_depthClip = true;
        graphicsPipelineParameters.m_rasterizerState.m_scissor = true;
        graphicsPipelineParameters.m_rasterizerState.m_cullMode = RHI::CullMode::None;

        graphicsPipelineParameters.m_colorFormats = pipelineColorFormats;
        graphicsPipelineParameters.m_numRenderTargets = 1;

        graphicsPipelineParameters.m_debugName = "ImguiPipeline";

        m_pPipeline = RHI::CreatePipeline( m_pRenderSystem->GetContextRHI(), graphicsPipelineParameters );

        //---------------------------------------------------------------------------------------------------

        for ( uint32_t frameIndex = 0; frameIndex < RHI::MaxPendingFrames; ++frameIndex )
        {
            RHI::BufferParameters bufferParameters = {};
            bufferParameters.m_debugName.sprintf( "Imgui Constant Buffer %i", frameIndex );
            bufferParameters.m_bufferSize = sizeof( ShaderTypes::ImguiConstantBuffer );
            bufferParameters.m_memoryType = RHI::ResourceMemoryType::HostToDevice;
            bufferParameters.m_descriptorTypes = RHI::DescriptorTypeFlags::ConstantBuffer;
            bufferParameters.m_flags = RHI::BufferFlags::PersistentMap;

            m_constantBuffers[frameIndex] = RHI::CreateBuffer( m_pRenderSystem->GetContextRHI(), bufferParameters );

            m_vertexBuffers[frameIndex].Initialize( m_pRenderSystem->GetContextRHI() );
            m_indexBuffers[frameIndex].Initialize( m_pRenderSystem->GetContextRHI() );
        }

        ImGuiIO& io = ::ImGui::GetIO();

        ImGui_BackendUserData* pBackendUserData = EE::New<ImGui_BackendUserData>();
        pBackendUserData->m_pRenderWindow = pPrimaryRenderWindow;
        pBackendUserData->m_pRenderSystem = m_pRenderSystem;

        io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;
        io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
        io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
        io.BackendRendererUserData = pBackendUserData;

        // Multiple Viewport Support
        if ( io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable )
        {
            ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
            platformIO.Renderer_CreateWindow = ImGui_CreateWindowContext;
            platformIO.Renderer_DestroyWindow = ImGui_DestroyWindowContext;
            platformIO.Renderer_SetWindowSize = ImGui_SetWindowSize;
        }

        m_imageCache.Initialize( m_pRenderSystem );
    }

    void ImguiRenderer::Shutdown()
    {
        m_imageCache.Shutdown();

        ImGuiIO& io = ::ImGui::GetIO();

        //-------------------------------------------------------------------------

        RHI::DestroyPipeline( m_pRenderSystem->GetContextRHI(), eastl::move( m_pPipeline ) );

        //-------------------------------------------------------------------------

        for ( uint32_t frameIndex = 0; frameIndex < RHI::MaxPendingFrames; ++frameIndex )
        {
            m_vertexBuffers[frameIndex].Shutdown( m_pRenderSystem->GetContextRHI() );
            m_indexBuffers[frameIndex].Shutdown( m_pRenderSystem->GetContextRHI() );
            RHI::DestroyBuffer( m_pRenderSystem->GetContextRHI(), eastl::move( m_constantBuffers[frameIndex] ) );
        }

        //-------------------------------------------------------------------------
        ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();

        for ( ImTextureData* pTexture : platformIO.Textures )
        {
            if ( pTexture->RefCount == 1 )
            {
                RHI::Texture* pTextureRHI = static_cast<RHI::Texture*>( pTexture->BackendUserData );
                RHI::DestroyTexture( m_pRenderSystem->GetContextRHI(), eastl::move( pTextureRHI ) );

                pTexture->BackendUserData = nullptr;
                pTexture->TexID = {};
                pTexture->SetStatus( ImTextureStatus_Destroyed );
            }
        }

        ImGui::DestroyPlatformWindows();

        ImGui_BackendUserData* pBackendUserData = static_cast<ImGui_BackendUserData*>( io.BackendRendererUserData );
        EE::Delete( pBackendUserData );

        io.BackendRendererUserData = nullptr;

        m_pRenderSystem = nullptr;
    }

    #if EE_DEVELOPMENT_TOOLS
    void ImguiRenderer::PreShaderHotReload()
    {
        static StringID const            s_ImguiShaderID( "Imgui" );
        SurfaceShader const* pImguiShader = m_pRenderSystem->FindSurfaceShader( s_ImguiShaderID );

        RHI::DataFormat pipelineColorFormats[] = { RHI::DataFormat::RGBA8_sRGB };

        RHI::GraphicsPipelineParameters graphicsPipelineParameters = {};
        graphicsPipelineParameters.m_pRootSignature = pImguiShader->m_pRootSignature;
        graphicsPipelineParameters.m_pShader = pImguiShader->m_pShader;

        graphicsPipelineParameters.m_blendState.m_srcFactors[0] = RHI::BlendConstant::SrcAlpha;
        graphicsPipelineParameters.m_blendState.m_srcAlphaFactors[0] = RHI::BlendConstant::SrcAlpha;
        graphicsPipelineParameters.m_blendState.m_dstFactors[0] = RHI::BlendConstant::OneMinusSrcAlpha;
        graphicsPipelineParameters.m_blendState.m_dstAlphaFactors[0] = RHI::BlendConstant::OneMinusSrcAlpha;
        graphicsPipelineParameters.m_blendState.m_blendModes[0] = RHI::BlendMode::Add;
        graphicsPipelineParameters.m_blendState.m_blendModesAlpha[0] = RHI::BlendMode::Add;
        graphicsPipelineParameters.m_blendState.m_blendEnabled = true;

        graphicsPipelineParameters.m_rasterizerState.m_depthClip = true;
        graphicsPipelineParameters.m_rasterizerState.m_scissor = true;
        graphicsPipelineParameters.m_rasterizerState.m_cullMode = RHI::CullMode::None;

        graphicsPipelineParameters.m_colorFormats = pipelineColorFormats;
        graphicsPipelineParameters.m_numRenderTargets = 1;

        graphicsPipelineParameters.m_debugName = "ImguiPipeline";

        m_pPipeline = RHI::CreatePipeline( m_pRenderSystem->GetContextRHI(), graphicsPipelineParameters );
    }

    void ImguiRenderer::PostShaderHotReload()
    {
        RHI::DestroyPipeline( m_pRenderSystem->GetContextRHI(), eastl::move( m_pPipeline ) );
    }
    #endif

    void ImguiRenderer::UpdateDeviceResources()
    {
        ImDrawData const* pPrimaryDrawData = ImGui::GetDrawData();
        UpdateTextureData( pPrimaryDrawData, m_pRenderSystem );

        if ( ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable )
        {
            ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();

            for ( int i = 1; i < platformIO.Viewports.Size; i++ )
            {
                ImGuiViewport* pViewport = platformIO.Viewports[i];

                if ( pViewport->Flags & ImGuiViewportFlags_IsMinimized )
                {
                    continue;
                }

                if ( !pViewport->DrawData )
                {
                    continue;
                }

                UpdateTextureData( pViewport->DrawData, m_pRenderSystem );
            }
        }
    }

    void ImguiRenderer::StartFrame()
    {
        uint32_t            frameIndex = m_pRenderSystem->GetFrameIndex();
        RHI::Context*       pContextRHI = m_pRenderSystem->GetContextRHI();

        if ( ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable )
        {
            ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();

            for ( int i = 1; i < platformIO.Viewports.Size; i++ )
            {
                ImGuiViewport* pViewport = platformIO.Viewports[i];
                if ( pViewport->Flags & ImGuiViewportFlags_IsMinimized )
                {
                    continue;
                }

                auto pUserData = static_cast<ImGui_RendererUserData*>( pViewport->RendererUserData );
                if ( pUserData )
                {
                    RHI::MapBuffer( pContextRHI, pUserData->m_constantBuffers[frameIndex], {} );
                }
            }
        }
    }

    void ImguiRenderer::SubmitFrame( bool clear )
    {
        uint32_t            frameIndex = m_pRenderSystem->GetFrameIndex();
        RHI::Context*       pContextRHI = m_pRenderSystem->GetContextRHI();

        ImGuiIO& io = ImGui::GetIO();

        uint32_t           totalImguiVertices = 0;
        uint32_t           totalImguiIndices = 0;
        ImguiGeometryState state = {};

        ImDrawData const* pPrimaryDrawData = ImGui::GetDrawData();
        if ( pPrimaryDrawData )
        {
            totalImguiVertices += pPrimaryDrawData->TotalVtxCount;
            totalImguiIndices += pPrimaryDrawData->TotalIdxCount;
        }

        if ( ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable )
        {
            ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();

            for ( int i = 1; i < platformIO.Viewports.Size; i++ )
            {
                ImGuiViewport* pViewport = platformIO.Viewports[i];
                if ( pViewport->Flags & ImGuiViewportFlags_IsMinimized )
                {
                    continue;
                }

                if ( pViewport->DrawData )
                {
                    totalImguiVertices += pViewport->DrawData->TotalVtxCount;
                    totalImguiIndices += pViewport->DrawData->TotalIdxCount;
                }

                auto pUserData = static_cast<ImGui_RendererUserData*>( pViewport->RendererUserData );
                if ( pUserData )
                {
                    RHI::MapBuffer( pContextRHI, pUserData->m_constantBuffers[frameIndex], {} );
                }
            }
        }

        // Begin recording command buffers
        //-------------------------------------------------------------------------

        RHI::CommandBuffer* pPrimaryCommandBuffer = m_pPrimaryRenderWindow->GetActiveCommandBuffer( frameIndex );

        if ( totalImguiVertices <= 0 || totalImguiIndices <= 0 )
        {
            return;
        }

        //-------------------------------------------------------------------------

        auto UpdateBuffer_ImguiVertex = [pContextRHI, frameIndex] ( RHI::Buffer* && pOldBuffer, size_t newBufferSize )
        {
            RHI::DestroyBuffer( pContextRHI, eastl::move( pOldBuffer ) );

            RHI::BufferParameters bufferParameters = {};
            bufferParameters.m_debugName.sprintf( "Imgui Vertex Buffer %i", frameIndex );
            bufferParameters.m_descriptorTypes = RHI::DescriptorTypeFlags::Buffer;
            bufferParameters.m_memoryType = RHI::ResourceMemoryType::HostToDevice;
            bufferParameters.m_bufferSize = newBufferSize;
            bufferParameters.m_bufferStride = sizeof( ImDrawVert );
            bufferParameters.m_flags = RHI::BufferFlags::PersistentMap;

            return RHI::CreateBuffer( pContextRHI, bufferParameters );
        };

        m_vertexBuffers[frameIndex].UpdateDeviceResources( totalImguiVertices * sizeof( ImDrawVert ), UpdateBuffer_ImguiVertex );

        //-------------------------------------------------------------------------

        auto UpdateBuffer_ImguiIndex = [pContextRHI, frameIndex] ( RHI::Buffer* && pOldBuffer, size_t newBufferSize )
        {
            RHI::DestroyBuffer( pContextRHI, eastl::move( pOldBuffer ) );

            RHI::BufferParameters bufferParameters = {};
            bufferParameters.m_debugName.sprintf( "Imgui Index Buffer %i", frameIndex );
            bufferParameters.m_descriptorTypes = RHI::DescriptorTypeFlags::IndexBuffer;
            bufferParameters.m_memoryType = RHI::ResourceMemoryType::HostToDevice;
            bufferParameters.m_bufferSize = newBufferSize;
            bufferParameters.m_bufferStride = sizeof( ImDrawIdx );
            bufferParameters.m_flags = RHI::BufferFlags::PersistentMap;
            bufferParameters.m_format = RHI::DataFormat::R32_UInt;

            return RHI::CreateBuffer( pContextRHI, bufferParameters );
        };

        m_indexBuffers[frameIndex].UpdateDeviceResources( totalImguiIndices * sizeof( ImDrawIdx ), UpdateBuffer_ImguiIndex );

        //-------------------------------------------------------------------------

        if ( pPrimaryDrawData->TotalVtxCount > 0 && pPrimaryDrawData->TotalIdxCount > 0 )
        {
            // Set MVP matrix and other constants
            //-------------------------------------------------------------------------

            float const L = pPrimaryDrawData->DisplayPos.x;
            float const R = pPrimaryDrawData->DisplayPos.x + pPrimaryDrawData->DisplaySize.x;
            float const T = pPrimaryDrawData->DisplayPos.y;
            float const B = pPrimaryDrawData->DisplayPos.y + pPrimaryDrawData->DisplaySize.y;
            float       mvp[4][4] = {
                { 2.0f / ( R - L ), 0.0f, 0.0f, 0.0f },
                { 0.0f, 2.0f / ( T - B ), 0.0f, 0.0f },
                { 0.0f, 0.0f, 0.5f, 0.0f },
                { ( R + L ) / ( L - R ), ( T + B ) / ( B - T ), 0.5f, 1.0f },
            };

            ShaderTypes::ImguiConstantBuffer alignas( 32 ) constantBufferData = {};
            std::memcpy( constantBufferData.m_projectionMatrix, mvp, sizeof( float ) * 16 );
            constantBufferData.m_vertexBuffer = RHI::GetBufferHandle( m_vertexBuffers[frameIndex].m_buffer, RHI::DescriptorTypeFlags::Buffer );

            Memory::CopyToWriteCombined( m_constantBuffers[frameIndex]->m_pMappedAddress_WriteCombined, &constantBufferData, sizeof( constantBufferData ) );

            RenderImguiData(
                pPrimaryDrawData,
                m_pRenderSystem,
                pPrimaryCommandBuffer,
                m_pPrimaryRenderWindow->GetCurrentRenderTarget(),
                m_pPipeline,
                m_constantBuffers[frameIndex],
                m_vertexBuffers[frameIndex].m_buffer,
                m_indexBuffers[frameIndex].m_buffer,
                frameIndex,
                clear,
                state );
        }

        // Viewport Support
        //-------------------------------------------------------------------------

        if ( io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable )
        {
            ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();

            for ( int i = 1; i < platformIO.Viewports.Size; i++ )
            {
                ImGuiViewport* pViewport = platformIO.Viewports[i];
                if ( pViewport->Flags & ImGuiViewportFlags_IsMinimized )
                {
                    continue;
                }

                if ( !pViewport->DrawData || pViewport->DrawData->TotalVtxCount <= 0 || pViewport->DrawData->TotalIdxCount <= 0 )
                {
                    continue;
                }

                auto pUserData = static_cast<ImGui_RendererUserData*>( pViewport->RendererUserData );
                if ( !pUserData )
                {
                    continue;
                }

                // Set MVP matrix and other constants
                //-------------------------------------------------------------------------

                float const L = pViewport->DrawData->DisplayPos.x;
                float const R = pViewport->DrawData->DisplayPos.x + pViewport->DrawData->DisplaySize.x;
                float const T = pViewport->DrawData->DisplayPos.y;
                float const B = pViewport->DrawData->DisplayPos.y + pViewport->DrawData->DisplaySize.y;
                float       mvp[4][4] = {
                    { 2.0f / ( R - L ), 0.0f, 0.0f, 0.0f },
                    { 0.0f, 2.0f / ( T - B ), 0.0f, 0.0f },
                    { 0.0f, 0.0f, 0.5f, 0.0f },
                    { ( R + L ) / ( L - R ), ( T + B ) / ( B - T ), 0.5f, 1.0f },
                };
                ShaderTypes::ImguiConstantBuffer alignas( 32 ) constantBufferData = {};
                std::memcpy( constantBufferData.m_projectionMatrix, mvp, sizeof( float ) * 16 );
                constantBufferData.m_vertexBuffer = RHI::GetBufferHandle( m_vertexBuffers[frameIndex].m_buffer, RHI::DescriptorTypeFlags::Buffer );

                Memory::CopyToWriteCombined( pUserData->m_constantBuffers[frameIndex]->m_pMappedAddress_WriteCombined, &constantBufferData, sizeof( constantBufferData ) );

                RenderImguiData(
                    pViewport->DrawData,
                    m_pRenderSystem,
                    pUserData->m_renderWindow.GetActiveCommandBuffer( frameIndex ),
                    pUserData->m_renderWindow.GetCurrentRenderTarget(),
                    m_pPipeline,
                    pUserData->m_constantBuffers[frameIndex],
                    m_vertexBuffers[frameIndex].m_buffer,
                    m_indexBuffers[frameIndex].m_buffer,
                    frameIndex,
                    true,
                    state );
            }
        }
    }

    void ImguiRenderer::EndFrame()
    {
        if ( ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable )
        {
            ImGui::UpdatePlatformWindows();
        }
    }

    void ImguiRenderer::RenderImguiData(
        ImDrawData const*                        pDrawData,
        RenderSystem*                            pRenderSystem,
        RHI::CommandBuffer*                      pCommandBuffer,
        RHI::Texture*                            pRenderTarget,
        RHI::Pipeline*                           pPipeline,
        RHI::Buffer*                             pConstantBuffer,
        RHI::Buffer*                             pVertexBuffer,
        RHI::Buffer*                             pIndexBuffer,
        uint32_t                                 frameIndex,
        bool                                     clear,
        ImguiRenderer::ImguiGeometryState& state )
    {
        if ( pDrawData->DisplaySize.x <= 0.0f || pDrawData->DisplaySize.y <= 0.0f )
        {
            return;
        }

        EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, "Imgui viewport" );

        RHI::LoadAction loadAction = {};
        loadAction.m_loadActionsColor[0] = RHI::LoadActionType::Load;
        if ( clear )
        {
            loadAction.m_loadActionsColor[0] = RHI::LoadActionType::Clear;
        }

        uint32_t viewportWidth = pRenderTarget->m_width;
        uint32_t viewportHeight = pRenderTarget->m_height;

        RHI::CmdSetRenderTargets( pCommandBuffer, { &pRenderTarget, 1 }, nullptr, &loadAction );
        RHI::CmdSetViewport( pCommandBuffer, 0.0F, 0.0F,
                             float( viewportWidth ),
                             float( viewportHeight ),
                             0.0F, 1.0F );
        RHI::CmdSetScissor( pCommandBuffer, 0, 0, viewportWidth, viewportHeight );

        RHI::CmdSetPipeline( pCommandBuffer, pPipeline );
        RHI::CmdSetRootParameter( pCommandBuffer, 1, pConstantBuffer, 0 );
        RHI::CmdSetIndexBuffer( pCommandBuffer, pIndexBuffer, RHI::IndexType::Uint32, 0 );

        ImDrawVert* pVB = static_cast<ImDrawVert*>( pVertexBuffer->m_pMappedAddress_WriteCombined );
        ImDrawIdx*  pIB = static_cast<ImDrawIdx*>( pIndexBuffer->m_pMappedAddress_WriteCombined );

        for ( int32_t cmdListIndex = 0; cmdListIndex < pDrawData->CmdListsCount; ++cmdListIndex )
        {
            ImDrawList const* pCmdList = pDrawData->CmdLists[cmdListIndex];

            EE_ASSERT( ( state.vertexOffset + pCmdList->VtxBuffer.Size ) <= uint32_t( pVertexBuffer->m_size / pVertexBuffer->m_stride ) );
            memcpy( pVB + state.vertexOffset, pCmdList->VtxBuffer.Data, pCmdList->VtxBuffer.Size * sizeof( ImDrawVert ) );

            EE_ASSERT( ( state.indexOffset + pCmdList->IdxBuffer.Size ) <= uint32_t( pIndexBuffer->m_size / pIndexBuffer->m_stride ) );
            memcpy( pIB + state.indexOffset, pCmdList->IdxBuffer.Data, pCmdList->IdxBuffer.Size * sizeof( ImDrawIdx ) );

            for ( int32_t cmdIdx = 0; cmdIdx < pCmdList->CmdBuffer.Size; ++cmdIdx )
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
                    ImVec2        clipMin( pCmd->ClipRect.x - clipOffset.x, pCmd->ClipRect.y - clipOffset.y );
                    ImVec2        clipMax( pCmd->ClipRect.z - clipOffset.x, pCmd->ClipRect.w - clipOffset.y );

                    uint32_t clipMinX = uint32_t( clipMin.x );
                    uint32_t clipMinY = uint32_t( clipMin.y );
                    uint32_t clipMaxX = uint32_t( clipMax.x );
                    uint32_t clipMaxY = uint32_t( clipMax.y );

                    if ( clipMaxX <= clipMinX || clipMaxY <= clipMinY )
                    {
                        continue;
                    }

                    RHI::SamplerStateHandle colorSampler;
                    RHI::TextureHandle      colorTexture;

                    ImTextureID_Unpack( pCmd->GetTexID(), colorSampler, colorTexture );

                    ShaderTypes::ImguiRootConstants rootConstants = {};
                    rootConstants.m_vertexOffset = pCmd->VtxOffset + state.vertexOffset;
                    rootConstants.m_colorSampler = colorSampler;
                    rootConstants.m_colorTexture = colorTexture;

                    RHI::CmdSetRootConstants( pCommandBuffer, 0, &rootConstants, sizeof( rootConstants ) );
                    RHI::CmdSetScissor( pCommandBuffer, clipMinX, clipMinY, clipMaxX - clipMinX, clipMaxY - clipMinY );
                    RHI::CmdDrawIndexed( pCommandBuffer, pCmd->ElemCount, pCmd->IdxOffset + state.indexOffset );
                }
            }

            state.indexOffset += pCmdList->IdxBuffer.Size;
            state.vertexOffset += pCmdList->VtxBuffer.Size;
        }

        RHI::CmdSetRenderTargets( pCommandBuffer, {}, nullptr );
    }

    void ImguiRenderer::UpdateTextureData( ImDrawData const* pDrawData, RenderSystem* pRenderSystem )
    {
        if ( pDrawData->Textures )
        {
            for ( ImTextureData* pTexture : *pDrawData->Textures )
            {
                if ( pTexture->Status == ImTextureStatus_WantCreate )
                {
                    RHI::TextureParameters textureParameters = {};
                    textureParameters.m_width = pTexture->Width;
                    textureParameters.m_height = pTexture->Height;
                    textureParameters.m_format = RHI::DataFormat::RGBA8_UNorm;
                    textureParameters.m_initialState = RHI::TextureState::Common;
                    textureParameters.m_debugName.sprintf( "Imgui Texture %i", pTexture->UniqueID );

                    auto CopyTextureMemory = [pTexture] ( uint8_t* pDstMemory_WriteCombined, size_t srcOffset, uint32_t rowStride, uint32_t row )
                    {
                        uint8_t* pPixels = reinterpret_cast<uint8_t*>( pTexture->GetPixels() );

                        // TODO: pPixels are not aligned!
                        //Memory::CopyToWriteCombined( pDstMemory_WriteCombined, pPixels + srcOffset, rowStride );
                        std::memcpy( pDstMemory_WriteCombined, pPixels + srcOffset, rowStride );
                    };
                    RHI::Texture* pTextureRHI = pRenderSystem->QueueTextureCreate( CopyTextureMemory, textureParameters );

                    pTexture->BackendUserData = pTextureRHI;
                    pTexture->TexID = ImTextureID_Pack(
                        RHI::GetSamplerStateHandle( pRenderSystem->GetLinearWrapSampler() ),
                        RHI::GetTextureHandle( pTextureRHI, RHI::DescriptorTypeFlags::Texture, 0 )
                    );

                    pTexture->SetStatus( ImTextureStatus_OK );
                }

                if ( pTexture->Status == ImTextureStatus_WantUpdates )
                {
                    RHI::Texture* pTextureRHI = static_cast<RHI::Texture*>( pTexture->BackendUserData );

                    RHI::TextureCopyRegion copyRegion = {};
                    copyRegion.m_x = pTexture->UpdateRect.x;
                    copyRegion.m_y = pTexture->UpdateRect.y;
                    copyRegion.m_width = pTexture->UpdateRect.w;
                    copyRegion.m_height = pTexture->UpdateRect.h;

                    auto CopyTextureMemory = [pTexture] ( uint8_t* pDstMemory_WriteCombined, size_t srcOffset, uint32_t rowStride, uint32_t row )
                    {
                        uint8_t* pPixels = reinterpret_cast<uint8_t*>( pTexture->GetPixelsAt( pTexture->UpdateRect.x, pTexture->UpdateRect.y + row ) );

                        // TODO: pPixels are not aligned!
                        //Memory::CopyToWriteCombined( pDstMemory_WriteCombined, pPixels + srcOffset, rowStride );
                        std::memcpy( pDstMemory_WriteCombined, pPixels, rowStride );
                    };
                    pRenderSystem->QueueTextureUpdate( CopyTextureMemory, pTextureRHI, copyRegion, 1, 1, RHI::TextureState::Common );

                    pTexture->SetStatus( ImTextureStatus_OK );
                }

                if ( pTexture->Status == ImTextureStatus_WantDestroy )
                {
                    RHI::Texture* pTextureRHI = static_cast<RHI::Texture*>( pTexture->BackendUserData );
                    pRenderSystem->QueueResourceDelete( eastl::move( pTextureRHI ) );

                    pTexture->BackendUserData = nullptr;
                    pTexture->TexID = {};
                    pTexture->SetStatus( ImTextureStatus_Destroyed );
                }
            }
        }
    }
}
#endif