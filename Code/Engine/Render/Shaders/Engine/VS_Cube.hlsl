// TODO convert to std: quads, fullscreen tri, cube, etc
cbuffer Transforms : register( b0 )
{;
    matrix m_modelViewProjTransform;
};

struct VertexShaderOutput
{
    float4 m_pixelPosition : SV_POSITION;
    float3 m_worldPosition : POSITION;
};

VertexShaderOutput main( uint i : SV_VertexID )
{
    VertexShaderOutput output;

    float3 vertex;
    vertex.x = (0x287A >> i) & 1; //TODO : check constants
    vertex.y = (0x3D50 >> i) & 1; //TODO : check constants
    vertex.z = (0x31E3 >> i) & 1; //TODO : check constants

    output.m_worldPosition = ( vertex * 2 ) - 1;
    output.m_pixelPosition = mul( m_modelViewProjTransform, float4( output.m_worldPosition, 1.0 ) );

    return output;
}