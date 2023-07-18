#include "RenderContext_DX11.h"
#include "Base/Types/Color.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    Float4 const RenderContext::s_defaultClearColor = Color( 96, 96, 96 ).ToFloat4();

    ID3D11DepthStencilState* RenderContext::s_pDepthTestingOn = nullptr;
    ID3D11DepthStencilState* RenderContext::s_pDepthTestingOff = nullptr;
    ID3D11DepthStencilState* RenderContext::s_pDepthTestingReadOnly = nullptr;

    //-------------------------------------------------------------------------

    RenderContext::RenderContext( ID3D11DeviceContext* pDeviceContext )
        : m_pDeviceContext( pDeviceContext )
    {
        EE_ASSERT( m_pDeviceContext != nullptr );
    }

    //-------------------------------------------------------------------------

    void RenderContext::SetPipelineState( PipelineState const& pipelineState ) const
    {
        // VS
        //-------------------------------------------------------------------------

        if ( pipelineState.m_pVertexShader != nullptr && pipelineState.m_pVertexShader->IsValid() )
        {
            m_pDeviceContext->VSSetShader( (ID3D11VertexShader*) pipelineState.m_pVertexShader->GetShaderHandle().m_pData, nullptr, 0 );

            auto const numCbuffers = pipelineState.m_pVertexShader->GetNumConstBuffers();
            for ( auto i = 0u; i < numCbuffers; i++ )
            {
                m_pDeviceContext->VSSetConstantBuffers( (uint32_t) i, 1, (ID3D11Buffer**) &pipelineState.m_pVertexShader->GetConstBuffer( i ).GetResourceHandle().m_pData );
            }
        }
        else
        {
            m_pDeviceContext->VSSetShader( nullptr, nullptr, 0 );
        }

        m_pDeviceContext->IASetInputLayout( nullptr );

        // GS
        //-------------------------------------------------------------------------

        if ( pipelineState.m_pGeometryShader != nullptr && pipelineState.m_pGeometryShader->IsValid() )
        {
            m_pDeviceContext->GSSetShader( (ID3D11GeometryShader*) pipelineState.m_pGeometryShader->GetShaderHandle().m_pData, nullptr, 0 );

            auto const numCbuffers = pipelineState.m_pGeometryShader->GetNumConstBuffers();
            for ( auto i = 0u; i < numCbuffers; i++ )
            {
                m_pDeviceContext->GSSetConstantBuffers( (uint32_t) i, 1, (ID3D11Buffer**) &pipelineState.m_pGeometryShader->GetConstBuffer( i ).GetResourceHandle().m_pData );
            }
        }
        else
        {
            m_pDeviceContext->GSSetShader( nullptr, nullptr, 0 );
        }

        // HS
        //-------------------------------------------------------------------------

        if ( pipelineState.m_pHullShader != nullptr && pipelineState.m_pHullShader->IsValid() )
        {
            m_pDeviceContext->HSSetShader( (ID3D11HullShader*) pipelineState.m_pHullShader->GetShaderHandle().m_pData, nullptr, 0 );

            auto const numCbuffers = pipelineState.m_pHullShader->GetNumConstBuffers();
            for ( auto i = 0u; i < numCbuffers; i++ )
            {
                m_pDeviceContext->HSSetConstantBuffers( (uint32_t) i, 1, (ID3D11Buffer**) &pipelineState.m_pHullShader->GetConstBuffer( i ).GetResourceHandle().m_pData );
            }
        }
        else
        {
            m_pDeviceContext->HSSetShader( nullptr, nullptr, 0 );
        }

        // CS
        //-------------------------------------------------------------------------

        if ( pipelineState.m_pComputeShader != nullptr && pipelineState.m_pComputeShader->IsValid() )
        {
            m_pDeviceContext->CSSetShader( (ID3D11ComputeShader*) pipelineState.m_pComputeShader->GetShaderHandle().m_pData, nullptr, 0 );

            auto const numCbuffers = pipelineState.m_pComputeShader->GetNumConstBuffers();
            for ( auto i = 0u; i < numCbuffers; i++ )
            {
                m_pDeviceContext->CSSetConstantBuffers( (uint32_t) i, 1, (ID3D11Buffer**) &pipelineState.m_pComputeShader->GetConstBuffer( i ).GetResourceHandle().m_pData );
            }
        }
        else
        {
            m_pDeviceContext->CSSetShader( nullptr, nullptr, 0 );
        }

        // PS
        //-------------------------------------------------------------------------

        if ( pipelineState.m_pPixelShader != nullptr && pipelineState.m_pPixelShader->IsValid() )
        {
            m_pDeviceContext->PSSetShader( (ID3D11PixelShader*) pipelineState.m_pPixelShader->GetShaderHandle().m_pData, nullptr, 0 );

            auto const numCbuffers = pipelineState.m_pPixelShader->GetNumConstBuffers();
            for ( auto i = 0u; i < numCbuffers; i++ )
            {
                m_pDeviceContext->PSSetConstantBuffers( (uint32_t) i, 1, (ID3D11Buffer**) &pipelineState.m_pPixelShader->GetConstBuffer( i ).GetResourceHandle().m_pData );
            }
        }
        else
        {
            m_pDeviceContext->PSSetShader( nullptr, nullptr, 0 );
        }

        // RS
        //-------------------------------------------------------------------------

        if ( pipelineState.m_pBlendState != nullptr && pipelineState.m_pBlendState->IsValid() )
        {
            m_pDeviceContext->OMSetBlendState( (ID3D11BlendState*) pipelineState.m_pBlendState->GetResourceHandle().m_pData, nullptr, 0xffffffff );
        }
        else
        {
            m_pDeviceContext->OMSetBlendState( nullptr, nullptr, 0xffffffff );
        }

        if ( pipelineState.m_pRasterizerState != nullptr && pipelineState.m_pRasterizerState->IsValid() )
        {
            m_pDeviceContext->RSSetState( (ID3D11RasterizerState*) pipelineState.m_pRasterizerState->GetResourceHandle().m_pData );
            SetRasterizerScissorRectangles( nullptr );
        }
    }

    //-------------------------------------------------------------------------
    void RenderContext::SetShaderInputBinding( ShaderInputBindingHandle const& inputBinding ) const
    {
        EE_ASSERT( IsValid() );
        m_pDeviceContext->IASetInputLayout( inputBinding.IsValid() ? (ID3D11InputLayout*) inputBinding.m_pData : nullptr );
    }

    void RenderContext::SetShaderResource( PipelineStage stage, uint32_t slot, ViewSRVHandle const& shaderResourceView ) const
    {
        EE_ASSERT( IsValid() );

        auto pSRV = shaderResourceView.IsValid() ? (ID3D11ShaderResourceView*) shaderResourceView.m_pData : nullptr;

        switch ( stage )
        {
            case PipelineStage::Vertex:
            m_pDeviceContext->VSSetShaderResources( slot, 1, (ID3D11ShaderResourceView* const*) &pSRV );
            break;

            case PipelineStage::Geometry:
            m_pDeviceContext->GSSetShaderResources( slot, 1, (ID3D11ShaderResourceView* const*) &pSRV );
            break;

            case PipelineStage::Pixel:
            m_pDeviceContext->PSSetShaderResources( slot, 1, (ID3D11ShaderResourceView* const*) &pSRV );
            break;

            case PipelineStage::Hull:
            m_pDeviceContext->HSSetShaderResources( slot, 1, (ID3D11ShaderResourceView* const*) &pSRV );
            break;

            case PipelineStage::Compute:
            m_pDeviceContext->CSSetShaderResources( slot, 1, (ID3D11ShaderResourceView* const*) &pSRV );
            break;

            default:
            EE_HALT();
            break;
        }
    }

    void RenderContext::ClearShaderResource( PipelineStage stage, uint32_t slot ) const
    {
        EE_ASSERT( IsValid() );

        ID3D11ShaderResourceView* noSRV[] = { nullptr };

        switch ( stage )
        {
            case PipelineStage::Vertex:
            m_pDeviceContext->VSSetShaderResources( slot, 1, noSRV );
            break;

            case PipelineStage::Geometry:
            m_pDeviceContext->GSSetShaderResources( slot, 1, noSRV );
            break;

            case PipelineStage::Pixel:
            m_pDeviceContext->PSSetShaderResources( slot, 1, noSRV );
            break;

            case PipelineStage::Hull:
            m_pDeviceContext->HSSetShaderResources( slot, 1, noSRV );
            break;

            case PipelineStage::Compute:
            m_pDeviceContext->CSSetShaderResources( slot, 1, noSRV );
            break;

            default:
            EE_HALT();
            break;
        }
    }

    void RenderContext::SetUnorderedAccess( PipelineStage stage, uint32_t slot, ViewUAVHandle const& unorderedAccessView ) const
    {
        EE_ASSERT( IsValid() && unorderedAccessView.IsValid() );

        auto pUAV = (ID3D11UnorderedAccessView*) unorderedAccessView.m_pData;

        switch ( stage )
        {
            case PipelineStage::Compute:
            m_pDeviceContext->CSSetUnorderedAccessViews( slot, 1, (ID3D11UnorderedAccessView* const*) &pUAV, nullptr );
            break;

            default:
            EE_HALT();
            break;
        }
    }

    void RenderContext::ClearUnorderedAccess( PipelineStage stage, uint32_t slot ) const
    {
        EE_ASSERT( IsValid() );

        ID3D11UnorderedAccessView* noUAV[] = { nullptr };

        switch ( stage )
        {
            case PipelineStage::Compute:
            m_pDeviceContext->CSSetUnorderedAccessViews( slot, 1, noUAV, nullptr );
            break;

            default:
            EE_HALT();
            break;
        }
    }

    void RenderContext::SetSampler( PipelineStage stage, uint32_t slot, SamplerState const& state ) const
    {
        EE_ASSERT( IsValid() && state.IsValid() );

        switch ( stage )
        {
            case PipelineStage::Vertex:
            m_pDeviceContext->VSSetSamplers( slot, 1, (ID3D11SamplerState* const*) &state.GetResourceHandle().m_pData );
            break;

            case PipelineStage::Geometry:
            m_pDeviceContext->GSSetSamplers( slot, 1, (ID3D11SamplerState* const*) &state.GetResourceHandle().m_pData );
            break;

            case PipelineStage::Pixel:
            m_pDeviceContext->PSSetSamplers( slot, 1, (ID3D11SamplerState* const*) &state.GetResourceHandle().m_pData );
            break;

            case PipelineStage::Hull:
            m_pDeviceContext->HSSetSamplers( slot, 1, (ID3D11SamplerState* const*) &state.GetResourceHandle().m_pData );
            break;

            case PipelineStage::Compute:
            m_pDeviceContext->CSSetSamplers( slot, 1, (ID3D11SamplerState* const*) &state.GetResourceHandle().m_pData );
            break;

            default:
            EE_HALT();
            break;
        }
    }

    //-------------------------------------------------------------------------

    void* RenderContext::MapBuffer( RenderBuffer const& buffer ) const
    {
        // Check that we have write access to this buffer and that the data to write is within the buffer size
        EE_ASSERT( IsValid() );
        EE_ASSERT( buffer.IsValid() );
        EE_ASSERT( buffer.m_usage == RenderBuffer::Usage::CPU_and_GPU ); // Buffer does not allow CPU access

        D3D11_MAPPED_SUBRESOURCE mappedData;
        EE::Memory::MemsetZero( &mappedData, sizeof( D3D11_MAPPED_SUBRESOURCE ) );

        auto result = m_pDeviceContext->Map( (ID3D11Buffer*) buffer.GetResourceHandle().m_pData, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData );
        EE_ASSERT( SUCCEEDED( result ) );
        return (uint8_t*) mappedData.pData;
    }

    void RenderContext::UnmapBuffer( RenderBuffer const& buffer ) const
    {
        // Check that we have write access to this buffer and that the data to write is within the buffer size
        EE_ASSERT( IsValid() && buffer.IsValid() && buffer.m_usage == RenderBuffer::Usage::CPU_and_GPU );
        m_pDeviceContext->Unmap( (ID3D11Buffer*) buffer.GetResourceHandle().m_pData, 0 );
    }

    void RenderContext::WriteToBuffer( RenderBuffer const& buffer, void const* pData, size_t const dataSize ) const
    {
        EE_ASSERT( pData != nullptr && buffer.m_byteSize >= dataSize );
        void* pMappedData = MapBuffer( buffer );
        memcpy( pMappedData, pData, dataSize );
        UnmapBuffer( buffer );
    }

    void RenderContext::SetVertexBuffer( RenderBuffer const& buffer, uint32_t offset ) const
    {
        EE_ASSERT( IsValid() && buffer.IsValid() && buffer.m_type == RenderBuffer::Type::Vertex );
        m_pDeviceContext->IASetVertexBuffers( 0, 1, (ID3D11Buffer**) &buffer.GetResourceHandle().m_pData, &buffer.m_byteStride, &offset );
    }

    void RenderContext::SetIndexBuffer( RenderBuffer const& buffer, uint32_t offset ) const
    {
        EE_ASSERT( IsValid() && buffer.IsValid() && buffer.m_type == RenderBuffer::Type::Index );
        m_pDeviceContext->IASetIndexBuffer( (ID3D11Buffer*) buffer.GetResourceHandle().m_pData, buffer.m_byteStride == 4 ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT, offset );
    }

    //-------------------------------------------------------------------------

    void RenderContext::SetViewport( Float2 dimensions, Float2 topLeft, Float2 rangeZ ) const
    {
        D3D11_VIEWPORT dxViewport;
        dxViewport.Width = dimensions.m_x;
        dxViewport.Height = dimensions.m_y;
        dxViewport.TopLeftX = topLeft.m_x;
        dxViewport.TopLeftY = topLeft.m_y;
        dxViewport.MinDepth = rangeZ.m_x;
        dxViewport.MaxDepth = rangeZ.m_y;
        m_pDeviceContext->RSSetViewports( 1, &dxViewport );
    }

    void RenderContext::SetDepthTestMode( DepthTestMode mode ) const
    {
        switch ( mode )
        {
            case DepthTestMode::On:
            m_pDeviceContext->OMSetDepthStencilState( s_pDepthTestingOn, 0 );
            break;

            case DepthTestMode::Off:
            m_pDeviceContext->OMSetDepthStencilState( s_pDepthTestingOff, 0 );
            break;

            case DepthTestMode::ReadOnly:
            m_pDeviceContext->OMSetDepthStencilState( s_pDepthTestingReadOnly, 0 );
            break;
        }
    }

    void RenderContext::SetRasterizerScissorRectangles( ScissorRect const* pScissorRects, uint32_t numRects ) const
    {
        EE_ASSERT( IsValid() );

        if ( numRects != 0 )
        {
            EE_ASSERT( pScissorRects != nullptr );
            m_pDeviceContext->RSSetScissorRects( numRects, (D3D11_RECT*) pScissorRects );
        }
        else
        {
            m_pDeviceContext->RSSetScissorRects( 0, nullptr );
        }
    }

    void RenderContext::SetBlendState( BlendState const& blendState ) const
    {
        EE_ASSERT( IsValid() );

        float const blendFactors[4] = { 0.f, 0.f, 0.f, 0.f };

        if ( blendState.IsValid() )
        {
            m_pDeviceContext->OMSetBlendState( (ID3D11BlendState*) blendState.GetResourceHandle().m_pData, blendFactors, 0xffffffff );
        }
        else
        {
            m_pDeviceContext->OMSetBlendState( nullptr, blendFactors, 0xffffffff );
        }
    }

    //-------------------------------------------------------------------------

    void RenderContext::SetRenderTarget( RenderTarget const& renderTarget ) const
    {
        EE_ASSERT( IsValid() && renderTarget.IsValid() );

        // Get render targets
        int32_t const numViews = renderTarget.HasPickingRT() ? 2 : 1;
        ID3D11RenderTargetView* renderTargetViews[2] =
        {
            (ID3D11RenderTargetView*) renderTarget.GetRenderTargetHandle().m_pData,
            renderTarget.HasPickingRT() ? (ID3D11RenderTargetView*) renderTarget.GetPickingRenderTargetHandle().m_pData : nullptr,
        };

        // Get depth stencil
        ID3D11DepthStencilView* pDepthStencilView = nullptr;
        if ( renderTarget.HasDepthStencil() )
        {
            pDepthStencilView = (ID3D11DepthStencilView*) renderTarget.GetDepthStencilHandle().m_pData;
        }

        //-------------------------------------------------------------------------

        m_pDeviceContext->OMSetRenderTargets( numViews, renderTargetViews, pDepthStencilView );
    }

    void RenderContext::SetRenderTarget( ViewDSHandle const& dsView ) const
    {
        EE_ASSERT( IsValid() && dsView.IsValid() );
        m_pDeviceContext->OMSetRenderTargets( 0, nullptr, (ID3D11DepthStencilView*) dsView.m_pData );
    }

    void RenderContext::SetRenderTarget( nullptr_t ) const
    {
        ID3D11RenderTargetView* nullViews[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = { 0 };
        m_pDeviceContext->OMSetRenderTargets( D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, nullViews, nullptr );
    }

    void RenderContext::ClearDepthStencilView( ViewDSHandle const& dsView, float depth, uint8_t stencil ) const
    {
        EE_ASSERT( IsValid() && dsView.IsValid() );
        m_pDeviceContext->ClearDepthStencilView( (ID3D11DepthStencilView*) dsView.m_pData, D3D10_CLEAR_DEPTH, depth, stencil );
    }

    void RenderContext::ClearRenderTargetViews( RenderTarget const& renderTarget ) const
    {
        EE_ASSERT( IsValid() && renderTarget.IsValid() );

        ViewRTHandle const& rtHandle = renderTarget.GetRenderTargetHandle();
        m_pDeviceContext->ClearRenderTargetView( (ID3D11RenderTargetView*) rtHandle.m_pData, &s_defaultClearColor.m_x );

        ViewDSHandle const& dsHandle = renderTarget.GetDepthStencilHandle();
        m_pDeviceContext->ClearDepthStencilView( (ID3D11DepthStencilView*) dsHandle.m_pData, D3D10_CLEAR_DEPTH, 1.0f, 0 );

        //-------------------------------------------------------------------------

        if ( renderTarget.HasPickingRT() )
        {
            ViewRTHandle const& pickingHandle = renderTarget.GetPickingRenderTargetHandle();
            m_pDeviceContext->ClearRenderTargetView( (ID3D11RenderTargetView*) pickingHandle.m_pData, &Float4::Zero.m_x );
        }
    }

    //-------------------------------------------------------------------------

    void RenderContext::SetPrimitiveTopology( Topology topology ) const
    {
        EE_ASSERT( IsValid() );
        D3D11_PRIMITIVE_TOPOLOGY d3dTopology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;

        switch ( topology )
        {
            case Topology::PointList:
            d3dTopology = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
            break;

            case Topology::LineList:
            d3dTopology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
            break;

            case Topology::LineStrip:
            d3dTopology = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
            break;

            case Topology::TriangleList:
            d3dTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
            break;

            case Topology::TriangleStrip:
            d3dTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
            break;

            default:
            EE_UNREACHABLE_CODE();
            break;
        }

        m_pDeviceContext->IASetPrimitiveTopology( d3dTopology );
    }

    void RenderContext::Draw( uint32_t vertexCount, uint32_t vertexStartIndex ) const
    {
        EE_ASSERT( IsValid() );
        m_pDeviceContext->Draw( vertexCount, vertexStartIndex );
    }

    void RenderContext::DrawIndexed( uint32_t vertexCount, uint32_t indexStartIndex, uint32_t vertexStartIndex ) const
    {
        EE_ASSERT( IsValid() );
        m_pDeviceContext->DrawIndexed( vertexCount, indexStartIndex, vertexStartIndex );
    }

    void RenderContext::Dispatch( uint32_t numGroupsX, uint32_t numGroupsY, uint32_t numGroupsZ ) const
    {
        EE_ASSERT( IsValid() );
        m_pDeviceContext->Dispatch( numGroupsX, numGroupsY, numGroupsZ );
    }

    void RenderContext::Present( RenderWindow& window ) const
    {
        auto pSwapChain = reinterpret_cast<IDXGISwapChain*>( window.m_pSwapChain );
        pSwapChain->Present( 0, 0 );
    }
}