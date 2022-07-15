#include "DebugShaderCommon.hlsli"

cbuffer ViewData : register( b0 )
{
    float4x4                g_viewProjectionMatrix;
    float2                  g_viewport;
};

VertexShaderOutput main( VertexShaderInput input )
{
    VertexShaderOutput output;
    output.m_position = mul( g_viewProjectionMatrix, float4( input.m_positionSize.xyz, 1.0 ) );
    output.m_color = input.m_color;
    output.m_color.a *= smoothstep( 0.0, 1.0, input.m_positionSize.w / kAntialiasing );
    output.m_size = max( input.m_positionSize.w, kAntialiasing );
    output.m_edgeDistance = 0;
    output.m_uv = float2( 0, 0 );
    return output;
}