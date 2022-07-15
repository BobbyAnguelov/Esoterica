#include "DebugShaderCommon.hlsli"

cbuffer TextGlobalData
{
    float4      m_viewportSize;
};

TextVertexShaderOutput main( TextVertexShaderInput input )
{
    float4 pointCS = { 0.0f, 0.0f, 0.0f, 1.0f };

    // Convert to clipspace
    //-------------------------------------------------------------------------

    // To Normalized pixel space
    pointCS.x = input.m_position.x / m_viewportSize.x;
    pointCS.y = input.m_position.y / m_viewportSize.y;

    // Invert Y
    pointCS.y = 1.0f - pointCS.y;

    // Convert from [0,1] to [-1,1]
    pointCS.x = ( pointCS.x * 2 ) - 1.0f;
    pointCS.y = ( pointCS.y * 2 ) - 1.0f;

    // Set PS input
    //-------------------------------------------------------------------------

    TextVertexShaderOutput output;
    output.m_position = pointCS;
    output.m_color = input.m_color;
    output.m_uv = input.m_uv;
    return output;
}