#pragma once

#include "EngineTools/Resource/ResourceDescriptor.h"
#include "System/Render/RenderShader.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    enum class ShaderType : uint8_t
    {
        EE_REGISTER_ENUM

        Vertex = 0,
        Geometry,
        Pixel,
        Hull,
        Compute,
    };

    struct ShaderResourceDescriptor : public Resource::ResourceDescriptor
    {
        EE_REGISTER_TYPE( ShaderResourceDescriptor );

    public:

        virtual bool IsValid() const override { return m_shaderPath.IsValid(); }
        virtual bool IsUserCreateableDescriptor() const override { return true; }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override { return Shader::GetStaticResourceTypeID(); }

        virtual void GetCompileDependencies( TVector<ResourceID>& outDependencies ) override
        {
            if ( m_shaderPath.IsValid() )
            {
                outDependencies.emplace_back( m_shaderPath );
            }
        }

    public:

        EE_EXPOSE ShaderType           m_shaderType = ShaderType::Vertex;
        EE_EXPOSE ResourcePath         m_shaderPath;
    };
}