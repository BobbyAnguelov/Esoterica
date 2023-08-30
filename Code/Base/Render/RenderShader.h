#pragma once

#include "RenderBuffer.h"
#include "Base/Resource/IResource.h"
#include "Base/TypeSystem/ReflectedType.h"

//-------------------------------------------------------------------------

namespace EE
{
    namespace Render
    {
        //-------------------------------------------------------------------------

        class EE_BASE_API Shader : public Resource::IResource
        {
            friend class RenderDevice;
            friend class ShaderCompiler;
            friend class ShaderLoader;

            EE_SERIALIZE( m_cbuffers, m_resourceBindings, m_byteCode );
            EE_RESOURCE( 'shdr', "Render Shader" );

        public:

            struct ResourceBinding
            {
                EE_SERIALIZE( m_ID, m_slot );

                ResourceBinding() : m_slot( 0 ) {}
                ResourceBinding( uint32_t ID, uint32_t slot ) : m_ID( ID ), m_slot( slot ) {}

                uint32_t                 m_ID;
                uint32_t                 m_slot;
            };

        public:

            Shader() = default;
            Shader( Shader const& ) = default;
            virtual ~Shader() { EE_ASSERT( !m_shaderHandle.IsValid() ); }

            Shader& operator=( Shader const& ) = default;

            inline PipelineStage GetPipelineStage() const { return m_pipelineStage; }

            inline ShaderHandle const& GetShaderHandle() const { return m_shaderHandle; }
            inline uint32_t GetNumConstBuffers() const { return (uint32_t) m_cbuffers.size(); }
            inline RenderBuffer const& GetConstBuffer( uint32_t i ) const { EE_ASSERT( i < m_cbuffers.size() ); return m_cbuffers[i]; }

            inline bool operator==( Shader const& rhs ) const { return m_shaderHandle.m_pData == rhs.m_shaderHandle.m_pData; }
            inline bool operator!=( Shader const& rhs ) const { return m_shaderHandle.m_pData != rhs.m_shaderHandle.m_pData; }

        protected:

            Shader( PipelineStage stage );
            Shader( PipelineStage stage, uint8_t const* pByteCode, size_t const byteCodeSize, TVector<RenderBuffer> const& constBuffers );

        protected:

            ShaderHandle                        m_shaderHandle;
            Blob                      m_byteCode;
            TVector<RenderBuffer>               m_cbuffers;
            TVector<ResourceBinding>            m_resourceBindings;
            PipelineStage                       m_pipelineStage;
        };

        //-------------------------------------------------------------------------

        class EE_BASE_API PixelShader : public Shader
        {
            EE_RESOURCE( 'psdr', "Pixel Shader" );

        public:

            PixelShader() : Shader( PipelineStage::Pixel ) {}
            PixelShader( uint8_t const* pByteCode, size_t const byteCodeSize, TVector<RenderBuffer> const& constBuffers );

            virtual bool IsValid() const override { return m_shaderHandle.IsValid(); }
        };

        //-------------------------------------------------------------------------

        class EE_BASE_API GeometryShader : public Shader
        {
            EE_RESOURCE( 'gsdr', "Geometry Shader");

        public:

            GeometryShader() : Shader( PipelineStage::Pixel ) {}
            GeometryShader( uint8_t const* pByteCode, size_t const byteCodeSize, TVector<RenderBuffer> const& constBuffers );

            virtual bool IsValid() const override { return m_shaderHandle.IsValid(); }
        };

        //-------------------------------------------------------------------------

        class EE_BASE_API VertexShader : public Shader
        {
            EE_SERIALIZE( EE_SERIALIZE_BASE( Shader ), m_vertexLayoutDesc );
            EE_RESOURCE( 'vsdr', "Vertex Shader" );

            friend class RenderDevice;
            friend class ShaderCompiler;
            friend class ShaderLoader;

        public:

            VertexShader() : Shader( PipelineStage::Vertex ) {}
            VertexShader( uint8_t const* pByteCode, size_t const byteCodeSize, TVector<RenderBuffer> const& constBuffers, VertexLayoutDescriptor const& vertexLayoutDesc );

            virtual bool IsValid() const override { return m_shaderHandle.IsValid(); }
            inline VertexLayoutDescriptor const& GetVertexLayoutDesc() const { return m_vertexLayoutDesc; }

        private:

            VertexLayoutDescriptor m_vertexLayoutDesc;
        };

        class EE_BASE_API ComputeShader : public Shader
        {
            EE_RESOURCE( 'csdr', "Compute Shader" );

            friend class RenderDevice;
            friend class ShaderCompiler;
            friend class ShaderLoader;

        public:

            ComputeShader() : Shader( PipelineStage::Compute ) {}
            ComputeShader( uint8_t const* pByteCode, size_t const byteCodeSize, TVector<RenderBuffer> const& constBuffers );

            virtual bool IsValid() const override { return m_shaderHandle.IsValid(); }
        };
    }
}