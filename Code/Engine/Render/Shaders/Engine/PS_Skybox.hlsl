#include "Common_lit.hlsli"

TextureCube skybox : register( t0 );

struct VerteShaderOutput
{
    float4 m_pos : SV_POSITION;
    float3 m_wpos : POSITION;
};

float4 main(VerteShaderOutput input) : SV_TARGET
{
    float4 color = skybox.Sample(bilinearSampler, input.m_wpos);
    return float4( ToneMap( color.rgb * m_skyboxLightIntensity ), color.a );
}