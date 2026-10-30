#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "Engine/Render/RenderDecalMaterial.h"
#include "Base/Resource/ResourcePtr.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    //-------------------------------------------------------------------------

    struct EE_ENGINETOOLS_API DecalMaterialResourceDescriptor final : public Resource::ResourceDescriptor
    {
        EE_REFLECT_TYPE( DecalMaterialResourceDescriptor );

        virtual bool IsValid() const override { return m_albedoTexture.IsSetAndIsValidResourceTypeID() && m_pbrTexture.IsSetAndIsValidResourceTypeID() && m_normalTexture.IsSetAndIsValidResourceTypeID() && m_emissiveTexture.IsSetAndIsValidResourceTypeID() && m_parallaxTexture.IsSetAndIsValidResourceTypeID(); }
        virtual int32_t GetFileVersion() const override { return 0; }
        virtual bool IsUserCreateableDescriptor() const override { return true; }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override { return DecalMaterial::GetStaticResourceTypeID(); }
        virtual FileSystem::Extension GetExtension() const override final { return DecalMaterial::GetStaticResourceTypeID().ToString(); }
        virtual char const* GetFriendlyName() const override final { return DecalMaterial::s_friendlyName; }

        virtual void GetCompileDependencies( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& sourceResourceDirectoryPath, String const& subResourceName, TVector<Resource::CompileDependency>& outDependencies ) const override
        {
            ForEachDependency( [&outDependencies] ( Resource::ResourcePtr const* pResource ) { outDependencies.emplace_back( pResource->GetDataPath(), true ); } );
        }

        virtual void GetInstallDependencies( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& sourceResourceDirectoryPath, String const& subResourceName, TVector<ResourceID>& outDependencies ) const override
        {
            ForEachDependency( [&outDependencies] ( Resource::ResourcePtr const* pResource ) { VectorEmplaceBackUnique( outDependencies, pResource->GetResourceID() ); } );
        }

        virtual void Clear() override
        {
            m_albedoTexture = ResourceID();
            m_pbrTexture = ResourceID();
            m_normalTexture = ResourceID();
            m_emissiveTexture = ResourceID();
            m_parallaxTexture = ResourceID();
            m_heightOffset = 0.0F;
        }

        template<typename F>
        inline void ForEachDependency( F fn ) const
        {
            if ( m_albedoTexture.IsSet() )
            {
                fn( &m_albedoTexture );
            }

            if ( m_pbrTexture.IsSet() )
            {
                fn( &m_pbrTexture );
            }

            if ( m_normalTexture.IsSet() )
            {
                fn( &m_normalTexture );
            }

            if ( m_emissiveTexture.IsSet() )
            {
                fn( &m_emissiveTexture );
            }

            if ( m_parallaxTexture.IsSet() )
            {
                fn( &m_parallaxTexture );
            }
        }

    public:

        EE_REFLECT( Category = "Decal" );
        TResourcePtr<TextureResource>                   m_albedoTexture = ResourceID( "data://Render/Textures/DefaultColor.texture" );

        EE_REFLECT( Category = "Decal" );
        TResourcePtr<TextureResource>                   m_pbrTexture = ResourceID( "data://Render/Textures/DefaultColor.texture" );

        EE_REFLECT( Category = "Decal" );
        TResourcePtr<TextureResource>                   m_normalTexture = ResourceID( "data://Render/Textures/DefaultNormal.texture" );

        EE_REFLECT( Category = "Decal" );
        TResourcePtr<TextureResource>                   m_emissiveTexture = ResourceID( "data://Render/Textures/DefaultBlack.texture" );

        EE_REFLECT( Category = "Decal" );
        TResourcePtr<TextureResource>                   m_parallaxTexture = ResourceID( "data://Render/Textures/DefaultParallax.texture" );

        EE_REFLECT( Category = "Decal" );
        Color                                           m_tint = Color( 255, 255, 255, 255 );

        EE_REFLECT( Category = "Decal", Min = "0.0F", Max = "1.0F" );
        float                                           m_opacity = 1.0F;

        EE_REFLECT( Category = "Decal", Min = "0.0F" );
        float                                           m_tiling = 1.0F;

        EE_REFLECT( Category = "Decal", Min = "0.0F", Max = "1.0F" );
        float                                           m_shapeRadius = 0.0F;

        EE_REFLECT( Category = "Decal", Min = "-1.0F", Max = "1.0F" );
        float                                           m_heightOffset = 0.0F;

        EE_REFLECT( Category = "Decal" );
        Color                                           m_emissiveColor = Color( 255, 255, 255, 255 );

        EE_REFLECT( Category = "Decal" );
        TBitFlags<DecalFlags>                           m_flags = {};
    };
}
