#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "System/Render/RenderTexture.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    enum class TextureType
    {
        EE_REFLECT_ENUM

        Default,
        AmbientOcclusion,
        TangentSpaceNormals,
    };

    //-------------------------------------------------------------------------

    struct EE_ENGINETOOLS_API TextureResourceDescriptor : public Resource::ResourceDescriptor
    {
        EE_REFLECT_TYPE( TextureResourceDescriptor );

        virtual bool IsValid() const override { return m_path.IsValid(); }
        virtual bool IsUserCreateableDescriptor() const override { return true; }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override { return Texture::GetStaticResourceTypeID(); }

        virtual void GetCompileDependencies( TVector<ResourceID>& outDependencies ) override
        {
            if ( m_path.IsValid() )
            {
                outDependencies.emplace_back( m_path );
            }
        }

    public:

        EE_REFLECT() ResourcePath     m_path;
        EE_REFLECT() TextureType      m_type = TextureType::Default;
        EE_REFLECT() String           m_name; // Optional: needed for extracting textures out of container files (e.g. glb, fbx)
    };

    //-------------------------------------------------------------------------

    struct EE_ENGINETOOLS_API CubemapTextureResourceDescriptor : public Resource::ResourceDescriptor
    {
        EE_REFLECT_TYPE( CubemapTextureResourceDescriptor );

        virtual bool IsValid() const override { return m_path.IsValid(); }
        virtual bool IsUserCreateableDescriptor() const override { return true; }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override { return CubemapTexture::GetStaticResourceTypeID(); }

        virtual void GetCompileDependencies( TVector<ResourceID>& outDependencies ) override
        {
            if ( m_path.IsValid() )
            {
                outDependencies.emplace_back( m_path );
            }
        }

    public:

        EE_REFLECT() ResourcePath     m_path;
    };
}