cbuffer Transforms : register( b0 )
{
    matrix m_worldTransform; // TODO: move to per instance data and apply camera centric world transform in veretx shader(campos=(0, 0, 0)), currently done on CPU
    matrix m_normalTransform;
    matrix m_viewprojTransform;
};

// TODO sync with ENGINE, via header file!!!!

static const uint LIGHTING_ENABLE_SUN = 1 << 0;
static const uint LIGHTING_ENABLE_SUN_SHADOW = 1 << 1;
static const uint LIGHTING_ENABLE_SKYLIGHT = 1 << 2;

static const uint MATERIAL_USE_ALBEDO_TEXTURE = (1 << 0);
static const uint MATERIAL_USE_NORMAL_TEXTURE = (1 << 1);
static const uint MATERIAL_USE_METALNESS_TEXTURE = (1 << 2);
static const uint MATERIAL_USE_ROUGHNESS_TEXTURE = (1 << 3);
static const uint MATERIAL_USE_AO_TEXTURE = (1 << 4);

static const uint VISUALIZATION_MODE_NONE = 0;
static const uint VISUALIZATION_MODE_ALBEDO = 1;
static const uint VISUALIZATION_MODE_NORMALS = 2;
static const uint VISUALIZATION_MODE_METALNESS = 3;
static const uint VISUALIZATION_MODE_ROUGHNESS = 4;
static const uint VISUALIZATION_MODE_AO = 5;

static const uint VISUALIZATION_MODE_BITS_SHIFT = 32 - 3;

static const uint MAX_PUNCTUAL_LIGHTS = 16;

struct PunctualLight
{
    float3 m_position;
    float  m_invRadiusSqr;
    float3 m_dir;
    float3 m_color;
    float2 m_spotAngles;
};

cbuffer Lights : register( b0 )
{
    float3        m_sunDir;
    float         m_skyboxLightIntensity; // TODO: either we use color+intensity of bake that into envmap.
    float3        m_sunColor;
    float         m_roughnessOneLevel;
    Matrix        m_sunShadowMapMatrix;
    float         m_manualExposure;
    uint          m_lightingFlags;
    uint          m_numPunctualLights;
    PunctualLight m_punctualLights[MAX_PUNCTUAL_LIGHTS];
};

sampler bilinearSampler : register( s0 );
sampler bilinearClampedSampler : register( s1 );

static const float PI = 3.14159265359;

struct PixelShaderInput
{
    float4 m_pos : SV_POSITION;
    float3 m_wpos : POSITION;
    float3 m_normal : NORMAL;
    float2 m_uv : TEXCOORD;
};

PixelShaderInput GeneratePixelShaderInput(float3 objectPos, float3 objectNormal, float2 uv)
{
    PixelShaderInput output;
    output.m_wpos = mul( m_worldTransform, float4(objectPos, 1.0) ).xyz;
    output.m_normal = mul( m_normalTransform, float4(objectNormal, 0.0) ).xyz;
    output.m_pos = mul( m_viewprojTransform, float4(output.m_wpos, 1.0) );
    output.m_uv = uv;
    return output;
}

float3 ReconstructNormal(float4 sampleNormal, float intensity)
{
    float3 tangentNormal;
    tangentNormal.xy = (sampleNormal.rg * 2 - 1) * intensity;
    tangentNormal.z = sqrt(1 - saturate(dot(tangentNormal.xy, tangentNormal.xy)));
    return tangentNormal;
}

float sqr(float x)
{
    return x * x;
}

float pow4(float x)
{
    float x2 = x * x;
    return x2 * x2;
}

// Simplified code from https://github.com/TheRealMJP/BakingLab
static const float FP16Scale = 0.0009765625f;

static const float3x3 ACESInputMat =
{
    {0.59719, 0.35458, 0.04823},
    {0.07600, 0.90834, 0.01566},
    {0.02840, 0.13383, 0.83777}
};

// ODT_SAT => XYZ => D60_2_D65 => sRGB
static const float3x3 ACESOutputMat =
{
    { 1.60475, -0.53108, -0.07367},
    {-0.10208,  1.10813, -0.00605},
    {-0.00327, -0.07276,  1.07602}
};

float3 RRTAndODTFit( float3 v )
{
    float3 a = v * ( v + 0.0245786f ) - 0.000090537f;
    float3 b = v * ( 0.983729f * v + 0.4329510f ) + 0.238081f;
    return a / b;
}

float3 ACESFitted( float3 color )
{
    color = mul( ACESInputMat, color );

    // Apply RRT and ODT
    color = RRTAndODTFit( color );

    color = mul( ACESOutputMat, color );

    // Clamp to [0, 1]
    color = saturate( color );

    return color;
}

float3 LinearTosRGB( in float3 color )
{
    float3 x = color * 12.92f;
    float3 y = 1.055f * pow( saturate( color ), 1.0f / 2.4f ) - 0.055f;

    float3 clr = color;
    clr.r = color.r < 0.0031308f ? x.r : y.r;
    clr.g = color.g < 0.0031308f ? x.g : y.g;
    clr.b = color.b < 0.0031308f ? x.b : y.b;

    return clr;
}

float3 CalcExposedColor( in float3 color, in float exposure )
{
    return exp2( exposure ) * color;
}

float3 ToneMap( float3 color )
{
    float exposure = m_manualExposure;// - log2(FP16Scale);
    return LinearTosRGB( ACESFitted( CalcExposedColor( color, exposure ) ) * 1.8f );
}