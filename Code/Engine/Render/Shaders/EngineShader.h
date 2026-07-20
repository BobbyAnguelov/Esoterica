#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Render/RenderProxies.h"
#include "Base/Types/StringID.h"
#include "Base/Types/Arrays.h"
#include "Base/Types/Color.h"
#include "Base/TypeSystem/ReflectedType.h"
#include "Base/Math/Matrix.h"
#include "Base/Render/RHI.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    enum class ShaderType
    {
        EE_REFLECT_ENUM

        Material,
        Surface,
        Compute
    };

    // Shader parameters
    //-------------------------------------------------------------------------

    enum class MaterialShaderFlags
    {
        EE_REFLECT_ENUM

        AlphaTest,
        AlphaBlend,
        TwoSided,
    };

    struct MaterialShaderParameterInfo
    {
        StringID                                        m_parameterType;
        StringID                                        m_parameterName;
        uint32_t                                        m_parameterStrideInBytes;
        uint32_t                                        m_parameterOffsetInBytes;
    };

    struct MaterialShaderParameterHandle
    {
        uint32_t                                        m_parameterStrideInBytes = 0;
        uint32_t                                        m_parameterOffsetInBytes = 0;

        inline bool IsValid() const
        {
            return m_parameterStrideInBytes != 0;
        }
    };

    class MaterialShaderParametersInstance
    {
    public:

        friend class RenderSystem;

        inline bool IsValid() const
        {
            return m_handle.m_handle.IsValid();
        }

        inline void SetScalar( MaterialShaderParameterHandle parameter, uint32_t parameterValue )
        {
            EE_ASSERT( parameter.m_parameterStrideInBytes == sizeof( parameterValue ) );
            *reinterpret_cast<uint32_t*>( &m_parametersMemory[parameter.m_parameterOffsetInBytes] ) = parameterValue;
        }

        inline void SetScalar( MaterialShaderParameterHandle parameter, int32_t parameterValue )
        {
            EE_ASSERT( parameter.m_parameterStrideInBytes == sizeof( parameterValue ) );
            *reinterpret_cast<int32_t*>( &m_parametersMemory[parameter.m_parameterOffsetInBytes] ) = parameterValue;
        }

        inline void SetScalar( MaterialShaderParameterHandle parameter, float parameterValue )
        {
            EE_ASSERT( parameter.m_parameterStrideInBytes == sizeof( parameterValue ) );
            *reinterpret_cast<float*>( &m_parametersMemory[parameter.m_parameterOffsetInBytes] ) = parameterValue;
        }

        inline void SetScalar( MaterialShaderParameterHandle parameter, Color parameterValue )
        {
            EE_ASSERT( parameter.m_parameterStrideInBytes == sizeof( parameterValue ) );

            // Convention is: everything in the pipeline is sRGB, everything in shaders is linear.
            // We do the conversion during material loading or when setting parameter values on the CPU.
            Color linearColor = parameterValue.ToLinear();
            *reinterpret_cast<uint32_t*>( &m_parametersMemory[parameter.m_parameterOffsetInBytes] ) = linearColor.ToUInt32();
        }

        inline void SetVector( MaterialShaderParameterHandle parameter, Int2 parameterValue )
        {
            EE_ASSERT( parameter.m_parameterStrideInBytes == sizeof( parameterValue ) );
            *reinterpret_cast<Int2*>( &m_parametersMemory[parameter.m_parameterOffsetInBytes] ) = parameterValue;
        }

        inline void SetVector( MaterialShaderParameterHandle parameter, Float2 parameterValue )
        {
            EE_ASSERT( parameter.m_parameterStrideInBytes == sizeof( parameterValue ) );
            *reinterpret_cast<Float2*>( &m_parametersMemory[parameter.m_parameterOffsetInBytes] ) = parameterValue;
        }

        inline void SetVector( MaterialShaderParameterHandle parameter, Int4 parameterValue )
        {
            EE_ASSERT( parameter.m_parameterStrideInBytes == sizeof( parameterValue ) );
            *reinterpret_cast<Int4*>( &m_parametersMemory[parameter.m_parameterOffsetInBytes] ) = parameterValue;
        }

        inline void SetVector( MaterialShaderParameterHandle parameter, Float4 parameterValue )
        {
            EE_ASSERT( parameter.m_parameterStrideInBytes == sizeof( parameterValue ) );
            *reinterpret_cast<Float4*>( &m_parametersMemory[parameter.m_parameterOffsetInBytes] ) = parameterValue;
        }

        inline void SetMatrix( MaterialShaderParameterHandle parameter, Matrix const& parameterValue )
        {
            EE_ASSERT( parameter.m_parameterStrideInBytes == sizeof( parameterValue ) );
            *reinterpret_cast<Matrix*>( &m_parametersMemory[parameter.m_parameterOffsetInBytes] ) = parameterValue;
        }

        inline void SetSampler( MaterialShaderParameterHandle parameter, RHI::SamplerStateHandle parameterValue )
        {
            EE_ASSERT( parameter.m_parameterStrideInBytes == sizeof( parameterValue ) );
            *reinterpret_cast<RHI::GenericResourceHandle*>( &m_parametersMemory[parameter.m_parameterOffsetInBytes] ) = parameterValue;
        }

        inline void SetBuffer( MaterialShaderParameterHandle parameter, RHI::BufferHandle parameterValue )
        {
            EE_ASSERT( parameter.m_parameterStrideInBytes == sizeof( parameterValue ) );
            *reinterpret_cast<RHI::GenericResourceHandle*>( &m_parametersMemory[parameter.m_parameterOffsetInBytes] ) = parameterValue;
        }

        inline void SetTexture( MaterialShaderParameterHandle parameter, RHI::TextureHandle parameterValue )
        {
            EE_ASSERT( parameter.m_parameterStrideInBytes == sizeof( parameterValue ) );
            *reinterpret_cast<RHI::GenericResourceHandle*>( &m_parametersMemory[parameter.m_parameterOffsetInBytes] ) = parameterValue;
        }

        inline uint32_t GetBufferOffsetIn32ByteBlocks() const { return uint32_t( m_handle.m_handle.m_offset ); }

        MaterialShaderParameterHandle FindParameter( StringID parameterName ) const;

        void InitializeDefaultResourceHandles();
        bool ValidateResourceHandles();

    private:

        ShaderDataHandle                                m_handle;
        TArrayView<MaterialShaderParameterInfo const>   m_parameterInfo;    // References parameter info from the shader instance
        TArrayView<uint8_t>                             m_parametersMemory; // References data inside of the shader data handle
    };

    using ByteCodeList = TInlineVector<RHI::ShaderByteCode, 2>;

    // Material shader
    //-------------------------------------------------------------------------

    class EE_ENGINE_API MaterialShader
    {
    public:

        enum Permutation : uint32_t
        {
            DepthOnly = 1 << 0,
            AlphaTest = 1 << 1,
            PermutationCount = 1 << 2,
        };

        enum ByteCodeIndices
        {
            MS = 0,
            PS_Default,
            PS_AlphaTest,
            PS_DepthOnly_AlphaTest
        };

    public:

        MaterialShader() = default;
        MaterialShader( RHI::Context* pContextRHI, StringID shaderName, ByteCodeList const& stageByteCodes );

        void Shutdown( RHI::Context* pContextRHI );

    public:

        StringID                                        m_shaderName;
        bool                                            m_allowBufferWritesInPixelShader = false;
        bool                                            m_showInResourceEditor = true;
        StringID                                        m_parametersStructName;
        TVector<MaterialShaderParameterInfo>            m_parameterInfo;
        RHI::RootSignature*                             m_pRootSignature = nullptr;
        TArray<RHI::Shader*, PermutationCount>          m_shaders = {};
        RHI::CommandSignature*                          m_pCommandSignature = nullptr;
    };

    // Surface shader
    //-------------------------------------------------------------------------

    class EE_ENGINE_API SurfaceShader
    {
    public:

        SurfaceShader() = default;
        SurfaceShader( RHI::Context* pContextRHI, StringID shaderName, ByteCodeList const& stageByteCodes );

        void Shutdown( RHI::Context* pContextRHI );

    public:

        StringID                                        m_shaderName;
        RHI::RootSignature*                             m_pRootSignature = nullptr;
        RHI::Shader*                                    m_pShader = nullptr;
        RHI::CommandSignature*                          m_pCommandSignatureDraw = nullptr;
        RHI::CommandSignature*                          m_pCommandSignatureDrawIndexed = nullptr;
        RHI::CommandSignature*                          m_pCommandSignatureMeshDispatch = nullptr;
    };

    // Compute shader
    //-------------------------------------------------------------------------

    class EE_ENGINE_API ComputeShader
    {
    public:

        ComputeShader() = default;
        ComputeShader( RHI::Context* contextRHI, StringID shaderName, ByteCodeList const& stageByteCodes );

        void Shutdown( RHI::Context* contextRHI );

    public:

        StringID                                        m_shaderName;
        RHI::Pipeline*                                  m_pPipeline;
        RHI::RootSignature*                             m_pRootSignature;
        RHI::Shader*                                    m_pShader;
        RHI::CommandSignature*                          m_pCommandSignature;
    };
}
