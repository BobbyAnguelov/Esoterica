#include "DebugShaderCommon.hlsli"

PixelShaderOutput main( VertexShaderOutput input )
{
    float d = abs( input.m_edgeDistance ) / input.m_size;
    d = smoothstep( 1.0, 1.0 - ( kAntialiasing / input.m_size ), d );

    PixelShaderOutput output;

    #if WITH_PICKING
    output.m_ID[0] = m_entityID0;
    output.m_ID[1] = m_entityID1;
    #endif

    output.m_color = input.m_color;
    output.m_color.a *= d;
    return output;
}