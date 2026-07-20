
#include "Component_Lights.h"
#include "Engine/Render/Device/DeviceRenderWorld.h"

namespace EE::Render
{
    // https://bottosson.github.io/posts/oklab/
    namespace Oklab
    {
        struct Lab { float L; float a; float b; };
        struct RGB { float r; float g; float b; };

        static Lab linear_srgb_to_oklab( RGB c )
        {
            float l = 0.4122214708f * c.r + 0.5363325363f * c.g + 0.0514459929f * c.b;
            float m = 0.2119034982f * c.r + 0.6806995451f * c.g + 0.1073969566f * c.b;
            float s = 0.0883024619f * c.r + 0.2817188376f * c.g + 0.6299787005f * c.b;

            float l_ = cbrtf( l );
            float m_ = cbrtf( m );
            float s_ = cbrtf( s );

            return {
                0.2104542553f * l_ + 0.7936177850f * m_ - 0.0040720468f * s_,
                1.9779984951f * l_ - 2.4285922050f * m_ + 0.4505937099f * s_,
                0.0259040371f * l_ + 0.7827717662f * m_ - 0.8086757660f * s_,
            };
        }

        static RGB oklab_to_linear_srgb( Lab c )
        {
            float l_ = c.L + 0.3963377774f * c.a + 0.2158037573f * c.b;
            float m_ = c.L - 0.1055613458f * c.a - 0.0638541728f * c.b;
            float s_ = c.L - 0.0894841775f * c.a - 1.2914855480f * c.b;

            float l = l_ * l_ * l_;
            float m = m_ * m_ * m_;
            float s = s_ * s_ * s_;

            return {
                +4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s,
                -1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s,
                -0.0041960863f * l - 0.7034186147f * m + 1.7076147010f * s,
            };
        }
    }

    // Blackbody -> linear sRGB
    // https://www.shadertoy.com/view/lsSXW1
    // http://www.tannerhelland.com/4435/convert-temperature-rgb-algorithm-code/
    Float3 ColorTemperatureToRGB( float temperatureInKelvins )
    {
        Float3 retColor;

        temperatureInKelvins = Math::Clamp( temperatureInKelvins, 1000.0F, 40000.0F ) / 100.0F;

        if ( temperatureInKelvins <= 66.0F )
        {
            retColor.m_x = 1.0;
            retColor.m_y = Math::Clamp( 0.39008157876901960784F * Math::Log10( temperatureInKelvins ) - 0.63184144378862745098F, 0.0F, 1.0F );
        }
        else
        {
            float t = temperatureInKelvins - 60.0F;
            retColor.m_x = Math::Clamp( 1.29293618606274509804F * Math::Pow( t, -0.1332047592F ), 0.0F, 1.0F );
            retColor.m_y = Math::Clamp( 1.12989086089529411765F * Math::Pow( t, -0.0755148492F ), 0.0F, 1.0F );
        }

        if ( temperatureInKelvins >= 66.0F )
        {
            retColor.m_z = 1.0F;
        }
        else if ( temperatureInKelvins <= 19.0F )
        {
            retColor.m_z = 0.0F;
        }
        else
        {
            retColor.m_z = Math::Clamp( 0.54320678911019607843F * Math::Log10( temperatureInKelvins - 10.0F ) - 1.19625408914F, 0.0F, 1.0F );
        }

        return retColor;
    }

    static Color ColorTemperatureToRGB( float temperatureInKelvins, Color tint )
    {
        Float3 color = ColorTemperatureToRGB( temperatureInKelvins );
        Float4 linearTint = tint.ToLinear().ToFloat4();

        Oklab::Lab mainColor = Oklab::linear_srgb_to_oklab( { color.m_x, color.m_y, color.m_z } );
        Oklab::Lab tintColor = Oklab::linear_srgb_to_oklab( { linearTint.m_x, linearTint.m_y, linearTint.m_z } );

        Oklab::RGB result = Oklab::oklab_to_linear_srgb
        (
            Oklab::Lab
            {
                mainColor.L,
                mainColor.a + ( tintColor.a - mainColor.a ) * linearTint.m_w,
                mainColor.b + ( tintColor.b - mainColor.b ) * linearTint.m_w
            }
        );

        result.r = Math::Clamp( result.r, 0.0F, 1.0F );
        result.g = Math::Clamp( result.g, 0.0F, 1.0F );
        result.b = Math::Clamp( result.b, 0.0F, 1.0F );

        return Color( Float4( result.r, result.g, result.b, 1.0F ) );
    }

    void DirectionalLightComponent::OnWorldTransformUpdated()
    {
        if ( !m_lightInstanceProxy.IsValid() )
        {
            return;
        }

        Color tintedColor = ColorTemperatureToRGB( GetTemperature(), GetTint() );
        m_lightInstanceProxy.WriteDirectionalLight( -GetLightDirection().ToFloat3(), GetMaxIntensity(), tintedColor, m_cascadedShadowIndex );
    }

    //-------------------------------------------------------------------------------------------------------

    void PointLightComponent::OnWorldTransformUpdated()
    {
        if ( !m_lightInstanceProxy.IsValid() )
        {
            return;
        }

        Color tintedColor = ColorTemperatureToRGB( GetTemperature(), GetTint() );
        m_lightInstanceProxy.WritePointLight( GetWorldTransform().GetTranslation(), GetMaxIntensity(), GetMaxRadius(), GetFalloff(), tintedColor, 0xFFFF );
    }

    //-------------------------------------------------------------------------------------------------------

    void SpotLightComponent::OnWorldTransformUpdated()
    {
        if ( !m_lightInstanceProxy.IsValid() )
        {
            return;
        }

        float beamHalfAngle = GetBeamAngle().ToRadians().ToFloat() * 0.5F;
        float outerCos = Math::Cos( beamHalfAngle );
        float innerCos = Math::Cos( beamHalfAngle * ( 1.0F - Math::Clamp( GetBlend(), 0.0F, 1.0F ) ) );

        Color tintedColor = ColorTemperatureToRGB( GetTemperature(), GetTint() );
        m_lightInstanceProxy.WriteSpotLight( GetWorldTransform().GetTranslation(), -GetLightDirection().ToFloat3(), GetMaxIntensity(), GetMaxRadius(), GetFalloff(), tintedColor, innerCos, outerCos, 0xFFFF );
    }
}
