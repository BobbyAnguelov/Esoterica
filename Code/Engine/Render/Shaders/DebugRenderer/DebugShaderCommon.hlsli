#define kAntialiasing 2.0

//-------------------------------------------------------------------------

#if WITH_PICKING
cbuffer EntityID : register( b0 )
{
    uint	m_entityID0;
    uint	m_entityID1;
    uint	m_padding0; // unused
    uint	m_padding1; // unused
    float4	m_padding2; // unused
};
#endif

struct VertexShaderInput
{
    float4                  m_positionSize  : POSITION;
    float4                  m_color         : COLOR;
};

struct VertexShaderOutput
{
    linear        float4    m_position      : SV_POSITION;
    linear        float4    m_color         : COLOR;
    linear        float2    m_uv            : TEXCOORD;
    noperspective float     m_size          : SIZE;
    noperspective float     m_edgeDistance  : EDGE_DISTANCE;
};

//-------------------------------------------------------------------------

struct TextVertexShaderInput
{
    float2 m_position : POSITION;
    float2 m_uv  : TEXCOORD0;
    float4 m_color : COLOR0;
};

struct TextVertexShaderOutput
{
    float4 m_position : SV_POSITION;
    float4 m_color : COLOR0;
    float2 m_uv  : TEXCOORD0;
};

//-------------------------------------------------------------------------

struct PixelShaderOutput
{
    float4                  m_color : SV_TARGET0;

    #if WITH_PICKING
    uint                    m_ID[2] : SV_Target1;
    #endif
};