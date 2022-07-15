#include "DebugShaderCommon.hlsli"

sampler sampler0;
Texture2D texture0;

PixelShaderOutput main( TextVertexShaderOutput input )
{
    PixelShaderOutput output;

    #if WITH_PICKING
    output.m_ID[0] = m_entityID0;
    output.m_ID[1] = m_entityID1;
    #endif

    output.m_color = input.m_color * texture0.Sample( sampler0, input.m_uv );
    return output;
}