#include "DebugRenderStates.h"
#include "Engine/Render/Shaders/DebugShaders.h"
#include "System/ThirdParty/stb/stb_truetype.h"
#include "System/Fonts/FontData_Proggy.h"
#include "System/Fonts/FontDecompressor.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Render
{
    bool DebugPointRenderState::Initialize( RenderDevice* pRenderDevice )
    {
        EE_ASSERT( pRenderDevice != nullptr );

        TVector<RenderBuffer> cbuffers;

        //-------------------------------------------------------------------------
        // VERTEX SHADER
        //-------------------------------------------------------------------------

        // World transform const buffer
        RenderBuffer buffer;
        buffer.m_byteSize = 16 * 5; // float4x4 + float2
        buffer.m_byteStride = 16;   // Vector4 aligned
        buffer.m_usage = RenderBuffer::Usage::CPU_and_GPU;
        buffer.m_type = RenderBuffer::Type::Constant;
        cbuffers.push_back( buffer );

        // VS
        VertexLayoutDescriptor vertexLayoutDesc;
        vertexLayoutDesc.m_elementDescriptors.push_back( VertexLayoutDescriptor::ElementDescriptor( DataSemantic::Position, DataFormat::Float_R32G32B32A32, 0, 0 ) );
        vertexLayoutDesc.m_elementDescriptors.push_back( VertexLayoutDescriptor::ElementDescriptor( DataSemantic::Color, DataFormat::Float_R32G32B32A32, 0, 16 ) );
        vertexLayoutDesc.CalculateByteSize();

        m_vertexShader = VertexShader( g_byteCode_VS_DebugRendererPoints, sizeof( g_byteCode_VS_DebugRendererPoints ), cbuffers, vertexLayoutDesc );
        pRenderDevice->CreateShader( m_vertexShader );
        cbuffers.clear();

        // Create VB and input binding
        m_vertexBuffer.m_byteStride = sizeof( float ) * 9;
        m_vertexBuffer.m_byteSize = m_vertexBuffer.m_byteStride * MaxPointsPerDrawCall;
        m_vertexBuffer.m_type = RenderBuffer::Type::Vertex;
        m_vertexBuffer.m_usage = RenderBuffer::Usage::CPU_and_GPU;
        pRenderDevice->CreateBuffer( m_vertexBuffer, nullptr );

        if ( !m_vertexBuffer.IsValid() )
        {
            return false;
        }

        // Create vertex layout and input binding
        pRenderDevice->CreateShaderInputBinding( m_vertexShader, vertexLayoutDesc, m_inputBinding );

        // Allocate staging memory for vertex data
        m_stagingVertexData.resize( m_vertexBuffer.m_byteSize );

        //-------------------------------------------------------------------------
        // GEOMETRY SHADER
        //-------------------------------------------------------------------------

        // View dimensions
        buffer.m_byteSize = 16 * 5; // float4x4 + float2
        buffer.m_byteStride = 16;   // Vector4 aligned
        buffer.m_usage = RenderBuffer::Usage::CPU_and_GPU;
        buffer.m_type = RenderBuffer::Type::Constant;
        cbuffers.push_back( buffer );

        // GS
        m_geometryShader = GeometryShader( g_byteCode_GS_DebugRendererPoints, sizeof( g_byteCode_GS_DebugRendererPoints ), cbuffers );
        pRenderDevice->CreateShader( m_geometryShader );
        cbuffers.clear();

        //-------------------------------------------------------------------------
        // PIXEL SHADER
        //-------------------------------------------------------------------------

        // PS
        m_pixelShader = PixelShader( g_byteCode_PS_DebugRendererPoints, sizeof( g_byteCode_PS_DebugRendererPoints ), cbuffers );
        pRenderDevice->CreateShader( m_pixelShader );
        cbuffers.clear();

        //-------------------------------------------------------------------------
        // RASTERIZER
        //-------------------------------------------------------------------------

        // Blend state for transparency
        {
            m_blendState.m_srcValue = BlendValue::SourceAlpha;
            m_blendState.m_dstValue = BlendValue::InverseSourceAlpha;
            m_blendState.m_blendOp = BlendOp::Add;
            m_blendState.m_srcAlphaValue = BlendValue::Zero;
            m_blendState.m_dstAlphaValue = BlendValue::One;
            m_blendState.m_blendOpAlpha = BlendOp::Add;
            m_blendState.m_blendEnable = true;

            pRenderDevice->CreateBlendState( m_blendState );

            if ( !m_blendState.IsValid() )
            {
                return false;
            }
        }

        // Set Rasterizer state for drawing
        {
            m_rasterizerState.m_cullMode = CullMode::None;
            m_rasterizerState.m_windingMode = WindingMode::CounterClockwise;
            m_rasterizerState.m_fillMode = FillMode::Solid;

            pRenderDevice->CreateRasterizerState( m_rasterizerState );

            if ( !m_rasterizerState.IsValid() )
            {
                return false;
            }
        }

        // Set up PSO
        m_PSO.m_pVertexShader = &m_vertexShader;
        m_PSO.m_pGeometryShader = &m_geometryShader;
        m_PSO.m_pPixelShader = &m_pixelShader;
        m_PSO.m_pBlendState = &m_blendState;
        m_PSO.m_pRasterizerState = &m_rasterizerState;

        return true;
    }

    void DebugPointRenderState::Shutdown( RenderDevice* pRenderDevice )
    {
        EE_ASSERT( pRenderDevice != nullptr );

        m_PSO.Clear();

        if ( m_blendState.IsValid() )
        {
            pRenderDevice->DestroyBlendState( m_blendState );
        }

        if ( m_rasterizerState.IsValid() )
        {
            pRenderDevice->DestroyRasterizerState( m_rasterizerState );
        }

        if ( m_vertexBuffer.IsValid() )
        {
            pRenderDevice->DestroyBuffer( m_vertexBuffer );
        }

        pRenderDevice->DestroyShaderInputBinding( m_inputBinding );
        pRenderDevice->DestroyShader( m_vertexShader );
        pRenderDevice->DestroyShader( m_geometryShader );
        pRenderDevice->DestroyShader( m_pixelShader );
    }

    void DebugPointRenderState::SetState( RenderContext const& renderContext, Viewport const& viewport )
    {
        renderContext.SetPipelineState( m_PSO );
        renderContext.SetVertexBuffer( m_vertexBuffer );
        renderContext.SetShaderInputBinding( m_inputBinding );

        struct CB
        {
            Matrix m_viewProjectionMatrix;
            Float4 m_viewport;
        };

        CB cb;
        cb.m_viewProjectionMatrix = viewport.GetViewVolume().GetViewProjectionMatrix();
        cb.m_viewport.m_x = (float) viewport.GetDimensions().m_x;
        cb.m_viewport.m_y = (float) viewport.GetDimensions().m_y;

        renderContext.WriteToBuffer( m_vertexShader.GetConstBuffer( 0 ), &cb, sizeof( CB ) );
        renderContext.WriteToBuffer( m_geometryShader.GetConstBuffer( 0 ), &cb, sizeof( CB ) );
    }

    //-------------------------------------------------------------------------

    bool DebugLineRenderState::Initialize( RenderDevice* pRenderDevice )
    {
        EE_ASSERT( pRenderDevice != nullptr );

        TVector<RenderBuffer> cbuffers;

        //-------------------------------------------------------------------------
        // VERTEX SHADER
        //-------------------------------------------------------------------------

        // World transform const buffer
        RenderBuffer buffer;
        buffer.m_byteSize = 16 * 5; // float4x4 + float2
        buffer.m_byteStride = 16;   // Vector4 aligned
        buffer.m_usage = RenderBuffer::Usage::CPU_and_GPU;
        buffer.m_type = RenderBuffer::Type::Constant;
        cbuffers.push_back( buffer );

        // VS
        VertexLayoutDescriptor vertexLayoutDesc;
        vertexLayoutDesc.m_elementDescriptors.push_back( VertexLayoutDescriptor::ElementDescriptor( DataSemantic::Position, DataFormat::Float_R32G32B32A32, 0, 0 ) );
        vertexLayoutDesc.m_elementDescriptors.push_back( VertexLayoutDescriptor::ElementDescriptor( DataSemantic::Color, DataFormat::Float_R32G32B32A32, 0, 16 ) );
        vertexLayoutDesc.CalculateByteSize();

        m_vertexShader = VertexShader( g_byteCode_VS_DebugRendererLines, sizeof( g_byteCode_VS_DebugRendererLines ), cbuffers, vertexLayoutDesc );
        pRenderDevice->CreateShader( m_vertexShader );
        cbuffers.clear();

        // Create VB and input binding
        m_vertexBuffer.m_byteStride = sizeof( float ) * 9;
        m_vertexBuffer.m_byteSize = m_vertexBuffer.m_byteStride * 2 * MaxLinesPerDrawCall;
        m_vertexBuffer.m_type = RenderBuffer::Type::Vertex;
        m_vertexBuffer.m_usage = RenderBuffer::Usage::CPU_and_GPU;
        pRenderDevice->CreateBuffer( m_vertexBuffer, nullptr );

        if ( !m_vertexBuffer.IsValid() )
        {
            return false;
        }

        // Create vertex layout and input binding
        pRenderDevice->CreateShaderInputBinding( m_vertexShader, vertexLayoutDesc, m_inputBinding );

        // Allocate staging memory for vertex data
        m_stagingVertexData.resize( m_vertexBuffer.m_byteSize );

        //-------------------------------------------------------------------------
        // GEOMETRY SHADER
        //-------------------------------------------------------------------------

        // View dimensions
        buffer.m_byteSize = 16 * 5; // float4x4 + float2
        buffer.m_byteStride = 16;   // Vector4 aligned
        buffer.m_usage = RenderBuffer::Usage::CPU_and_GPU;
        buffer.m_type = RenderBuffer::Type::Constant;
        cbuffers.push_back( buffer );

        // GS
        m_geometryShader = GeometryShader( g_byteCode_GS_DebugRendererLines, sizeof( g_byteCode_GS_DebugRendererLines ), cbuffers );
        pRenderDevice->CreateShader( m_geometryShader );
        cbuffers.clear();

        //-------------------------------------------------------------------------
        // PIXEL SHADER
        //-------------------------------------------------------------------------

        // Selection ID buffer
        buffer.m_byteSize = 16;
        buffer.m_byteStride = 16;
        buffer.m_usage = RenderBuffer::Usage::CPU_and_GPU;
        buffer.m_type = RenderBuffer::Type::Constant;
        cbuffers.push_back( buffer );

        // PS
        m_pixelShader = PixelShader( g_byteCode_PS_DebugRendererLines, sizeof( g_byteCode_PS_DebugRendererLines ), cbuffers );
        pRenderDevice->CreateShader( m_pixelShader );
        cbuffers.clear();

        //-------------------------------------------------------------------------
        // RASTERIZER
        //-------------------------------------------------------------------------

        // Blend state for transparency
        {
            m_blendState.m_srcValue = BlendValue::SourceAlpha;
            m_blendState.m_dstValue = BlendValue::InverseSourceAlpha;
            m_blendState.m_blendOp = BlendOp::Add;
            m_blendState.m_srcAlphaValue = BlendValue::Zero;
            m_blendState.m_dstAlphaValue = BlendValue::One;
            m_blendState.m_blendOpAlpha = BlendOp::Add;
            m_blendState.m_blendEnable = true;

            pRenderDevice->CreateBlendState( m_blendState );

            if ( !m_blendState.IsValid() )
            {
                return false;
            }
        }

        // Set Rasterizer state for drawing
        {
            m_rasterizerState.m_cullMode = CullMode::None;
            m_rasterizerState.m_windingMode = WindingMode::CounterClockwise;
            m_rasterizerState.m_fillMode = FillMode::Solid;

            pRenderDevice->CreateRasterizerState( m_rasterizerState );

            if ( !m_rasterizerState.IsValid() )
            {
                return false;
            }
        }

        // Set up PSO
        m_PSO.m_pVertexShader = &m_vertexShader;
        m_PSO.m_pGeometryShader = &m_geometryShader;
        m_PSO.m_pPixelShader = &m_pixelShader;
        m_PSO.m_pBlendState = &m_blendState;
        m_PSO.m_pRasterizerState = &m_rasterizerState;

        return true;
    }

    void DebugLineRenderState::Shutdown( RenderDevice* pRenderDevice )
    {
        EE_ASSERT( pRenderDevice != nullptr );

        m_PSO.Clear();

        if ( m_blendState.IsValid() )
        {
            pRenderDevice->DestroyBlendState( m_blendState );
        }

        if ( m_rasterizerState.IsValid() )
        {
            pRenderDevice->DestroyRasterizerState( m_rasterizerState );
        }

        if ( m_vertexBuffer.IsValid() )
        {
            pRenderDevice->DestroyBuffer( m_vertexBuffer );
        }

        pRenderDevice->DestroyShaderInputBinding( m_inputBinding );
        pRenderDevice->DestroyShader( m_vertexShader );
        pRenderDevice->DestroyShader( m_geometryShader );
        pRenderDevice->DestroyShader( m_pixelShader );
    }

    void DebugLineRenderState::SetState( RenderContext const& renderContext, Viewport const& viewport )
    {
        renderContext.SetPipelineState( m_PSO );
        renderContext.SetVertexBuffer( m_vertexBuffer );
        renderContext.SetShaderInputBinding( m_inputBinding );

        struct CB
        {
            Matrix m_viewProjectionMatrix;
            Float4 m_viewport;
        };

        CB cb;
        cb.m_viewProjectionMatrix = viewport.GetViewVolume().GetViewProjectionMatrix();
        cb.m_viewport.m_x = (float) viewport.GetDimensions().m_x;
        cb.m_viewport.m_y = (float) viewport.GetDimensions().m_y;

        renderContext.WriteToBuffer( m_vertexShader.GetConstBuffer( 0 ), &cb, sizeof( CB ) );
        renderContext.WriteToBuffer( m_geometryShader.GetConstBuffer( 0 ), &cb, sizeof( CB ) );
    }

    //-------------------------------------------------------------------------

    bool DebugPrimitiveRenderState::Initialize( RenderDevice* pRenderDevice )
    {
        EE_ASSERT( pRenderDevice != nullptr );

        TVector<RenderBuffer> cbuffers;

        // Global const buffer
        RenderBuffer buffer;
        buffer.m_byteSize = 16 * 5; // float4x4 + float2
        buffer.m_byteStride = 16;   // Vector4 aligned
        buffer.m_usage = RenderBuffer::Usage::CPU_and_GPU;
        buffer.m_type = RenderBuffer::Type::Constant;
        cbuffers.push_back( buffer );

        // VS
        VertexLayoutDescriptor vertexLayoutDesc;
        vertexLayoutDesc.m_elementDescriptors.push_back( VertexLayoutDescriptor::ElementDescriptor( DataSemantic::Position, DataFormat::Float_R32G32B32A32, 0, 0 ) );
        vertexLayoutDesc.m_elementDescriptors.push_back( VertexLayoutDescriptor::ElementDescriptor( DataSemantic::Color, DataFormat::Float_R32G32B32A32, 0, 16 ) );
        vertexLayoutDesc.CalculateByteSize();

        m_vertexShader = VertexShader( g_byteCode_VS_DebugRendererTriangles, sizeof( g_byteCode_VS_DebugRendererTriangles ), cbuffers, vertexLayoutDesc );
        pRenderDevice->CreateShader( m_vertexShader );
        cbuffers.clear();

        // Create vertex buffer
        m_vertexBuffer.m_byteStride = sizeof( float ) * 9;
        m_vertexBuffer.m_byteSize = m_vertexBuffer.m_byteStride * 3 * MaxTrianglesPerDrawCall;
        m_vertexBuffer.m_type = RenderBuffer::Type::Vertex;
        m_vertexBuffer.m_usage = RenderBuffer::Usage::CPU_and_GPU;
        pRenderDevice->CreateBuffer( m_vertexBuffer, nullptr );

        if ( !m_vertexBuffer.IsValid() )
        {
            return false;
        }

        // Create vertex layout and input binding
        pRenderDevice->CreateShaderInputBinding( m_vertexShader, vertexLayoutDesc, m_inputBinding );

        // PS
        m_pixelShader = PixelShader( g_byteCode_PS_DebugRendererTriangles, sizeof( g_byteCode_PS_DebugRendererTriangles ), cbuffers );
        pRenderDevice->CreateShader( m_pixelShader );
        cbuffers.clear();

        // Allocate staging memory for vertex data
        m_stagingVertexData.resize( m_vertexBuffer.m_byteSize );

        // Blend state for transparency
        {
            m_blendState.m_srcValue = BlendValue::SourceAlpha;
            m_blendState.m_dstValue = BlendValue::InverseSourceAlpha;
            m_blendState.m_blendOp = BlendOp::Add;
            m_blendState.m_srcAlphaValue = BlendValue::Zero;
            m_blendState.m_dstAlphaValue = BlendValue::One;
            m_blendState.m_blendOpAlpha = BlendOp::Add;
            m_blendState.m_blendEnable = true;

            pRenderDevice->CreateBlendState( m_blendState );

            if ( !m_blendState.IsValid() )
            {
                return false;
            }
        }

        // Set Rasterizer state for drawing
        {
            m_rasterizerState.m_cullMode = CullMode::None;
            m_rasterizerState.m_windingMode = WindingMode::CounterClockwise;
            m_rasterizerState.m_fillMode = FillMode::Solid;

            pRenderDevice->CreateRasterizerState( m_rasterizerState );

            if ( !m_rasterizerState.IsValid() )
            {
                return false;
            }
        }

        // Set up PSO
        m_PSO.m_pVertexShader = &m_vertexShader;
        m_PSO.m_pPixelShader = &m_pixelShader;
        m_PSO.m_pBlendState = &m_blendState;
        m_PSO.m_pRasterizerState = &m_rasterizerState;

        return true;
    }

    void DebugPrimitiveRenderState::Shutdown( RenderDevice* pRenderDevice )
    {
        EE_ASSERT( pRenderDevice != nullptr );

        m_PSO.Clear();

        if ( m_blendState.IsValid() )
        {
            pRenderDevice->DestroyBlendState( m_blendState );
        }

        if ( m_rasterizerState.IsValid() )
        {
            pRenderDevice->DestroyRasterizerState( m_rasterizerState );
        }

        if ( m_vertexBuffer.IsValid() )
        {
            pRenderDevice->DestroyBuffer( m_vertexBuffer );
        }

        pRenderDevice->DestroyShaderInputBinding( m_inputBinding );
        pRenderDevice->DestroyShader( m_vertexShader );
        pRenderDevice->DestroyShader( m_pixelShader );
    }

    void DebugPrimitiveRenderState::SetState( RenderContext const& renderContext, Viewport const& viewport )
    {
        renderContext.SetPipelineState( m_PSO );

        renderContext.SetVertexBuffer( m_vertexBuffer );
        renderContext.SetShaderInputBinding( m_inputBinding );

        struct CB
        {
            Matrix m_viewProjectionMatrix;
            Float4 m_viewport;
        };

        CB cb;
        cb.m_viewProjectionMatrix = viewport.GetViewVolume().GetViewProjectionMatrix();
        cb.m_viewport.m_x = (float) viewport.GetDimensions().m_x;
        cb.m_viewport.m_y = (float) viewport.GetDimensions().m_y;
        renderContext.WriteToBuffer( m_vertexShader.GetConstBuffer( 0 ), &cb, sizeof( CB ) );
    }

    //-------------------------------------------------------------------------

    Float4 const DebugTextRenderState::ClipSpaceTopLeft( -1, 1, 0, 0 );

    //-------------------------------------------------------------------------

    bool DebugTextRenderState::Initialize( RenderDevice* pRenderDevice )
    {
        TVector<RenderBuffer> cbuffers;

        // Viewport info buffer
        RenderBuffer buffer;
        buffer.m_byteSize = sizeof( Float4 );
        buffer.m_byteStride = buffer.m_byteSize;
        buffer.m_usage = RenderBuffer::Usage::CPU_and_GPU;
        buffer.m_type = RenderBuffer::Type::Constant;
        cbuffers.push_back( buffer );

        // Text VS
        VertexLayoutDescriptor vertexLayoutDesc;
        vertexLayoutDesc.m_elementDescriptors.push_back( VertexLayoutDescriptor::ElementDescriptor( DataSemantic::Position, DataFormat::Float_R32G32, 0, 0 ) );
        vertexLayoutDesc.m_elementDescriptors.push_back( VertexLayoutDescriptor::ElementDescriptor( DataSemantic::TexCoord, DataFormat::Float_R32G32, 0, 8 ) );
        vertexLayoutDesc.m_elementDescriptors.push_back( VertexLayoutDescriptor::ElementDescriptor( DataSemantic::Color, DataFormat::UNorm_R8G8B8A8, 0, 16 ) );
        vertexLayoutDesc.CalculateByteSize();

        m_vertexShader = VertexShader( g_byteCode_VS_DebugRendererText, sizeof( g_byteCode_VS_DebugRendererText ), cbuffers, vertexLayoutDesc );
        pRenderDevice->CreateShader( m_vertexShader );
        cbuffers.clear();

        // Create text VB and input binding
        m_vertexBuffer.m_byteStride = sizeof( DebugFontGlyphVertex );
        m_vertexBuffer.m_byteSize = m_vertexBuffer.m_byteStride * MaxGlyphsPerDrawCall * 4; // 4 verts per glyph
        m_vertexBuffer.m_type = RenderBuffer::Type::Vertex;
        m_vertexBuffer.m_usage = RenderBuffer::Usage::CPU_and_GPU;
        pRenderDevice->CreateBuffer( m_vertexBuffer, nullptr );

        if ( !m_vertexBuffer.IsValid() )
        {
            return false;
        }

        // Create vertex layout and input binding
        pRenderDevice->CreateShaderInputBinding( m_vertexShader, vertexLayoutDesc, m_inputBinding );

        // Create text IB
        m_indexBuffer.m_byteStride = sizeof( uint16_t );
        m_indexBuffer.m_byteSize = m_vertexBuffer.m_byteStride * MaxGlyphsPerDrawCall * 6; // 6 indices per glyph
        m_indexBuffer.m_type = RenderBuffer::Type::Index;
        m_indexBuffer.m_usage = RenderBuffer::Usage::CPU_and_GPU;
        pRenderDevice->CreateBuffer( m_indexBuffer, nullptr );

        if ( !m_indexBuffer.IsValid() )
        {
            return false;
        }

        // Text PS
        m_pixelShader = PixelShader( g_byteCode_PS_DebugRendererText, sizeof( g_byteCode_PS_DebugRendererText ), cbuffers );
        pRenderDevice->CreateShader( m_pixelShader );

        // Initialize sampler state
        pRenderDevice->CreateSamplerState( m_samplerState );

        // Blend state for transparency
        {
            m_blendState.m_srcValue = BlendValue::SourceAlpha;
            m_blendState.m_dstValue = BlendValue::InverseSourceAlpha;
            m_blendState.m_blendOp = BlendOp::Add;
            m_blendState.m_srcAlphaValue = BlendValue::Zero;
            m_blendState.m_dstAlphaValue = BlendValue::One;
            m_blendState.m_blendOpAlpha = BlendOp::Add;
            m_blendState.m_blendEnable = true;

            pRenderDevice->CreateBlendState( m_blendState );

            if ( !m_blendState.IsValid() )
            {
                return false;
            }
        }

        // Set Rasterizer state for drawing
        {
            m_rasterizerState.m_cullMode = CullMode::None;
            m_rasterizerState.m_windingMode = WindingMode::CounterClockwise;
            m_rasterizerState.m_fillMode = FillMode::Solid;

            pRenderDevice->CreateRasterizerState( m_rasterizerState );

            if ( !m_rasterizerState.IsValid() )
            {
                return false;
            }
        }

        // Initialize font atlas and create atlas texture
        //-------------------------------------------------------------------------

        DebugTextFontAtlas::FontDesc fonts[] =
        {
            { Fonts::Proggy::Tiny::GetData(), 10 },
            { Fonts::Proggy::Clean::GetData(), 13 },
        };

        if ( !m_fontAtlas.Generate( fonts, 2 ) )
        {
            return false;
        }

        Int2 const fontAtlasDimensions = m_fontAtlas.GetDimensions();
        int32_t const fontAtlasDimensionsSq = fontAtlasDimensions.m_x * fontAtlasDimensions.m_y;

        m_nonZeroAlphaTexCoords = Float2( 0, 0 );

        // Convert from 8 bit to 32 bit pixels
        TVector<uint32_t> fontdata( fontAtlasDimensionsSq );
        uint8_t const* pSrc = m_fontAtlas.GetAtlasData();
        for ( int i = 0; i < fontAtlasDimensionsSq; i++ )
        {
            if ( m_nonZeroAlphaTexCoords == Float2::Zero && *pSrc == 0xFF )
            {
                float const u = float( i % fontAtlasDimensions.m_x );
                float const v = float( i / fontAtlasDimensions.m_x );
                m_nonZeroAlphaTexCoords = Float2( u, v );
            }

            fontdata[i] = ( (uint32_t) ( *pSrc++ ) << 24 ) | 0x00FFFFFF;
        }

        m_fontAtlasTexture = Texture( fontAtlasDimensions );
        pRenderDevice->CreateDataTexture( m_fontAtlasTexture, TextureFormat::Raw, (uint8_t*) fontdata.data(), fontAtlasDimensions.m_x * fontAtlasDimensions.m_y * 4 );

        // Set up PSO
        m_PSO.m_pVertexShader = &m_vertexShader;
        m_PSO.m_pPixelShader = &m_pixelShader;
        m_PSO.m_pBlendState = &m_blendState;
        m_PSO.m_pRasterizerState = &m_rasterizerState;

        return true;
    }

    void DebugTextRenderState::Shutdown( RenderDevice* pRenderDevice )
    {
        EE_ASSERT( pRenderDevice != nullptr );

        m_PSO.Clear();

        if ( m_blendState.IsValid() )
        {
            pRenderDevice->DestroyBlendState( m_blendState );
        }

        if ( m_rasterizerState.IsValid() )
        {
            pRenderDevice->DestroyRasterizerState( m_rasterizerState );
        }

        if ( m_vertexBuffer.IsValid() )
        {
            pRenderDevice->DestroyBuffer( m_vertexBuffer );
        }

        if ( m_indexBuffer.IsValid() )
        {
            pRenderDevice->DestroyBuffer( m_indexBuffer );
        }

        if ( m_fontAtlasTexture.IsValid() )
        {
            pRenderDevice->DestroyTexture( m_fontAtlasTexture );
        }

        if ( m_samplerState.IsValid() )
        {
            pRenderDevice->DestroySamplerState( m_samplerState );
        }

        pRenderDevice->DestroyShaderInputBinding( m_inputBinding );
        pRenderDevice->DestroyShader( m_vertexShader );
        pRenderDevice->DestroyShader( m_pixelShader );
    }

    void DebugTextRenderState::SetState( RenderContext const& renderContext, Viewport const& viewport )
    {
        renderContext.SetPipelineState( m_PSO );

        renderContext.SetShaderInputBinding( m_inputBinding );
        renderContext.SetSampler( PipelineStage::Pixel, 0, m_samplerState );
        renderContext.SetShaderResource( PipelineStage::Pixel, 0, m_fontAtlasTexture.GetShaderResourceView() );
        renderContext.SetPrimitiveTopology( Topology::TriangleList );
        renderContext.SetVertexBuffer( m_vertexBuffer );
        renderContext.SetIndexBuffer( m_indexBuffer );

        // Set viewport info
        Float4 const m_viewportInfo( viewport.GetDimensions() );
        renderContext.WriteToBuffer( m_vertexShader.GetConstBuffer( 0 ), &m_viewportInfo, sizeof( Float4 ) );
    }

    //-------------------------------------------------------------------------
    // Fonts and Glyphs
    //-------------------------------------------------------------------------

    bool DebugTextFontAtlas::Generate( FontDesc* pFonts, int32_t const numFonts )
    {
        static IntRange const glyphRange( 0x20, 0xFF );

        // Allocate temporary atlas data
        //-------------------------------------------------------------------------

        Int2 const fontAtlasDimensions = GetDimensions();
        int32_t const fontAtlasDimensionsSq = fontAtlasDimensions.m_x * fontAtlasDimensions.m_y;
        m_atlasData.resize( fontAtlasDimensionsSq, 0 );

        // Render debug fonts to atlas
        //-------------------------------------------------------------------------

        stbtt_pack_context context;
        stbtt_PackSetOversampling( &context, 2, 2 );

        if ( !stbtt_PackBegin( &context, m_atlasData.data(), fontAtlasDimensions.m_x, fontAtlasDimensions.m_y, 0, 0, nullptr ) )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        TVector<stbtt_packedchar> charInfo;
        Blob fontData;

        for ( auto i = 0; i < numFonts; i++ )
        {
            // Decompress font data
            //-------------------------------------------------------------------------

            Fonts::GetDecompressedFontData( pFonts[i].m_compressedFontData, fontData );

            stbtt_fontinfo stbFontInfo;
            stbtt_InitFont( &stbFontInfo, fontData.data(), 0 );
            float const fontScale = stbtt_ScaleForPixelHeight( &stbFontInfo, 10 );

            int32_t unscaledAscent, unscaledDescent, unscaledLineGap;
            stbtt_GetFontVMetrics( &stbFontInfo, &unscaledAscent, &unscaledDescent, &unscaledLineGap );

            // Create font info
            //-------------------------------------------------------------------------

            m_fonts.push_back( FontInfo() );
            FontInfo& fontInfo = m_fonts.back();
            fontInfo.m_ascent = unscaledAscent * fontScale;
            fontInfo.m_descent = -unscaledDescent * fontScale;
            fontInfo.m_lineGap = unscaledLineGap * fontScale;

            // Pack font into the atlas
            //-------------------------------------------------------------------------

            charInfo.resize( glyphRange.GetLength() );
            if ( !stbtt_PackFontRange( &context, fontData.data(), 0, pFonts[i].m_fontSize, glyphRange.m_begin, glyphRange.GetLength(), charInfo.data() ) )
            {
                return false;
            }

            // Create Glyph lookup table
            //-------------------------------------------------------------------------

            for ( auto g = glyphRange.m_begin; g < glyphRange.m_end; g++ )
            {
                int32_t charIdx = g - glyphRange.m_begin;
                stbtt_aligned_quad q;
                Float2 pos( 0.0f );
                stbtt_GetPackedQuad( charInfo.data(), fontAtlasDimensions.m_x, fontAtlasDimensions.m_y, charIdx, &pos.m_x, &pos.m_y, &q, 0 );

                // Glyphs positions are stored relative of their baseline positions (i.e. bottom left)
                DebugFontGlyph glyph;
                glyph.m_positionTL = Float2( q.x0, q.y0 );
                glyph.m_positionBR = Float2( q.x1, q.y1 );
                glyph.m_texCoordsTL = Float2( q.s0, q.t0 );
                glyph.m_texCoordsBR = Float2( q.s1, q.t1 );
                glyph.m_advanceX = pos.m_x + 0.5f;
                fontInfo.m_glyphs.emplace_back( std::move( glyph ) );
            }
        }

        stbtt_PackEnd( &context );
        return true;
    }

    void DebugTextFontAtlas::GetGlyphsForString( uint32_t fontIdx, TInlineString<24> const& str, TInlineVector<int32_t, 100>& outGlyphIndices ) const
    {
        EE_ASSERT( fontIdx < m_fonts.size() );
        auto const& fontInfo = m_fonts[fontIdx];

        IntRange const glyphRange = fontInfo.GetValidGlyphRange();
        int32_t const spaceIdx = ' ' - glyphRange.m_begin;

        outGlyphIndices.reserve( (int32_t) ( str.size() * 1.5f ) );

        for ( char ch : str )
        {
            if ( ch == '\t' )
            {
                outGlyphIndices.push_back( spaceIdx );
                outGlyphIndices.push_back( spaceIdx );
                outGlyphIndices.push_back( spaceIdx );
                outGlyphIndices.push_back( spaceIdx );
            }
            else if ( ch == '\n' )
            {
                outGlyphIndices.push_back( '\n' );
            }
            else if ( glyphRange.ContainsInclusive( ch ) )
            {
                outGlyphIndices.push_back( ch - glyphRange.m_begin );
            }
        }
    }

    void DebugTextFontAtlas::GetGlyphsForString( uint32_t fontIdx, char const* pStr, TInlineVector<int32_t, 100>& outGlyphIndices ) const
    {
        EE_ASSERT( fontIdx < m_fonts.size() );
        auto const& fontInfo = m_fonts[fontIdx];

        IntRange const glyphRange = fontInfo.GetValidGlyphRange();
        int32_t const spaceIdx = ' ' - glyphRange.m_begin;

        int32_t const strLen = (int32_t) strlen( pStr );
        outGlyphIndices.reserve( Math::CeilingToInt( strLen * 1.5f ) );

        for ( int32_t i = 0; i < strLen; i++ )
        {
            if ( pStr[i] == '\t' )
            {
                outGlyphIndices.push_back( spaceIdx );
                outGlyphIndices.push_back( spaceIdx );
                outGlyphIndices.push_back( spaceIdx );
                outGlyphIndices.push_back( spaceIdx );
            }
            else if ( pStr[i] == '\n' )
            {
                outGlyphIndices.push_back( '\n' );
            }
            else if ( glyphRange.ContainsInclusive( pStr[i] ) )
            {
                outGlyphIndices.push_back( pStr[i] - glyphRange.m_begin );
            }
        }
    }

    Int2 DebugTextFontAtlas::GetTextExtents( uint32_t fontIdx, char const* pText ) const
    {
        EE_ASSERT( pText != nullptr );

        EE_ASSERT( fontIdx < m_fonts.size() );
        auto const& fontInfo = m_fonts[fontIdx];

        auto const glyphRange = fontInfo.GetValidGlyphRange();
        auto const spaceGlyphIdx = fontInfo.GetGlyphIndex( ' ' );
        auto const numChars = strlen( pText );

        float const lineHeight = fontInfo.m_ascent + fontInfo.m_descent + fontInfo.m_lineGap;
        float currentLineWidth = 0;
        float maxWidth = 0;
        uint32_t numLines = 1;

        for ( auto i = 0; i < numChars; i++ )
        {
            // Increment Y offset
            if ( pText[i] == '\n' )
            {
                numLines++;
                maxWidth = Math::Max( currentLineWidth, maxWidth );
                currentLineWidth = 0;
            }
            else if ( pText[i] == '\t' )
            {
                currentLineWidth += fontInfo.m_glyphs[spaceGlyphIdx].m_advanceX * 4;
            }
            else // Regular characters increment x offset
            {
                if ( glyphRange.ContainsInclusive( pText[i] ) )
                {
                    auto const glyphIdx = pText[i] - glyphRange.m_begin;
                    auto const& glyph = fontInfo.m_glyphs[glyphIdx];
                    currentLineWidth += glyph.m_advanceX;
                }
            }
        }

        maxWidth = Math::Max( currentLineWidth, maxWidth );
        Int2 const extents( (int32_t) Math::Floor( maxWidth - 0.5f ) - 1, (int32_t) Math::Floor( ( lineHeight * numLines ) - 0.5f ) );
        return extents;
    }

    uint32_t DebugTextFontAtlas::WriteGlyphsToBuffer( DebugFontGlyphVertex* pVertexBuffer, uint16_t indexStartOffset, uint16_t* pIndexBuffer, uint32_t fontIdx, TInlineVector<int32_t, 100> const& glyphIndices, Float2 const& textPosTopLeft, Float4 const& color ) const
    {
        EE_ASSERT( fontIdx < m_fonts.size() );
        auto const& fontInfo = m_fonts[fontIdx];

        float const descent = fontInfo.GetDescent();
        float const lineHeight = fontInfo.GetAscent() + fontInfo.GetDescent() + fontInfo.GetLineGap();

        // Draw characters
        //-------------------------------------------------------------------------

        uint32_t numGlyphsDrawn = 0;
        Float2 textDrawPos = textPosTopLeft;
        textDrawPos.m_y += ( lineHeight - descent );

        //-------------------------------------------------------------------------

        size_t const numGlyphs = glyphIndices.size();
        for ( auto i = 0; i < numGlyphs; i++ )
        {
            if ( glyphIndices[i] == '\n' )
            {
                textDrawPos.m_x = textPosTopLeft.m_x;
                textDrawPos.m_y += lineHeight;
            }
            else
            {
                DebugFontGlyph const& glyph = fontInfo.GetGlyph( glyphIndices[i] );

                Float2 const bl( Math::Floor( textDrawPos.m_x + glyph.m_positionTL.m_x ), Math::Floor( textDrawPos.m_y + glyph.m_positionBR.m_y ) );
                Float2 const tl( Math::Floor( textDrawPos.m_x + glyph.m_positionTL.m_x ), Math::Floor( textDrawPos.m_y + glyph.m_positionTL.m_y ) );
                Float2 const tr( Math::Floor( textDrawPos.m_x + glyph.m_positionBR.m_x ), Math::Floor( textDrawPos.m_y + glyph.m_positionTL.m_y ) );
                Float2 const br( Math::Floor( textDrawPos.m_x + glyph.m_positionBR.m_x ), Math::Floor( textDrawPos.m_y + glyph.m_positionBR.m_y ) );

                uint32_t const vertexColor = ( uint8_t( color[3] * 255 ) << 24 ) | ( uint8_t( color[2] * 255 ) << 16 ) | ( uint8_t( color[1] * 255 ) << 8 ) | uint8_t( color[0] * 255 );

                // 0 = BL, 1 = TL, 2 = TR, 3 = BR
                pVertexBuffer[0] = DebugFontGlyphVertex{ bl, Float2( glyph.m_texCoordsTL.m_x, glyph.m_texCoordsBR.m_y ), vertexColor };
                pVertexBuffer[1] = DebugFontGlyphVertex{ tl, Float2( glyph.m_texCoordsTL.m_x, glyph.m_texCoordsTL.m_y ), vertexColor };
                pVertexBuffer[2] = DebugFontGlyphVertex{ tr, Float2( glyph.m_texCoordsBR.m_x, glyph.m_texCoordsTL.m_y ), vertexColor };
                pVertexBuffer[3] = DebugFontGlyphVertex{ br, Float2( glyph.m_texCoordsBR.m_x, glyph.m_texCoordsBR.m_y ), vertexColor };

                pIndexBuffer[0] = indexStartOffset + 3;
                pIndexBuffer[1] = indexStartOffset + 1;
                pIndexBuffer[2] = indexStartOffset + 0;
                pIndexBuffer[3] = indexStartOffset + 3;
                pIndexBuffer[4] = indexStartOffset + 2;
                pIndexBuffer[5] = indexStartOffset + 1;

                // Offset buffer ptrs
                //-------------------------------------------------------------------------

                pVertexBuffer += 4;
                indexStartOffset += 4;
                pIndexBuffer += 6;

                // Update glyph draw state
                //-------------------------------------------------------------------------

                textDrawPos.m_x += glyph.m_advanceX;
                numGlyphsDrawn++;
            }
        }

        return numGlyphsDrawn;
    }

    void DebugTextFontAtlas::WriteCustomGlyphToBuffer( DebugFontGlyphVertex* pVertexBuffer, uint16_t indexStartOffset, uint16_t* pIndexBuffer, uint32_t fontIdx, int32_t firstGlyphIdx, Float2 const& texCoords, Float2 const& textPosTopLeft, Int2 const& textExtents, int32_t pixelPadding, Float4 const& color ) const
    {
        EE_ASSERT( fontIdx < m_fonts.size() );
        auto const& fontInfo = m_fonts[fontIdx];

        float const descent = fontInfo.GetDescent();
        float const ascent = fontInfo.GetAscent();
        float const lineHeight = fontInfo.GetAscent() + fontInfo.GetDescent() + fontInfo.GetLineGap();

        Float2 textDrawPos = textPosTopLeft;
        textDrawPos.m_y += ( lineHeight - descent );

        DebugFontGlyph const& glyph = fontInfo.GetGlyph( firstGlyphIdx );

        Float2 const tl( Math::Floor( textDrawPos.m_x + glyph.m_positionTL.m_x - pixelPadding ), Math::Floor( textDrawPos.m_y + glyph.m_positionTL.m_y - pixelPadding ) );
        Float2 const tr( Math::Floor( textDrawPos.m_x + textExtents.m_x + pixelPadding ), Math::Floor( textDrawPos.m_y + glyph.m_positionTL.m_y - pixelPadding ) );
        Float2 const bl( Math::Floor( textDrawPos.m_x + glyph.m_positionTL.m_x - pixelPadding ), Math::Floor( textDrawPos.m_y - ascent + textExtents.m_y + pixelPadding ) );
        Float2 const br( Math::Floor( textDrawPos.m_x + textExtents.m_x + pixelPadding ), Math::Floor( textDrawPos.m_y - ascent + textExtents.m_y + pixelPadding ) );

        uint32_t const vertexColor = ( uint8_t( color[3] * 255 ) << 24 ) | ( uint8_t( color[2] * 255 ) << 16 ) | ( uint8_t( color[1] * 255 ) << 8 ) | uint8_t( color[0] * 255 );

        // 0 = BL, 1 = TL, 2 = TR, 3 = BR
        pVertexBuffer[0] = DebugFontGlyphVertex{ bl, texCoords, vertexColor };
        pVertexBuffer[1] = DebugFontGlyphVertex{ tl, texCoords, vertexColor };
        pVertexBuffer[2] = DebugFontGlyphVertex{ tr, texCoords, vertexColor };
        pVertexBuffer[3] = DebugFontGlyphVertex{ br, texCoords, vertexColor };

        pIndexBuffer[0] = indexStartOffset + 3;
        pIndexBuffer[1] = indexStartOffset + 1;
        pIndexBuffer[2] = indexStartOffset + 0;
        pIndexBuffer[3] = indexStartOffset + 3;
        pIndexBuffer[4] = indexStartOffset + 2;
        pIndexBuffer[5] = indexStartOffset + 1;
    }
}
#endif