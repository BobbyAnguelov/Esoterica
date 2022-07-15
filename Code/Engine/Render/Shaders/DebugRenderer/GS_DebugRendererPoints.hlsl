#include "DebugShaderCommon.hlsli"

cbuffer ViewData : register( b0 )
{
    float4x4                g_viewProjectionMatrix;
    float2                  g_viewport;
};

[maxvertexcount(4)]
void main( point VertexShaderOutput input[1], inout TriangleStream<VertexShaderOutput> output )
{
    VertexShaderOutput vert;

    float2 scale = 1.0 / g_viewport * input[0].m_size;
    vert.m_size = input[0].m_size;
    vert.m_color = input[0].m_color;
    vert.m_edgeDistance = input[0].m_edgeDistance;

    vert.m_position = float4( input[0].m_position.xy + float2( -1.0, -1.0 ) * scale * input[0].m_position.w, input[0].m_position.zw );
    vert.m_uv = float2( 0.0, 0.0 );
    output.Append( vert );

    vert.m_position = float4( input[0].m_position.xy + float2( 1.0, -1.0 ) * scale * input[0].m_position.w, input[0].m_position.zw );
    vert.m_uv = float2( 1.0, 0.0 );
    output.Append( vert );

    vert.m_position = float4( input[0].m_position.xy + float2( -1.0, 1.0 ) * scale * input[0].m_position.w, input[0].m_position.zw );
    vert.m_uv = float2( 0.0, 1.0 );
    output.Append( vert );

    vert.m_position = float4( input[0].m_position.xy + float2( 1.0, 1.0 ) * scale * input[0].m_position.w, input[0].m_position.zw );
    vert.m_uv = float2( 1.0, 1.0 );
    output.Append( vert );
}