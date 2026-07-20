#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "Engine/Render/RenderTexture.h"
#include "Base/Resource/ResourcePtr.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    struct EE_ENGINETOOLS_API TextureGroupFileMask final : public IReflectedType
    {
        EE_REFLECT_TYPE( TextureGroupFileMask );

        enum class ChannelMask
        {
            EE_REFLECT_ENUM

            R,
            G,
            B,
            A
        };

        EE_REFLECT();
        String                          m_channelName = {};

        EE_REFLECT();
        TBitFlags<ChannelMask>          m_sourceMask = TBitFlags<ChannelMask>( ChannelMask::R, ChannelMask::G, ChannelMask::B, ChannelMask::A );

        EE_REFLECT();
        TBitFlags<ChannelMask>          m_destinationMask = TBitFlags<ChannelMask>( ChannelMask::R, ChannelMask::G, ChannelMask::B, ChannelMask::A );

        EE_REFLECT();
        uint32_t                        m_depthSlice = 0;

        EE_REFLECT();
        uint32_t                        m_arrayIndex = 0;
    };

    //-------------------------------------------------------------------------

    struct EE_ENGINETOOLS_API TextureGroupMipSettings final : public IReflectedType
    {
        EE_REFLECT_TYPE( TextureGroupMipSettings );

        enum class MipFilter
        {
            EE_REFLECT_ENUM

            LinearDownscale,
            EnvironmentMapConvolution,
        };

        EE_REFLECT();
        bool                            m_generateMipmaps = false;

        EE_REFLECT();
        MipFilter                       m_mipmapFilter = MipFilter::LinearDownscale;
    };

    //-------------------------------------------------------------------------

    struct EE_ENGINETOOLS_API TextureGroupCompressionSettings final : public IReflectedType
    {
        EE_REFLECT_TYPE( TextureGroupCompressionSettings );

        enum class TextureType
        {
            EE_REFLECT_ENUM

            Uncompressed,
            UncompressedSRGB,
            Color,
            ColorSRGB,
            Grayscale1Channel,
            Grayscale2Channel,
            FloatingPoint3Channel,
        };

        EE_REFLECT();
        TextureType                     m_textureType = TextureType::Uncompressed;

        EE_REFLECT();
        bool                            m_isCubemap = false;
    };

    //-------------------------------------------------------------------------

    struct EE_ENGINETOOLS_API TextureGroup final : public IDataFile
    {
        EE_DATA_FILE( TextureGroup, "txtg", "Texture Group", 1 );

    public:

        EE_REFLECT();
        TVector<TextureGroupFileMask>   m_fileMasks;

        EE_REFLECT();
        TextureGroupMipSettings         m_mipSettings;

        EE_REFLECT();
        TextureGroupCompressionSettings m_compressionSettings;
    };

    //-------------------------------------------------------------------------

    struct EE_ENGINETOOLS_API TextureResourceDescriptor final : public Resource::ResourceDescriptor
    {
        EE_REFLECT_TYPE( TextureResourceDescriptor );

        virtual bool IsValid() const override { return m_textureGroup.IsValid() && !m_sourcePaths.empty() && m_sourcePaths[0].IsValid(); }
        virtual int32_t GetFileVersion() const override { return 0; }
        virtual bool IsUserCreateableDescriptor() const override { return true; }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override { return TextureResource::GetStaticResourceTypeID(); }
        virtual FileSystem::Extension GetExtension() const override { return TextureResource::GetStaticResourceTypeID().ToString(); }
        virtual char const* GetFriendlyName() const override { return TextureResource::s_friendlyName; }

        virtual void GetCompileDependencies( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& sourceResourceDirectoryPath, String const& subResourceName, TVector<Resource::CompileDependency>& outDependencies ) const override
        {
            for ( DataPath const& path : m_sourcePaths )
            {
                if ( path.IsValid() )
                {
                    outDependencies.emplace_back( path, false );
                }
            }

            if ( m_textureGroup.IsValid() )
            {
                outDependencies.emplace_back( m_textureGroup, false );
            }
        }

        virtual void Clear() override
        {
            m_sourcePaths.clear();
            m_textureGroup.Clear();
            m_name.clear();
        }

    public:

        EE_REFLECT();
        TVector<DataPath>               m_sourcePaths;

        EE_REFLECT();
        TDataFilePath<TextureGroup>     m_textureGroup;

        EE_REFLECT();
        String                          m_name; // Optional: needed for extracting textures out of container files (e.g. glb, fbx)

        EE_REFLECT();
        bool                            m_invertNormalMap = false; // Inverts the green channel in the input texture
    };
}
