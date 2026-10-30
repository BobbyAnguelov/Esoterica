#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Render/RenderTexture.h"
#include "Base/Resource/IResource.h"
#include "Base/Resource/ResourcePtr.h"
#include "Base/Types/BitFlags.h"
#include "Base/Types/Color.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    //-------------------------------------------------------------------------

    enum class DecalFlags : uint16_t // Has ot match flags in DecalInstance.esh!
    {
        EE_REFLECT_ENUM

        TilingXY,
        TilingXStretchY,
        DisableAlbedo,
        DisableNormal,
        DisableRoughness,
        DisableMetalness,
        DisableOcclusion,
        DisableEmissive,
        AlphaCircle,
        AlphaRoundBox,
        Parallax,
        TwoSided,
        Omnidirectional
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API DecalMaterial final : public Resource::IResource
    {
        EE_RESOURCE( "decalmat", "Decal Material", Colors::MediumPurple, 3, false );
        EE_SERIALIZE
        (
            m_albedoTexture,
            m_pbrTexture,
            m_normalTexture,
            m_emissiveTexture,
            m_parallaxTexture,
            m_tint,
            m_opacity,
            m_tiling,
            m_shapeRadius,
            m_heightOffset,
            m_emissiveColor,
            m_flags
        );

        friend class DecalMaterialLoader;
        friend class DecalMaterialCompiler;

    public:

        inline TextureResource const* GetAlbedoTexture() const { return m_albedoTexture.GetPtr(); }
        inline TextureResource const* GetPBRTexture() const { return m_pbrTexture.GetPtr(); }
        inline TextureResource const* GetNormalTexture() const { return m_normalTexture.GetPtr(); }
        inline TextureResource const* GetEmissiveTexture() const { return m_emissiveTexture.GetPtr(); }
        inline TextureResource const* GetParallaxTexture() const { return m_parallaxTexture.GetPtr(); }

        inline ResourceID const& GetAlbedoTextureResourceID() const { return m_albedoTexture.GetResourceID(); }
        inline ResourceID const& GetPBRTextureResourceID() const { return m_pbrTexture.GetResourceID(); }
        inline ResourceID const& GetNormalTextureResourceID() const { return m_normalTexture.GetResourceID(); }
        inline ResourceID const& GetEmissiveTextureResourceID() const { return m_emissiveTexture.GetResourceID(); }
        inline ResourceID const& GetParallaxTextureResourceID() const { return m_parallaxTexture.GetResourceID(); }

        inline Color const& GetTint() const { return m_tint; }
        inline float GetOpacity() const { return m_opacity; }
        inline float GetTiling() const { return m_tiling; }
        inline float GetShapeRadius() const { return m_shapeRadius; }
        inline float GetHeightOffset() const { return m_heightOffset; }
        inline Color const& GetEmissiveColor() const { return m_emissiveColor; }
        inline TBitFlags<DecalFlags> GetFlags() const { return m_flags; }

        //-------------------------------------------------------------------------

        inline uint16_t GetFlagsMask() const { return uint16_t( m_flags.Get() ); }

        virtual bool IsValid() const override
        {
            return m_albedoTexture.IsSet() && m_pbrTexture.IsSet() && m_normalTexture.IsSet() && m_emissiveTexture.IsSet() && m_parallaxTexture.IsSet();
        }

    private:
        //-------------------------------------------------------------------------

        TResourcePtr<TextureResource>                   m_albedoTexture = ResourceID( "data://Render/Textures/DefaultColor.texture" );
        TResourcePtr<TextureResource>                   m_pbrTexture = ResourceID( "data://Render/Textures/DefaultColor.texture" );
        TResourcePtr<TextureResource>                   m_normalTexture = ResourceID( "data://Render/Textures/DefaultNormal.texture" );
        TResourcePtr<TextureResource>                   m_emissiveTexture = ResourceID( "data://Render/Textures/DefaultBlack.texture" );
        TResourcePtr<TextureResource>                   m_parallaxTexture = ResourceID( "data://Render/Textures/DefaultParallax.texture" );

        Color                                           m_tint = Color( 255, 255, 255, 255 );
        float                                           m_opacity = 1.0F;
        float                                           m_tiling = 1.0F;
        float                                           m_shapeRadius = 0.0F;
        float                                           m_heightOffset = 0.0F;
        Color                                           m_emissiveColor = Color( 255, 255, 255, 255 );

        TBitFlags<DecalFlags>                           m_flags = {};
    };
}
