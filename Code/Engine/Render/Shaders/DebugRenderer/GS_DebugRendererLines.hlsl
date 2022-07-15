#include "DebugShaderCommon.hlsli"

cbuffer ViewData : register( b0 )
{
    float4x4                g_viewProjectionMatrix;
    float2                  g_viewport;
};

[maxvertexcount( 4 )]
void main( line VertexShaderOutput input[2], inout TriangleStream<VertexShaderOutput> output )
{
    float2 pos0 = input[0].m_position.xy / input[0].m_position.w;
    float2 pos1 = input[1].m_position.xy / input[1].m_position.w;

    float2 dir = pos0 - pos1;
    dir = normalize( float2( dir.x, dir.y * g_viewport.y / g_viewport.x ) ); // correct for aspect ratio
    float2 tng0 = float2( -dir.y, dir.x );
    float2 tng1 = tng0 * input[1].m_size / g_viewport;
    tng0 = tng0 * input[0].m_size / g_viewport;

    VertexShaderOutput vert;

    // line start
    vert.m_size = input[0].m_size;
    vert.m_color = input[0].m_color;
    vert.m_uv = float2( 0.0, 0.0 );

    vert.m_position = float4( ( pos0 - tng0 ) * input[0].m_position.w, input[0].m_position.zw );
    vert.m_edgeDistance = -input[0].m_size;
    output.Append( vert );

    vert.m_position = float4( ( pos0 + tng0 ) * input[0].m_position.w, input[0].m_position.zw );
    vert.m_edgeDistance = input[0].m_size;
    output.Append( vert );

    // line end
    vert.m_size = input[1].m_size;
    vert.m_color = input[1].m_color;
    vert.m_uv = float2( 1.0, 1.0 );

    vert.m_position = float4( ( pos1 - tng1 ) * input[1].m_position.w, input[1].m_position.zw );
    vert.m_edgeDistance = -input[1].m_size;
    output.Append( vert );

    vert.m_position = float4( ( pos1 + tng1 ) * input[1].m_position.w, input[1].m_position.zw );
    vert.m_edgeDistance = input[1].m_size;
    output.Append( vert );
}