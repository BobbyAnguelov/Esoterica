
#include "EngineShader.h"
#include "Base/Render/RHI.h"

namespace EE::Render
{
    MaterialShaderParameterHandle MaterialShaderParametersInstance::FindParameter( StringID parameterName ) const
    {
        auto Predicate = [parameterName] ( MaterialShaderParameterInfo const& parameterInfo )
        {
            return parameterInfo.m_parameterName == parameterName;
        };

        auto itr = eastl::find_if( m_parameterInfo.begin(), m_parameterInfo.end(), Predicate );
        if ( itr == m_parameterInfo.end() )
        {
            return {};
        }

        return { itr->m_parameterStrideInBytes, itr->m_parameterOffsetInBytes };
    }

    void MaterialShaderParametersInstance::InitializeDefaultResourceHandles()
    {
        for ( MaterialShaderParameterInfo const& parameterInfo : m_parameterInfo )
        {
            EE_ASSERT( parameterInfo.m_parameterType.IsValid() );
            StringView const parameterTypeStr( parameterInfo.m_parameterType.c_str() );

            MaterialShaderParameterHandle handle = { parameterInfo.m_parameterStrideInBytes, parameterInfo.m_parameterOffsetInBytes };
            if ( parameterTypeStr.starts_with( "Texture" ) )
            {
                SetTexture( handle, RHI::g_invalidResourceHandle );
            }
            else if ( parameterTypeStr.starts_with( "Buffer" ) )
            {
                SetBuffer( handle, RHI::g_invalidResourceHandle );
            }
        }
    }

    bool MaterialShaderParametersInstance::ValidateResourceHandles()
    {
        for ( MaterialShaderParameterInfo const& parameterInfo : m_parameterInfo )
        {
            EE_ASSERT( parameterInfo.m_parameterType.IsValid() );
            StringView const parameterTypeStr( parameterInfo.m_parameterType.c_str() );

            MaterialShaderParameterHandle handle = { parameterInfo.m_parameterStrideInBytes, parameterInfo.m_parameterOffsetInBytes };
            if ( parameterTypeStr.starts_with( "Texture" ) || parameterTypeStr.starts_with( "Buffer" ) )
            {
                if ( *reinterpret_cast<RHI::GenericResourceHandle*>( &m_parametersMemory[parameterInfo.m_parameterOffsetInBytes] ) == RHI::g_invalidResourceHandle )
                {
                    return false;
                }
            }
        }
        return true;
    }

    //-------------------------------------------------------------------------

    MaterialShader::MaterialShader( RHI::Context* pContextRHI, StringID shaderName, ByteCodeList const& stageByteCodes )
        : m_shaderName( shaderName )
    {
        TInlineVector<RHI::ShaderByteCode, 2> shaderPermutationParameters[MaterialShader::PermutationCount];

        // Default
        shaderPermutationParameters[0] =
        {
            stageByteCodes[ByteCodeIndices::MS],
            stageByteCodes[ByteCodeIndices::PS_Default],
        };

        // Depth Only
        shaderPermutationParameters[1] =
        {
            stageByteCodes[ByteCodeIndices::MS]
        };

        // Alpha Test
        shaderPermutationParameters[2] =
        {
            stageByteCodes[ByteCodeIndices::MS],
            stageByteCodes[ByteCodeIndices::PS_AlphaTest],
        };

        // Depth Only Alpha Test
        shaderPermutationParameters[3] =
        {
            stageByteCodes[ByteCodeIndices::MS],
            stageByteCodes[ByteCodeIndices::PS_DepthOnly_AlphaTest],
        };

        //-------------------------------------------------------------------------

        for ( size_t permutationIndex = 0; permutationIndex < MaterialShader::PermutationCount; ++permutationIndex )
        {
            m_shaders[permutationIndex] = RHI::CreateShader( pContextRHI, shaderPermutationParameters[permutationIndex] );
        }

        #if EE_DEVELOPMENT_TOOLS
        if ( stageByteCodes.size() > MaterialShader::ByteCodeIndices::PS_OutlineID )
        {
            TInlineVector<RHI::ShaderByteCode, 2> outlineIDShaderParameters =
            {
                stageByteCodes[MaterialShader::ByteCodeIndices::MS],
                stageByteCodes[MaterialShader::ByteCodeIndices::PS_OutlineID],
            };

            m_pOutlineShader = RHI::CreateShader( pContextRHI, outlineIDShaderParameters );
        }
        #endif

        RHI::RootSignatureParameters rootSignatureParameters = {};
        rootSignatureParameters.m_debugName.sprintf( "%s RootSignature", m_shaderName.c_str() );
        rootSignatureParameters.m_pShader = m_shaders[0];

        m_pRootSignature = RHI::CreateRootSignature( pContextRHI, rootSignatureParameters );

        TInlineVector<RHI::IndirectArgumentDescriptor, 3> commandArguments;
        for ( uint32_t descriptorIndex = 0; descriptorIndex < m_pRootSignature->m_descriptorReflections.size(); ++descriptorIndex )
        {
            RHI::DescriptorReflection const& descriptorReflection = m_pRootSignature->m_descriptorReflections[descriptorIndex];

            if ( descriptorReflection.m_descriptorTypeFlags.IsFlagSet( RHI::DescriptorTypeFlags::RootConstant ) )
            {
                commandArguments.emplace_back( RHI::IndirectArgumentType::Constant, descriptorIndex, descriptorReflection.m_numConstants * uint32_t( sizeof( uint32_t ) ) );
            }
            else if ( descriptorReflection.m_descriptorTypeFlags.IsFlagSet( RHI::DescriptorTypeFlags::ConstantBuffer ) )
            {
                EE_ASSERT( !descriptorReflection.m_descriptorTypeFlags.AreAnyFlagsSet( RHI::DescriptorTypeFlags::Buffer, RHI::DescriptorTypeFlags::RWBuffer ) );
                commandArguments.emplace_back( RHI::IndirectArgumentType::ConstantBufferView, descriptorIndex, 2 * sizeof( uint32_t ) );
            }
            else if ( descriptorReflection.m_descriptorTypeFlags.IsFlagSet( RHI::DescriptorTypeFlags::Buffer ) )
            {
                EE_ASSERT( !descriptorReflection.m_descriptorTypeFlags.AreAnyFlagsSet( RHI::DescriptorTypeFlags::ConstantBuffer, RHI::DescriptorTypeFlags::RWBuffer ) );
                commandArguments.emplace_back( RHI::IndirectArgumentType::ShaderResourceView, descriptorIndex, 2 * sizeof( uint32_t ) );
            }
            else if ( descriptorReflection.m_descriptorTypeFlags.IsFlagSet( RHI::DescriptorTypeFlags::RWBuffer ) )
            {
                EE_ASSERT( !descriptorReflection.m_descriptorTypeFlags.AreAnyFlagsSet( RHI::DescriptorTypeFlags::ConstantBuffer, RHI::DescriptorTypeFlags::Buffer ) );
                commandArguments.emplace_back( RHI::IndirectArgumentType::UnorderedAccessView, descriptorIndex, 2 * sizeof( uint32_t ) );
            }
            else
            {
                EE_UNIMPLEMENTED_FUNCTION();
            }
        }

        commandArguments.emplace_back( RHI::IndirectArgumentType::DispatchMesh );

        RHI::CommandSignatureParameters commandSignatureParameters = {};
        commandSignatureParameters.m_pRootSignature = m_pRootSignature;
        commandSignatureParameters.m_indirectArgumentParameters = commandArguments;
        commandSignatureParameters.m_debugName.sprintf( "%s CommandSignature", m_shaderName.c_str() );

        m_pCommandSignature = RHI::CreateCommandSignature( pContextRHI, commandSignatureParameters );
    }

    void MaterialShader::Shutdown( RHI::Context* pContextRHI )
    {
        RHI::DestroyRootSignature( pContextRHI, eastl::move( m_pRootSignature ) );
        for ( RHI::Shader*& shader : m_shaders )
        {
            RHI::DestroyShader( pContextRHI, eastl::move( shader ) );
        }
        RHI::DestroyCommandSignature( pContextRHI, eastl::move( m_pCommandSignature ) );

        #if EE_DEVELOPMENT_TOOLS
        RHI::DestroyShader( pContextRHI, eastl::move( m_pOutlineShader ) );
        #endif
    }

    //-------------------------------------------------------------------------

    SurfaceShader::SurfaceShader( RHI::Context* pContextRHI, StringID shaderName, ByteCodeList const& stageByteCodes )
        : m_shaderName( shaderName )
    {
        static StringID const s_OutlineIDStageID( "PS_OutlineID" );

        ByteCodeList mainStageByteCodes;
        mainStageByteCodes.reserve( stageByteCodes.size() );
        for ( RHI::ShaderByteCode const& byteCode : stageByteCodes )
        {
            if ( byteCode.m_ID == s_OutlineIDStageID )
            {
                continue;
            }

            mainStageByteCodes.emplace_back( byteCode );
        }

        m_pShader = RHI::CreateShader( pContextRHI, mainStageByteCodes );

        #if EE_DEVELOPMENT_TOOLS
        bool hasOutlineIDPixelShader = false;
        for ( RHI::ShaderByteCode const& byteCode : stageByteCodes )
        {
            if ( byteCode.m_ID == s_OutlineIDStageID )
            {
                hasOutlineIDPixelShader = true;
                break;
            }
        }

        if ( hasOutlineIDPixelShader )
        {
            ByteCodeList outlineIDStageByteCodes;
            outlineIDStageByteCodes.reserve( stageByteCodes.size() );
            for ( RHI::ShaderByteCode const& byteCode : stageByteCodes )
            {
                if ( byteCode.m_stage != RHI::ShaderStage::Pixel || byteCode.m_ID == s_OutlineIDStageID )
                {
                    outlineIDStageByteCodes.emplace_back( byteCode );
                }
            }

            m_pOutlineShader = RHI::CreateShader( pContextRHI, outlineIDStageByteCodes );
        }
        #endif

        RHI::RootSignatureParameters rootSignatureParameters = {};
        rootSignatureParameters.m_pShader = m_pShader;
        rootSignatureParameters.m_debugName.sprintf( "%s RootSignature", m_shaderName.c_str() );

        m_pRootSignature = RHI::CreateRootSignature( pContextRHI, rootSignatureParameters );

        TInlineVector<RHI::IndirectArgumentDescriptor, 3> commandArguments;
        for ( RHI::DescriptorReflection const& descriptorReflection : m_pRootSignature->m_descriptorReflections )
        {
            uint32_t descriptorIndex = uint32_t( commandArguments.size() );

            if ( descriptorReflection.m_descriptorTypeFlags.IsFlagSet( RHI::DescriptorTypeFlags::RootConstant ) )
            {
                commandArguments.emplace_back( RHI::IndirectArgumentType::Constant, descriptorIndex, descriptorReflection.m_numConstants * uint32_t( sizeof( uint32_t ) ) );
            }
            else if ( descriptorReflection.m_descriptorTypeFlags.IsFlagSet( RHI::DescriptorTypeFlags::ConstantBuffer ) )
            {
                EE_ASSERT( !descriptorReflection.m_descriptorTypeFlags.AreAnyFlagsSet( RHI::DescriptorTypeFlags::Buffer, RHI::DescriptorTypeFlags::RWBuffer ) );
                commandArguments.emplace_back( RHI::IndirectArgumentType::ConstantBufferView, descriptorIndex, 2 * sizeof( uint32_t ) );
            }
            else if ( descriptorReflection.m_descriptorTypeFlags.IsFlagSet( RHI::DescriptorTypeFlags::Buffer ) )
            {
                EE_ASSERT( !descriptorReflection.m_descriptorTypeFlags.AreAnyFlagsSet( RHI::DescriptorTypeFlags::ConstantBuffer, RHI::DescriptorTypeFlags::RWBuffer ) );
                commandArguments.emplace_back( RHI::IndirectArgumentType::ShaderResourceView, descriptorIndex, 2 * sizeof( uint32_t ) );
            }
            else if ( descriptorReflection.m_descriptorTypeFlags.IsFlagSet( RHI::DescriptorTypeFlags::RWBuffer ) )
            {
                EE_ASSERT( !descriptorReflection.m_descriptorTypeFlags.AreAnyFlagsSet( RHI::DescriptorTypeFlags::ConstantBuffer, RHI::DescriptorTypeFlags::Buffer ) );
                commandArguments.emplace_back( RHI::IndirectArgumentType::UnorderedAccessView, descriptorIndex, 2 * sizeof( uint32_t ) );
            }
            else
            {
                EE_UNIMPLEMENTED_FUNCTION();
            }
        }

        commandArguments.emplace_back( RHI::IndirectArgumentType::Draw );

        RHI::CommandSignatureParameters commandSignatureParameters = {};
        commandSignatureParameters.m_pRootSignature = m_pRootSignature;
        commandSignatureParameters.m_indirectArgumentParameters = commandArguments;
        commandSignatureParameters.m_debugName.sprintf( "%s CommandSignature", m_shaderName.c_str() );

        m_pCommandSignatureDraw = RHI::CreateCommandSignature( pContextRHI, commandSignatureParameters );

        commandArguments.back().m_type = RHI::IndirectArgumentType::DrawIndexed;

        m_pCommandSignatureDrawIndexed = RHI::CreateCommandSignature( pContextRHI, commandSignatureParameters );

        commandArguments.back().m_type = RHI::IndirectArgumentType::DispatchMesh;

        m_pCommandSignatureMeshDispatch = RHI::CreateCommandSignature( pContextRHI, commandSignatureParameters );
    }

    void SurfaceShader::Shutdown( RHI::Context* pContextRHI )
    {
        RHI::DestroyRootSignature( pContextRHI, eastl::move( m_pRootSignature ) );
        RHI::DestroyShader( pContextRHI, eastl::move( m_pShader ) );
        RHI::DestroyCommandSignature( pContextRHI, eastl::move( m_pCommandSignatureDraw ) );
        RHI::DestroyCommandSignature( pContextRHI, eastl::move( m_pCommandSignatureDrawIndexed ) );
        RHI::DestroyCommandSignature( pContextRHI, eastl::move( m_pCommandSignatureMeshDispatch ) );

        #if EE_DEVELOPMENT_TOOLS
        RHI::DestroyShader( pContextRHI, eastl::move( m_pOutlineShader ) );
        #endif
    }

    //-------------------------------------------------------------------------

    ComputeShader::ComputeShader( RHI::Context* pContextRHI, StringID shaderName, ByteCodeList const& stageByteCodes )
        : m_shaderName( shaderName )
    {
        m_pShader = RHI::CreateShader( pContextRHI, stageByteCodes );

        RHI::RootSignatureParameters computeRootSignatureParameters = {};
        computeRootSignatureParameters.m_pShader = m_pShader;
        computeRootSignatureParameters.m_debugName.sprintf( "%s RootSignature", m_shaderName.c_str() );

        m_pRootSignature = RHI::CreateRootSignature( pContextRHI, computeRootSignatureParameters );

        RHI::ComputePipelineParameters computePipelineParameters = {};
        computePipelineParameters.m_pRootSignature = m_pRootSignature;
        computePipelineParameters.m_pShader = m_pShader;
        computePipelineParameters.m_debugName.sprintf( "%s Pipeline", m_shaderName.c_str() );

        m_pPipeline = RHI::CreatePipeline( pContextRHI, computePipelineParameters );

        TInlineVector<RHI::IndirectArgumentDescriptor, 3> commandArguments;
        for ( RHI::DescriptorReflection const& descriptorReflection : m_pRootSignature->m_descriptorReflections )
        {
            uint32_t descriptorIndex = uint32_t( commandArguments.size() );

            if ( descriptorReflection.m_descriptorTypeFlags.IsFlagSet( RHI::DescriptorTypeFlags::RootConstant ) )
            {
                commandArguments.emplace_back( RHI::IndirectArgumentType::Constant, descriptorIndex, descriptorReflection.m_numConstants * uint32_t( sizeof( uint32_t ) ) );
            }
            else if ( descriptorReflection.m_descriptorTypeFlags.IsFlagSet( RHI::DescriptorTypeFlags::ConstantBuffer ) )
            {
                EE_ASSERT( !descriptorReflection.m_descriptorTypeFlags.AreAnyFlagsSet( RHI::DescriptorTypeFlags::Buffer, RHI::DescriptorTypeFlags::RWBuffer ) );
                commandArguments.emplace_back( RHI::IndirectArgumentType::ConstantBufferView, descriptorIndex, 2 * sizeof( uint32_t ) );
            }
            else if ( descriptorReflection.m_descriptorTypeFlags.IsFlagSet( RHI::DescriptorTypeFlags::Buffer ) )
            {
                EE_ASSERT( !descriptorReflection.m_descriptorTypeFlags.AreAnyFlagsSet( RHI::DescriptorTypeFlags::ConstantBuffer, RHI::DescriptorTypeFlags::RWBuffer ) );
                commandArguments.emplace_back( RHI::IndirectArgumentType::ShaderResourceView, descriptorIndex, 2 * sizeof( uint32_t ) );
            }
            else if ( descriptorReflection.m_descriptorTypeFlags.IsFlagSet( RHI::DescriptorTypeFlags::RWBuffer ) )
            {
                EE_ASSERT( !descriptorReflection.m_descriptorTypeFlags.AreAnyFlagsSet( RHI::DescriptorTypeFlags::ConstantBuffer, RHI::DescriptorTypeFlags::Buffer ) );
                commandArguments.emplace_back( RHI::IndirectArgumentType::UnorderedAccessView, descriptorIndex, 2 * sizeof( uint32_t ) );
            }
            else
            {
                EE_UNIMPLEMENTED_FUNCTION();
            }
        }

        commandArguments.emplace_back( RHI::IndirectArgumentType::DispatchCompute );

        RHI::CommandSignatureParameters commandSignatureParameters = {};
        commandSignatureParameters.m_pRootSignature = m_pRootSignature;
        commandSignatureParameters.m_indirectArgumentParameters = commandArguments;
        commandSignatureParameters.m_debugName.sprintf( "%s CommandSignature", m_shaderName.c_str() );

        m_pCommandSignature = RHI::CreateCommandSignature( pContextRHI, commandSignatureParameters );
    }

    void ComputeShader::Shutdown( RHI::Context* pContextRHI )
    {
        RHI::DestroyPipeline( pContextRHI, eastl::move( m_pPipeline ) );
        RHI::DestroyRootSignature( pContextRHI, eastl::move( m_pRootSignature ) );
        RHI::DestroyShader( pContextRHI, eastl::move( m_pShader ) );
        RHI::DestroyCommandSignature( pContextRHI, eastl::move( m_pCommandSignature ) );
    }
}
