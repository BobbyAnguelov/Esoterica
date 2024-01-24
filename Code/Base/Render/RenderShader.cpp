#include "RenderShader.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    Shader::Shader( PipelineStage stage )
        : m_pipelineStage( stage )
    {
    }

    Shader::Shader( PipelineStage stage, uint8_t const* pByteCode, size_t const byteCodeSize, TVector<RenderBuffer> const& constBuffers )
        : m_pipelineStage( stage )
    {
        EE_ASSERT( pByteCode != nullptr && byteCodeSize > 0 );

        // Set byte-code and const buffers descriptors
        m_byteCode.resize( byteCodeSize );
        memcpy( m_byteCode.data(), pByteCode, byteCodeSize );
        m_cbuffers = constBuffers;
    }

    //-------------------------------------------------------------------------

    PixelShader::PixelShader( uint8_t const* pByteCode, size_t const byteCodeSize, TVector<RenderBuffer> const& constBuffers )
        : Shader( PipelineStage::Pixel, pByteCode, byteCodeSize, constBuffers )
    {
    }

    //-------------------------------------------------------------------------

    GeometryShader::GeometryShader( uint8_t const* pByteCode, size_t const byteCodeSize, TVector<RenderBuffer> const& constBuffers )
        : Shader( PipelineStage::Geometry, pByteCode, byteCodeSize, constBuffers )
    {
    }

    //-------------------------------------------------------------------------

    VertexShader::VertexShader( uint8_t const* pByteCode, size_t const byteCodeSize, TVector<RenderBuffer> const& constBuffers, VertexLayoutDescriptor const& vertexLayoutDesc )
        : Shader( PipelineStage::Vertex, pByteCode, byteCodeSize, constBuffers )
        , m_vertexLayoutDesc( vertexLayoutDesc )
    {
    }

    ComputeShader::ComputeShader( uint8_t const* pByteCode, size_t const byteCodeSize, TVector<RenderBuffer> const& constBuffers )
        : Shader( PipelineStage::Compute, pByteCode, byteCodeSize, constBuffers )
    {
    }
}