#pragma once

#include "EngineTools/Resource/ResourceDescriptor.h"
#include "Base/Render/RenderShader.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    enum class ShaderType : uint8_t
    {
        EE_REFLECT_ENUM

        Vertex = 0,
        Geometry,
        Pixel,
        Hull,
        Compute,
    };

    struct ShaderResourceDescriptor : public Resource::ResourceDescriptor
    {
        EE_REFLECT_TYPE( ShaderResourceDescriptor );

    public:

        virtual bool IsValid() const override { return m_shaderPath.IsValid(); }
        virtual int32_t GetFileVersion() const override { return 0; }
        virtual bool IsUserCreateableDescriptor() const override { return true; }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override { return Shader::GetStaticResourceTypeID(); }
        virtual FileSystem::Extension GetExtension() const override final { return Shader::GetStaticResourceTypeID().ToString(); }
        virtual char const* GetFriendlyName() const override final { return Shader::s_friendlyName; }

        virtual void GetCompileDependencies( TVector<DataPath>& outDependencies ) override
        {
            if ( m_shaderPath.IsValid() )
            {
                outDependencies.emplace_back( m_shaderPath );
            }
        }

        virtual void Clear() override
        {
            m_shaderType = ShaderType::Vertex;
            m_shaderPath.Clear();
        }

    public:

        EE_REFLECT() ShaderType           m_shaderType = ShaderType::Vertex;
        EE_REFLECT() DataPath         m_shaderPath;
    };
}