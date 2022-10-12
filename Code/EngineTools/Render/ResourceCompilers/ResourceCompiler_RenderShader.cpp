#include "ResourceCompiler_RenderShader.h"
#include "EngineTools/Render/ResourceDescriptors/ResourceDescriptor_RenderShader.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Serialization/BinarySerialization.h"

#include <d3dcompiler.h>

//-------------------------------------------------------------------------

namespace EE::Render
{
    static bool GetCBufferDescs( ID3D11ShaderReflection* pShaderReflection, TVector<RenderBuffer>& cbuffers )
    {
        EE_ASSERT( pShaderReflection != nullptr );

        D3D11_SHADER_DESC shaderDesc;
        auto result = pShaderReflection->GetDesc( &shaderDesc );

        if ( FAILED( result ) )
        {
            return false;
        }

        for ( UINT i = 0; i < shaderDesc.ConstantBuffers; i++ )
        {
            ID3D11ShaderReflectionConstantBuffer* pConstBufferDesc = pShaderReflection->GetConstantBufferByIndex( i );
            D3D11_SHADER_BUFFER_DESC desc;
            result = pConstBufferDesc->GetDesc( &desc );
            EE_ASSERT( SUCCEEDED( result ) );

            // Get constant buffer desc
            RenderBuffer buffer;
            buffer.m_ID = Hash::GetHash32( desc.Name );
            buffer.m_byteSize = desc.Size;
            buffer.m_byteStride = 16; // Vector4 aligned
            buffer.m_usage = RenderBuffer::Usage::CPU_and_GPU;
            buffer.m_type = RenderBuffer::Type::Constant;
            buffer.m_slot = i;
            cbuffers.push_back( buffer );
        }

        return true;
    }

    //-------------------------------------------------------------------------

    static bool GetResourceBindingDescs( ID3D11ShaderReflection* pShaderReflection, TVector<Shader::ResourceBinding>& resourceBindings )
    {
        EE_ASSERT( pShaderReflection != nullptr );

        D3D11_SHADER_DESC shaderDesc;
        HRESULT result = pShaderReflection->GetDesc( &shaderDesc );

        if ( FAILED( result ) )
        {
            return false;
        }

        for ( UINT i = 0; i < shaderDesc.BoundResources; i++ )
        {
            D3D11_SHADER_INPUT_BIND_DESC desc;
            result = pShaderReflection->GetResourceBindingDesc( i, &desc );
            if ( SUCCEEDED( result ) )
            {
                Shader::ResourceBinding binding = { Hash::GetHash32( desc.Name ), desc.BindPoint };
                resourceBindings.push_back( binding );
            }
            else
            {
                return false;
            }
        }

        return true;
    }

    //-------------------------------------------------------------------------

    static DataSemantic GetSemanticForName( char const* pName )
    {
        if ( strcmp( pName, "POSITION" ) == 0 )
        {
            return DataSemantic::Position;
        }

        if ( strcmp( pName, "NORMAL" ) == 0 )
        {
            return DataSemantic::Normal;
        }

        if ( strcmp( pName, "TANGENT" ) == 0 )
        {
            return DataSemantic::Tangent;
        }

        if ( strcmp( pName, "BINORMAL" ) == 0 )
        {
            return DataSemantic::BiTangent;
        }

        if ( strcmp( pName, "COLOR" ) == 0 )
        {
            return DataSemantic::Color;
        }

        if ( strcmp( pName, "TEXCOORD" ) == 0 )
        {
            return DataSemantic::TexCoord;
        }

        return DataSemantic::None;
    }

    static bool GetInputLayoutDesc( ID3D11ShaderReflection* pShaderReflection, VertexLayoutDescriptor& vertexLayoutDesc )
    {
        EE_ASSERT( pShaderReflection != nullptr );

        D3D11_SHADER_DESC shaderDesc;
        pShaderReflection->GetDesc( &shaderDesc );

        // Read input layout description from shader info
        for ( UINT i = 0; i < shaderDesc.InputParameters; i++ )
        {
            D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
            if ( FAILED( pShaderReflection->GetInputParameterDesc( i, &paramDesc ) ) )
            {
                return false;
            }

            VertexLayoutDescriptor::ElementDescriptor elementDesc;
            elementDesc.m_semantic = GetSemanticForName( paramDesc.SemanticName );
            elementDesc.m_semanticIndex = (uint16_t) paramDesc.SemanticIndex;

            // Determine DXGI format
            if ( paramDesc.Mask == 1 )
            {
                if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32 ) elementDesc.m_format = DataFormat::UInt_R32;
                else if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32 ) elementDesc.m_format = DataFormat::SInt_R32;
                else if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32 ) elementDesc.m_format = DataFormat::Float_R32;
            }
            else if ( paramDesc.Mask <= 3 )
            {
                if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32 ) elementDesc.m_format = DataFormat::UInt_R32G32;
                else if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32 ) elementDesc.m_format = DataFormat::SInt_R32G32;
                else if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32 ) elementDesc.m_format = DataFormat::Float_R32G32;
            }
            else if ( paramDesc.Mask <= 7 )
            {
                if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32 ) elementDesc.m_format = DataFormat::UInt_R32G32B32;
                else if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32 ) elementDesc.m_format = DataFormat::SInt_R32G32B32;
                else if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32 ) elementDesc.m_format = DataFormat::Float_R32G32B32;
            }
            else if ( paramDesc.Mask <= 15 )
            {
                if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32 ) elementDesc.m_format = DataFormat::UInt_R32G32B32A32;
                else if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32 ) elementDesc.m_format = DataFormat::SInt_R32G32B32A32;
                else if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32 ) elementDesc.m_format = DataFormat::Float_R32G32B32A32;
            }

            vertexLayoutDesc.m_elementDescriptors.push_back( elementDesc );
        }

        vertexLayoutDesc.CalculateByteSize();
        return true;
    }

    //-------------------------------------------------------------------------

    Render::ShaderCompiler::ShaderCompiler()
        : Resource::Compiler( "ShaderCompiler", s_version )
    {
        m_outputTypes.push_back( VertexShader::GetStaticResourceTypeID() );
        m_outputTypes.push_back( PixelShader::GetStaticResourceTypeID() );
    }

    Resource::CompilationResult Render::ShaderCompiler::Compile( Resource::CompileContext const& ctx ) const
    {
        ShaderResourceDescriptor resourceDescriptor;
        if ( !Resource::ResourceDescriptor::TryReadFromFile( *m_pTypeRegistry, ctx.m_inputFilePath, resourceDescriptor ) )
        {
            return Error( "Failed to read resource descriptor from input file: %s", ctx.m_inputFilePath.c_str() );
        }

        Shader* pShader = nullptr;

        // Set compile options
        //-------------------------------------------------------------------------

        String compileTarget;
        if ( resourceDescriptor.m_shaderType == ShaderType::Vertex )
        {
            compileTarget = "vs_5_0";
            pShader = EE::New<VertexShader>();
        }
        else if ( resourceDescriptor.m_shaderType == ShaderType::Pixel )
        {
            compileTarget = "ps_5_0";
            pShader = EE::New<PixelShader>();
        }
        else
        {
            return Error( "Unknown shader type" );
        }

        uint32_t shaderCompileFlags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS;
        #ifdef EE_DEBUG
        shaderCompileFlags |= D3DCOMPILE_DEBUG;
        #endif

        // Load shader file
        //-------------------------------------------------------------------------

        FileSystem::Path shaderFilePath;
        if ( !ConvertResourcePathToFilePath( resourceDescriptor.m_shaderPath, shaderFilePath ) )
        {
            return Error( "Invalid texture data path: %s", resourceDescriptor.m_shaderPath.c_str() );
        }

        Blob fileData;
        if ( !FileSystem::LoadFile( shaderFilePath, fileData ) )
        {
            return Error( "Failed to load specified shader file: %s", shaderFilePath.c_str() );
        }

        // Compile shader file
        //-------------------------------------------------------------------------

        ID3DBlob* pCompiledShaderBlob = nullptr;
        ID3DBlob* pErrorMessagesBlob = nullptr;
        auto result = D3DCompile( fileData.data(), fileData.size(), nullptr, nullptr, nullptr, "main", compileTarget.c_str(), shaderCompileFlags, 0, &pCompiledShaderBlob, &pErrorMessagesBlob );

        if ( FAILED( result ) )
        {
            char const * pErrorMessage = (const char*) pErrorMessagesBlob->GetBufferPointer();
            Error( "Failed to compile specified shader file, error: %u", pErrorMessage );
            pErrorMessagesBlob->Release();
            return Resource::CompilationResult::Failure;
        }

        EE_ASSERT( pErrorMessagesBlob == nullptr );

        // Store compiled shader byte code 
        pShader->m_byteCode.resize( pCompiledShaderBlob->GetBufferSize() );
        memcpy( &pShader->m_byteCode[0], pCompiledShaderBlob->GetBufferPointer(), pCompiledShaderBlob->GetBufferSize() );

        // Reflect shader info
        //-------------------------------------------------------------------------

        ID3D11ShaderReflection* pShaderReflection = nullptr;
        if ( FAILED( D3DReflect( pCompiledShaderBlob->GetBufferPointer(), pCompiledShaderBlob->GetBufferSize(), IID_ID3D11ShaderReflection, (void**) &pShaderReflection ) ) )
        {
            return Error( "Failed to reflect compiled shader file" );
        }

        if ( !GetCBufferDescs( pShaderReflection, pShader->m_cbuffers ) )
        {
            return Error( "Failed to get cbuffer descs" );
        }

        if ( !GetResourceBindingDescs( pShaderReflection, pShader->m_resourceBindings ) )
        {
            return Error( "Failed to get resource binding descs" );
        }

        // If vertex shader
        if ( pShader->GetPipelineStage() == PipelineStage::Vertex )
        {
            auto pVertexShader = static_cast<VertexShader*>( pShader );
            // Get vertex buffer input element descs
            if ( !GetInputLayoutDesc( pShaderReflection, pVertexShader->m_vertexLayoutDesc ) )
            {
                return Error( "Failed to get input element descs for vertex buffer" );
            }
        }

        // Release reflected and compiled shader data - semantic string names were contained in this data
        pShaderReflection->Release();
        pCompiledShaderBlob->Release();

        // Output shader resource
        //-------------------------------------------------------------------------

        Serialization::BinaryOutputArchive archive;

        if ( pShader->GetPipelineStage() == PipelineStage::Pixel )
        {
            Resource::ResourceHeader hdr( s_version, PixelShader::GetStaticResourceTypeID() );
            archive << hdr << *static_cast<PixelShader*>( pShader );
        }
        if ( pShader->GetPipelineStage() == PipelineStage::Vertex )
        {
            Resource::ResourceHeader hdr( s_version, PixelShader::GetStaticResourceTypeID() );
            archive << hdr << *static_cast<VertexShader*>( pShader );
        }

        if ( archive.WriteToFile( ctx.m_outputFilePath ) )
        {
            return CompilationSucceeded( ctx );
        }
        else
        {
            return CompilationFailed( ctx );
        }
    }
}