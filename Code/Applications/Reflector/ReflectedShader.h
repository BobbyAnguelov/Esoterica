#pragma once
#include "Base/FileSystem/FileSystemPath.h"
#include "Engine/Render/Shaders/EngineShader.h"

//-------------------------------------------------------------------------

namespace EE::Reflection
{
    class ReflectedShader
    {

    public:

        struct CompiledData
        {
            CompiledData( String const& ID, uint8_t const* pData, size_t dataSize );

            Render::RHI::ShaderStage GetStage() const;

            inline InlineString GetCompileByteCodeVariableName( String const& shaderName ) const
            {
                return InlineString( InlineString::CtorSprintf(), "g_byteCode_%s_%s", shaderName.c_str(), m_ID.c_str() );
            }

        public:

            String                              m_ID; // Identifier string (e.g. VS, PS_Default, PS_AlphaTest, etc... )
            TVector<uint8_t>                    m_byteCode;
        };

        //-------------------------------------------------------------------------

        struct ParameterAnnotation
        {
            ParameterAnnotation() = default;

            ParameterAnnotation( String const& type, String const& value )
                : m_type( type )
                , m_value( value )
            {}

            ParameterAnnotation( String&& type, String&& value )
                : m_type( eastl::forward<String&&>( type ) )
                , m_value( eastl::forward<String&&>( value ) )
            {}

        public:

            String                              m_type;
            String                              m_value;
        };

        struct ParameterInfo
        {
            ParameterInfo() = default;

            ParameterInfo( char const* pType, char const* pName, uint32_t stride, uint32_t offset, bool isHandle = false, TVector<String> const& templateArguments = {}, TVector<ParameterAnnotation> const& annotations = {} );

        public:

            String                              m_type;
            String                              m_name;
            String                              m_friendlyName;
            uint32_t                            m_strideInBytes = 0;
            uint32_t                            m_offsetInBytes = 0;
            bool                                m_isHandle = false;
            bool                                m_isPacked = false;

            TVector<String>                     m_templateArguments;
            TVector<ParameterAnnotation>        m_annotations;
        };

        //-------------------------------------------------------------------------

        struct StageToolMetadata
        {
            char const*                         m_pEntryPoint = nullptr;
            char const*                         m_pStage = nullptr;
            char const*                         m_pMacroDefinitions = nullptr;
        };

    public:

        FileSystem::Path                        m_path;
        String                                  m_name;
        FileSystem::Path                        m_compiledShaderPath;
        FileSystem::Path                        m_reflectedParametersFilePath;
        String                                  m_rawSourceText;
        Render::ShaderType                      m_type = Render::ShaderType::Material;
        TVector<ParameterInfo>                  m_parameters;
        String                                  m_customParametersStructName;
        TVector<ParameterInfo>                  m_resourceTable;
        String                                  m_resourceTableStructName;
        bool                                    m_allowBufferWritesInPixelShader = false;
        bool                                    m_showInResourceEditor = true;
        bool                                    m_useMeshShader = false;
        bool                                    m_useTaskShader = false;
        TVector<CompiledData>                   m_compiledData; // Note: order of shader compilation and the order of this data is important!!!

        TVector<StageToolMetadata>              m_shaderStageToolMetadata;
    };
}
