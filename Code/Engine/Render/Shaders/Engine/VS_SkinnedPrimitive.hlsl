#include "Common_Lit.hlsli"

cbuffer Skeleton : register( b1 )
{
    matrix m_boneTransforms[255];
};

struct VertexShaderInput
{
    float3 m_pos : POSITION;
    float3 m_normal : NORMAL;
    float2 m_uv0 : TEXCOORD0;
    float2 m_uv1 : TEXCOORD1;
    int4   m_boneIndices : BLENDINDICES0;
    float4 m_boneWeights : BLENDWEIGHTS0;
};

PixelShaderInput main( VertexShaderInput vsInput )
{
    float3 blendPos = float3(0, 0, 0);
    float3 blendNormal = float3(0, 0, 0);

    for ( int i = 0; i < 4; ++i )
    {
        if ( vsInput.m_boneIndices[i] != -1 )
        {
            matrix boneTransform = m_boneTransforms[vsInput.m_boneIndices[i]];
            blendPos += mul( boneTransform, float4(vsInput.m_pos, 1.0) ).xyz * vsInput.m_boneWeights[i];
            blendNormal += mul( boneTransform, float4(vsInput.m_normal, 0.0) ).xyz * vsInput.m_boneWeights[i]; // HACK: check idea, assumes orthonormal matrix, without scaling
        }
    }

    // re-enable if we need support for 8 bone weights
    //for ( int j = 0; j < 4; ++j )
    //{
    //    if ( vsInput.m_boneIndices1[j] != -1 )
    //    {
    //        matrix boneTransform = m_boneTransforms[vsInput.m_boneIndices1[j]];
    //        blendPos += mul( boneTransform, float4( vsInput.m_pos, 1.0 ) ).xyz * vsInput.m_boneWeights1[j];
    //        blendNormal += mul( boneTransform, float4( vsInput.m_normal, 0.0 ) ).xyz * vsInput.m_boneWeights1[j];
    //    }
    //}

    return GeneratePixelShaderInput(blendPos, blendNormal, vsInput.m_uv0);
}