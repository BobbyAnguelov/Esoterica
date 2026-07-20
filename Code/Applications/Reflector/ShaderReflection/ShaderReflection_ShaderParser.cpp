#include "ShaderReflection_ShaderParser.h"
#include "Applications/Reflector/ReflectedProject.h"
#include "Engine/Render/Shaders/EngineShader.esh"
#include "Base/Utils/StringTokenizer.h"
#include "Base/FileSystem/FileSystem.h"

#include "EASTL/sort.h"

//-------------------------------------------------------------------------

namespace EE::Reflection
{
    Render::RHI::ShaderStage ReflectedShader::CompiledData::GetStage() const
    {
        StringView const view( m_ID.c_str() );

        if ( view.starts_with( "MS" ) )
        {
            return Render::RHI::ShaderStage::Mesh;
        }
        else if ( view.starts_with( "TS" ) )
        {
            return Render::RHI::ShaderStage::Task;
        }
        else if ( view.starts_with( "PS" ) )
        {
            return Render::RHI::ShaderStage::Pixel;
        }
        else if ( view.starts_with( "VS" ) )
        {
            return Render::RHI::ShaderStage::Vertex;
        }
        else if ( view.starts_with( "CS" ) )
        {
            return Render::RHI::ShaderStage::Compute;
        }
        else if ( view.starts_with( "RT" ) )
        {
            return Render::RHI::ShaderStage::RayTracing;
        }
        else
        {
            EE_UNREACHABLE_CODE();
        }

        //-------------------------------------------------------------------------

        return Render::RHI::ShaderStage::Vertex;
    }

    uint32_t ParseStructure( StringTokenIterator& tokens, String const& shaderName, StringView expectedParent, uint32_t currentStructureSize,
                             String& structName, TVector<ReflectedShader::ParameterInfo>& members )
    {
        uint32_t resultSize = currentStructureSize;

        if ( tokens.Next() != "struct" )
        {
            PrintError( "Failed to parse structure in \"%s\" shader, expected STRUCT after ESF_CUSTOM_PARAMETERS", shaderName.c_str() );
            return 0;
        }

        StringView parsedStructName = tokens.Next();

        if ( !expectedParent.empty() )
        {
            if ( tokens.Next() != ":" )
            {
                PrintError( "Failed to parse structure in \"%s\" shader, expected inheritance from \"%s\"", shaderName.c_str(), expectedParent.data() );
                return 0;
            }
        }

        if ( !expectedParent.empty() )
        {
            StringView shaderParametersStructName = tokens.Next();
            if ( shaderParametersStructName != "ShaderParameters" )
            {
                PrintError( "Failed to parse structure in \"%s\" shader, expected inheritance from \"ShaderParameters\"", shaderName.c_str() );
                return 0;
            }
        }

        if ( tokens.Next() != "{" )
        {
            PrintError( "Failed to parse structure in \"%s\" shader, unexpected token after struct declaration", shaderName.c_str() );
            return 0;
        }

        structName = parsedStructName;

        while ( tokens.Next() != "}" )
        {
            StringView parameterType = tokens.Peek();
            StringView parameterName = tokens.Next();

            TVector<ReflectedShader::ParameterAnnotation> parameterAnnotations;
            while ( tokens.PeekNext().starts_with( "ESF_" ) )
            {
                StringView annotationType = tokens.Next();
                StringView annotationValue = tokens.Next();

                parameterAnnotations.emplace_back( String( annotationType.begin(), annotationType.end() ), String( annotationValue.begin(), annotationValue.end() ) );
            }

            TVector<String> parameterTemplateArguments;

            size_t bracketIndex = parameterType.find_first_of( '<' );
            if ( bracketIndex != StringView::npos )
            {
                StringView templateArguments = parameterType.substr( bracketIndex );
                templateArguments.remove_suffix( 1 );
                templateArguments.remove_prefix( 1 );

                StringUtils::Split( String( templateArguments.begin(), templateArguments.end() ), parameterTemplateArguments, "," );
                parameterType = parameterType.substr( 0, bracketIndex );
            }

            bool isHandle = false;
            uint32_t parameterStride = 0;
            if ( parameterType == "int" || parameterType == "uint" || parameterType == "float" )
            {
                parameterStride = 4;
            }
            else if ( parameterType == "Color" )
            {
                parameterStride = 4;
            }
            else if ( parameterType == "int2" || parameterType == "uint2" || parameterType == "float2" )
            {
                parameterStride = 8;
            }
            else if ( parameterType == "int4" || parameterType == "uint4" || parameterType == "float4" )
            {
                parameterStride = 16;
            }
            else if ( parameterType == "int16_t" || parameterType == "uint16_t" || parameterType == "float16_t" )
            {
                parameterStride = 2;
            }
            else if ( parameterType == "int16_t2" || parameterType == "uint16_t2" || parameterType == "float16_t2" )
            {
                parameterStride = 4;
            }
            else if ( parameterType == "int16_t4" || parameterType == "uint16_t4" || parameterType == "float16_t4" || parameterType == "uint64_t" || parameterType == "int64_t" )
            {
                parameterStride = 8;
            }
            else if ( parameterType == "float4x4" )
            {
                parameterStride = 16;
            }
            else if ( parameterType == "Buffer" || parameterType == "RWBuffer" || parameterType == "StructuredBuffer" || parameterType == "RWStructuredBuffer" ||
                      parameterType == "Texture" || parameterType == "Texture1D" || parameterType == "Texture2D" || parameterType == "Texture3D" || parameterType == "TextureCube" ||
                      parameterType == "RWTexture" || parameterType == "RWTexture1D" || parameterType == "RWTexture2D" || parameterType == "RWTexture3D" || parameterType == "RWTextureCube" )
            {
                // Assume RHI 16-bit handle
                isHandle = true;
                parameterStride = 2;
            }
            else if ( parameterType == "ByteAddressBuffer" || parameterType == "RWByteAddressBuffer" || parameterType == "SamplerState" )
            {
                // Assume RHI 16-bit handle
                isHandle = true;
                parameterStride = 2;
            }
            else if ( parameterType == "AppendBuffer" || parameterType == "RawAppendBuffer" )
            {
                // 64-bit RHI handle
                isHandle = true;
                parameterStride = 8;
            }
            else
            {
                PrintError( "Failed to parse structure in \"%s\" shader, unsupported parameter type \"%.*s\"", shaderName.c_str(), int( parameterType.size() ), parameterType.data() );
                return 0;
            }

            InlineString typeStr( parameterType.begin(), parameterType.end() );
            InlineString parameterNameStr( parameterName.begin(), parameterName.end() );

            members.emplace_back( typeStr.c_str(), parameterNameStr.c_str(), parameterStride, resultSize, isHandle, parameterTemplateArguments, parameterAnnotations );

            resultSize += parameterStride;
        }

        return resultSize;
    }

    void PackStructure( TVector<ReflectedShader::ParameterInfo>& members, uint32_t alignment, uint32_t offset )
    {
        eastl::stable_sort( members.begin() + offset, members.end(), [] ( ReflectedShader::ParameterInfo const& a, ReflectedShader::ParameterInfo const& b )
        {
            return a.m_strideInBytes > b.m_strideInBytes;
        } );

        uint32_t currentOffsetInBytes = offset * 4;

        for ( size_t memberIndex = offset; memberIndex < members.size(); ++memberIndex )
        {
            ReflectedShader::ParameterInfo& parameter = members[memberIndex];

            if ( parameter.m_strideInBytes == 2 )
            {
                // No padding
                parameter.m_isPacked = true;
            }
            else if ( parameter.m_strideInBytes == 4 )
            {
                // Pad to next dword
                currentOffsetInBytes = Math::RoundUpToNearestMultiple32( currentOffsetInBytes, 4 );
            }
            else if ( parameter.m_strideInBytes == 8 )
            {
                // Pad to next dword
                currentOffsetInBytes = Math::RoundUpToNearestMultiple32( currentOffsetInBytes, 4 );
                parameter.m_isPacked = true;
            }

            parameter.m_offsetInBytes = currentOffsetInBytes;
            currentOffsetInBytes += parameter.m_strideInBytes;
        }

        if ( ( currentOffsetInBytes % 4 ) != 0 )
        {
            // means that the struct has free 16 bits in the last packed member, just adjust the offset
            EE_ASSERT( ( currentOffsetInBytes % 2 ) == 0 );
            currentOffsetInBytes += 2;
        }

        if ( ( currentOffsetInBytes % alignment ) != 0 )
        {
            uint32_t alignedSize = Math::RoundUpToNearestMultiple32( currentOffsetInBytes, alignment );
            uint32_t paddingSize = alignedSize - currentOffsetInBytes;

            EE_ASSERT( ( paddingSize % 4 ) == 0 );
            for ( uint32_t paddingIndex = 0; paddingIndex < paddingSize / 4; ++paddingIndex )
            {
                TInlineString<20> name;
                name.sprintf( "esf_parameter_padding%i", paddingIndex );

                members.emplace_back( "esf_padding_t", name.c_str(), 4, currentOffsetInBytes );
                currentOffsetInBytes += 4;
            }
        }
    }

    //-------------------------------------------------------------------------

    ShaderParser::ShaderParser( TVector<ReflectedShader>& shaders )
        : m_shaders( shaders )
    {}

    bool ShaderParser::ParseShaders()
    {
        for ( ReflectedShader& shader : m_shaders )
        {
            if ( !ParseShader( shader ) )
            {
                return false;
            }
        }

        return true;
    }

    bool ShaderParser::ParseShader( ReflectedShader& shader ) const
    {
        shader.m_rawSourceText.clear();

        if ( !FileSystem::ReadTextFile( shader.m_path, shader.m_rawSourceText ) )
        {
            PrintError( "Failed to read shader file: '%s'", shader.m_path.c_str() );
            return false;
        }

        if ( shader.m_rawSourceText.find_first_of( "﻿" ) == 0 )
        {
            PrintError( "UTF8 BOM detected, please resave this file in an encoding without the BOM: '%s'", shader.m_path.c_str() );
            return false;
        }

        // Setup parameter info
        //-------------------------------------------------------------------------

        shader.m_parameters = { ESF_COMMON_SHADER_PARAMETERS };

        // Parse raw text
        //-------------------------------------------------------------------------

        StringTokenIterator tokens( shader.m_rawSourceText, " \t\n;=()" );
        uint32_t customParametersStructureSize = sizeof( Render::ShaderTypes::ShaderParameters );
        while ( tokens.HasNext() )
        {
            tokens.Next();

            //-----------------------------------------------------------------

            if ( tokens.Peek() == "ESF_CUSTOM_PARAMETERS" )
            {
                if ( !shader.m_customParametersStructName.empty() )
                {
                    PrintError( "Failed to parse custom shader parameters in \"%s\" shader, only one custom parameters struct is supported", shader.m_name.c_str() );
                    return false;
                }

                uint32_t structureSize = ParseStructure( tokens, shader.m_name, "ShaderParameters", customParametersStructureSize, shader.m_customParametersStructName, shader.m_parameters );
                if ( !structureSize )
                {
                    PrintError( "Failed to parse custom shader parameters in \"%s\" shader", shader.m_name.c_str() );
                    return false;
                }
            }

            //-----------------------------------------------------------------

            if ( tokens.Peek() == "ESF_RESOURCE_TABLE" )
            {
                if ( !shader.m_resourceTableStructName.empty() )
                {
                    PrintError( "Failed to parse shader resource table in \"%s\" shader, only one struct is supported", shader.m_name.c_str() );
                    return false;
                }

                uint32_t structureSize = ParseStructure( tokens, shader.m_name, {}, 0, shader.m_resourceTableStructName, shader.m_resourceTable );
                if ( !structureSize )
                {
                    PrintError( "Failed to parse resourceTable in \"%s\" shader", shader.m_name.c_str() );
                    return false;
                }
            }

            //-----------------------------------------------------------------

            if ( tokens.Peek() == "ESF_PARAMETER" )
            {
                StringView parameterType = tokens.Next();
                StringView parameterName = tokens.Next();
                StringView parameterValue = tokens.Next();

                //-------------------------------------------------------------

                if ( parameterName == "ShaderType" )
                {
                    if ( parameterType != "string" )
                    {
                        PrintError( "Failed to parse shader parameters in \"%s\" shader, \"ShaderType\" is expected to be of type \"string\"", shader.m_name.c_str() );
                        return false;
                    }

                    if ( parameterValue == "\"Material\"" )
                    {
                        shader.m_type = Render::ShaderType::Material;
                    }
                    else if ( parameterValue == "\"Surface\"" )
                    {
                        shader.m_type = Render::ShaderType::Surface;
                    }
                    else if ( parameterValue == "\"Compute\"" )
                    {
                        shader.m_type = Render::ShaderType::Compute;
                    }
                    else
                    {
                        PrintError( "Failed to parse shader parameters in \"%s\" shader, invalid parameter value \"%.*s\"", shader.m_name.c_str(), int( parameterValue.size() ), parameterValue.data() );
                        return false;
                    }
                }

                //-------------------------------------------------------------

                else if ( parameterName == "AllowBufferWritesInPixelShader" )
                {
                    if ( parameterType != "bool" )
                    {
                        PrintError( "Failed to parse shader parameters in \"%s\" shader, \"AllowBufferWritesInPixelShader\" is expected to be of type \"bool\"", shader.m_name.c_str() );
                        return false;
                    }

                    if ( parameterValue == "true" )
                    {
                        shader.m_allowBufferWritesInPixelShader = true;
                    }
                    else if ( parameterValue == "false" )
                    {
                        shader.m_allowBufferWritesInPixelShader = false;
                    }
                    else
                    {
                        PrintError( "Failed to parse shader parameters in \"%s\" shader, invalid parameter value \"%.*s\"", shader.m_name.c_str(), int( parameterValue.size() ), parameterValue.data() );
                        return false;
                    }
                }

                //-------------------------------------------------------------

                else if ( parameterName == "ShowInResourceEditor" )
                {
                    if ( parameterType != "bool" )
                    {
                        PrintError( "Failed to parse shader parameters in \"%s\" shader, \"ShowInResourceEditor\" is expected to be of type \"bool\"", shader.m_name.c_str() );
                        return false;
                    }

                    if ( parameterValue == "true" )
                    {
                        shader.m_showInResourceEditor = true;
                    }
                    else if ( parameterValue == "false" )
                    {
                        shader.m_showInResourceEditor = false;
                    }
                    else
                    {
                        PrintError( "Failed to parse shader parameters in \"%s\" shader, invalid parameter value \"%.*s\"", shader.m_name.c_str(), int( parameterValue.size() ), parameterValue.data() );
                        return false;
                    }
                }

                //-------------------------------------------------------------

                else if ( parameterName == "UseMeshShader" )
                {
                    if ( parameterType != "bool" )
                    {
                        PrintError( "Failed to parse shader parameters in \"%s\" shader, \"UseMeshShader\" is expected to be of type \"bool\"", shader.m_name.c_str() );
                        return false;
                    }

                    if ( parameterValue == "true" )
                    {
                        shader.m_useMeshShader = true;
                    }
                    else if ( parameterValue == "false" )
                    {
                        shader.m_useMeshShader = false;
                    }
                    else
                    {
                        PrintError( "Failed to parse shader parameters in \"%s\" shader, invalid parameter value \"%.*s\"", shader.m_name.c_str(), int( parameterValue.size() ), parameterValue.data() );
                        return false;
                    }
                }

                //-------------------------------------------------------------

                else if ( parameterName == "UseTaskShader" )
                {
                    if ( parameterType != "bool" )
                    {
                        PrintError( "Failed to parse shader parameters in \"%s\" shader, \"UseTaskShader\" is expected to be of type \"bool\"", shader.m_name.c_str() );
                        return false;
                    }

                    if ( parameterValue == "true" )
                    {
                        shader.m_useTaskShader = true;
                    }
                    else if ( parameterValue == "false" )
                    {
                        shader.m_useTaskShader = false;
                    }
                    else
                    {
                        PrintError( "Failed to parse shader parameters in \"%s\" shader, invalid parameter value \"%.*s\"", shader.m_name.c_str(), int( parameterValue.size() ), parameterValue.data() );
                        return false;
                    }
                }

                //-------------------------------------------------------------

                else
                {
                    PrintError( "Failed to parse shader parameters in \"%s\" shader, invalid parameter \"%.*s\"", shader.m_name.c_str(), int( parameterName.size() ), parameterName.data() );
                    return false;
                }
            }
        }

        //-------------------------------------------------------------

        PackStructure( shader.m_parameters, 32, 4 );

        if ( !shader.m_resourceTableStructName.empty() )
        {
            PackStructure( shader.m_resourceTable, 8, 0 );
        }

        return true;
    }
}