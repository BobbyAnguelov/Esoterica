#include "DebugShaderCommon.hlsli"

PixelShaderOutput main( VertexShaderOutput input )
{
    PixelShaderOutput output;

    #if WITH_PICKING
    output.m_ID[0] = m_entityID0;
    output.m_ID[1] = m_entityID1;
    #endif

    output.m_color = input.m_color;
    return output;
}