#include "DebugShaderCommon.hlsli"

PixelShaderOutput main( VertexShaderOutput input )
{
    float d = length( input.m_uv - float2( 0.5, 0.5 ) );
    d = smoothstep( 0.5, 0.5 - ( kAntialiasing / input.m_size ), d );

    PixelShaderOutput output;

    #if WITH_PICKING
    output.m_ID[0] = m_entityID0;
    output.m_ID[1] = m_entityID1;
    #endif

    output.m_color = input.m_color;
    output.m_color.a *= d;
    return output;
}