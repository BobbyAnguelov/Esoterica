#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "Base/Render/RenderTexture.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    enum class TextureType
    {
        EE_REFLECT_ENUM

        Default,
        AmbientOcclusion,
        TangentSpaceNormals,
        Uncompressed
    };

    //-------------------------------------------------------------------------

    struct EE_ENGINETOOLS_API TextureResourceDescriptor : public Resource::ResourceDescriptor
    {
        EE_REFLECT_TYPE( TextureResourceDescriptor );

        virtual bool IsValid() const override { return m_path.IsValid(); }
        virtual int32_t GetFileVersion() const override { return 0; }
        virtual bool IsUserCreateableDescriptor() const override { return true; }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override { return Texture::GetStaticResourceTypeID(); }
        virtual FileSystem::Extension GetExtension() const override final { return Texture::GetStaticResourceTypeID().ToString(); }
        virtual char const* GetFriendlyName() const override final { return Texture::s_friendlyName; }

        virtual void GetCompileDependencies( TVector<DataPath>& outDependencies ) override
        {
            if ( m_path.IsValid() )
            {
                outDependencies.emplace_back( m_path );
            }
        }

        virtual void Clear() override
        {
            m_path.Clear();
            m_type = TextureType::Default;
            m_name.clear();
        }

    public:

        EE_REFLECT() DataPath     m_path;
        EE_REFLECT() TextureType      m_type = TextureType::Default;
        EE_REFLECT() String           m_name; // Optional: needed for extracting textures out of container files (e.g. glb, fbx)
    };

    //-------------------------------------------------------------------------

    struct EE_ENGINETOOLS_API CubemapTextureResourceDescriptor : public Resource::ResourceDescriptor
    {
        EE_REFLECT_TYPE( CubemapTextureResourceDescriptor );

        virtual bool IsValid() const override { return m_path.IsValid(); }
        virtual int32_t GetFileVersion() const override { return 0; }
        virtual bool IsUserCreateableDescriptor() const override { return true; }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override { return CubemapTexture::GetStaticResourceTypeID(); }
        virtual FileSystem::Extension GetExtension() const override final { return CubemapTexture::GetStaticResourceTypeID().ToString(); }
        virtual char const* GetFriendlyName() const override final { return CubemapTexture::s_friendlyName; }

        virtual void GetCompileDependencies( TVector<DataPath>& outDependencies ) override
        {
            if ( m_path.IsValid() )
            {
                outDependencies.emplace_back( m_path );
            }
        }

        virtual void Clear() override
        {
            m_path.Clear();
        }

    public:

        EE_REFLECT() DataPath     m_path;
    };
}