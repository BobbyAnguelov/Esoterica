#include "Common_Lit.hlsli"

Texture2D albedoTexture    : register( t0 );
Texture2D normalTexture    : register( t1 );
Texture2D metalnessTexture : register( t2 );
Texture2D roughnessTexture : register( t3 );
Texture2D aoTexture        : register( t4 );

Texture2D   shadowMap       : register( t10 );
Texture2D   precomputedBRDF : register( t11 );
TextureCube globalEnvMap    : register( t12 );

sampler shadowSampler : register( s2 );

cbuffer Materials : register( b1 )
{
	uint   m_surfaceFlags;
	float  m_metalness;
	float  m_roughness;
	float  m_normalScaler;
	float3 m_albedo;
};

#if WITH_PICKING
cbuffer EntityID : register( b2 )
{
	uint	m_ID0;
	uint	m_ID1;
	uint	m_ID2;
	uint	m_ID3;
	float4	m_padding; // unused
};
#endif

struct SurfaceParams
{
	float3 normal;
	float3 albedo;
	float  roughness;
	float  metalness;
	float  ao;
};

float3 UnpackNormals(float2 uv, float3 pos, float3 normal, float intensity)
{
	float3 tangentNormal = ReconstructNormal(normalTexture.Sample(bilinearSampler, uv), intensity);

	float3 Q1 = ddx(pos);
	float3 Q2 = ddy(pos);
	float2 st1 = ddx(uv);
	float2 st2 = ddy(uv);

	float3 N = normalize(normal);
	float3 T = normalize(Q1*st2.g - Q2 * st1.g);
	float3 B = normalize(cross(T, N)); // TODO: fix tangent space is not orthogonal
	float3x3 TBN = float3x3(T, B, N);
	return normalize(mul(tangentNormal, TBN));
}

SurfaceParams LoadSurfaceParams(PixelShaderInput psInput)
{
	SurfaceParams surfaceParams;

	surfaceParams.normal =  (m_surfaceFlags&MATERIAL_USE_NORMAL_TEXTURE) ? UnpackNormals(psInput.m_uv, psInput.m_wpos, psInput.m_normal, m_normalScaler) : normalize(psInput.m_normal);

	float3 albedo = (m_surfaceFlags&MATERIAL_USE_ALBEDO_TEXTURE) ? albedoTexture.Sample( bilinearSampler, psInput.m_uv ).rgb : m_albedo;
	// TODFO: WTF??? implement proper sRGB decoding
	surfaceParams.albedo = pow( abs( albedo ), 2.2f);

	float roughness = (m_surfaceFlags&MATERIAL_USE_ROUGHNESS_TEXTURE) ? roughnessTexture.Sample( bilinearSampler, psInput.m_uv ).r : m_roughness;
	surfaceParams.roughness = max(roughness, 0.04);
	surfaceParams.metalness = (m_surfaceFlags&MATERIAL_USE_METALNESS_TEXTURE) ? metalnessTexture.Sample( bilinearSampler, psInput.m_uv ).r : m_metalness;

	surfaceParams.ao = (m_surfaceFlags&MATERIAL_USE_AO_TEXTURE) ? aoTexture.Sample( bilinearSampler, psInput.m_uv ).r : 1.0;

	return surfaceParams;
}

float3 F_SchlickRoughness(float cosTheta, float3 F0, float roughness)
{
	float3 ret = float3(0.0, 0.0, 0.0);
	float powTheta = pow(1.0 - cosTheta, 5.0);
	float invRough = float(1.0 - roughness);

	ret.x = F0.x + (max(invRough, F0.x) - F0.x) * powTheta;
	ret.y = F0.y + (max(invRough, F0.y) - F0.y) * powTheta;
	ret.z = F0.z + (max(invRough, F0.z) - F0.z) * powTheta;

	return ret;
}

float D_GGX(float NoH, float roughness)
{
	float a = roughness*roughness;
	float a2 = a*a;
	float NoH2 = NoH*NoH;
	float nom = a2;
	float denom = (NoH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return nom / denom;
}

float V_SchlickGGX(float NdotV, float roughness)
{
	float r = (roughness + 1.0f);
	float k = (r*r) / 8.0f;

	float nom = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return nom / denom;
}

float V_Smith(float NdotV, float NdotL, float roughness)
{
	float ggx2 = V_SchlickGGX(NdotV, roughness);
	float ggx1 = V_SchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}

float3 F_Schlick(float cosTheta, float3 F0)
{
	return F0 + (1.0f - F0) * pow(1.0 - cosTheta, 5.0);
}

float3 D_Lambert()
{
	return 1.0 / PI;
}

float3 BRDF(float NoL, float NoV, float NoH, float VoH, SurfaceParams surfaceParams)
{	
	float3 F0 = float3(0.04f, 0.04f, 0.04f);
	F0 = lerp(F0, surfaceParams.albedo, surfaceParams.metalness);

	float D = D_GGX(NoH, surfaceParams.roughness);
	float V = V_Smith(NoV, NoL, surfaceParams.roughness);
	float3 F = F_Schlick(NoH, F0);

	float3 kD = (1.0f - F) * surfaceParams.albedo * (1.0f - surfaceParams.metalness);

	float3 Is = D * V * F / (4.0f * NoV * NoL + 0.001f);
	float3 Id = kD * D_Lambert();

	return Id + Is;
}

float3 IndirectBRDF(float3 R, float NoV, SurfaceParams surfaceParams)
{
	float3 F0 = float3(0.04f, 0.04f, 0.04f);
	F0 = lerp(F0, surfaceParams.albedo, surfaceParams.metalness);

	float3 F = F_SchlickRoughness(NoV, F0, surfaceParams.roughness);

	float3 kD = (1.0f - F) * (1.0f - surfaceParams.metalness);

	// TODO: figure out cubemap conventions
	float3 irradiance = globalEnvMap.SampleLevel(bilinearSampler, R, m_roughnessOneLevel).rgb; // Use roughness 1 for diffuse
	float3 specular = globalEnvMap.SampleLevel(bilinearSampler, R, surfaceParams.roughness * m_roughnessOneLevel).rgb;

	float2 maxNVRough = float2(NoV, surfaceParams.roughness);
	float2 brdf = precomputedBRDF.Sample(bilinearClampedSampler, maxNVRough).rg;

	float3 Is = specular * (F0 * brdf.x + brdf.y);
	float3 Id = kD * irradiance * surfaceParams.albedo;

	return Id + Is;
}

float smoothDistanceAtt ( float sqrDist, float invSqrR )
{
	float factor = sqrDist * invSqrR;
	float smoothFactor = saturate(1.0 - sqr(factor));
	return sqr(smoothFactor);
}

float getDistanceAtt(float3 lightVector, float invSqrR)
{
	float sqrDist = dot(lightVector, lightVector);
	float att = 1.0 / (max(sqrDist, 0.01 * 0.01));
	att *= smoothDistanceAtt(sqrDist, invSqrR);

	return att;
}

float getAngleAtt(float3 normLightVector, float3 lightDir, float2 spotAngles )
{
	float cosTheta = dot(lightDir, normLightVector );
	float att = saturate((cosTheta - spotAngles.x) * spotAngles.y );
	return sqr(att);
}

float TestShadow(float3 P)
{
	const float4 projShadowPos = mul( m_sunShadowMapMatrix, float4(P, 1.0f) ); // TODO: use normal offset to remove acne?
	float3 lightSpacePos = projShadowPos.xyz / projShadowPos.w;

	// clip space [-1, 1] --> texture space [0, 1]
	const float2 shadowTexCoords = float2(0.5f, 0.5f) + projShadowPos.xy * float2(0.5f, -0.5f);	// invert Y

	const float BIAS = 0.0001f; // TODo: make configurable or use normal offset

	float shadowing = 0.0f;
	const int rowHalfSize = 2;
	const int kernelSizeSqr = sqr(rowHalfSize * 2 + 1);

	// shadow filter
	[unroll]
	for (int x = -rowHalfSize; x <= rowHalfSize; ++x)
	{
		[unroll]
		for (int y = -rowHalfSize; y <= rowHalfSize; ++y)
		{
			float shadowMapZ = shadowMap.SampleLevel(shadowSampler, shadowTexCoords, 0, int2(x, y)).x;
			shadowing += (lightSpacePos.z - BIAS> shadowMapZ) ? 0.0f : 1.0f; // TODO: optimize via Gather and non box filter
		}
	}

	shadowing /= kernelSizeSqr;
	return shadowing;
}

struct PS_OUTPUT
{
	float4 m_color: SV_Target0;

	#if WITH_PICKING
	uint4 m_ID : SV_Target1;
	#endif
};

PS_OUTPUT main(PixelShaderInput psInput)
{
	PS_OUTPUT output;

	#if WITH_PICKING
	output.m_ID[0] = m_ID0;
	output.m_ID[1] = m_ID1;
	output.m_ID[2] = m_ID2;
	output.m_ID[3] = m_ID3;
	#endif

	//-------------------------------------------------------------

	SurfaceParams surfaceParams = LoadSurfaceParams(psInput);

	switch( m_lightingFlags >> VISUALIZATION_MODE_BITS_SHIFT )
	{
		case VISUALIZATION_MODE_ALBEDO: 
		{
			output.m_color = float4( surfaceParams.albedo, 1.0 );
			return output;
		}

		case VISUALIZATION_MODE_NORMALS: 
		{
			output.m_color = float4( abs( surfaceParams.normal ), 1.0 );
			return output;
		}

		case VISUALIZATION_MODE_METALNESS: 
		{
			output.m_color = float4(surfaceParams.metalness.xxx, 1.0);
			return output;
		}

		case VISUALIZATION_MODE_ROUGHNESS:
		{
			output.m_color = float4( surfaceParams.roughness.xxx, 1.0 );
			return output;
		}

		case VISUALIZATION_MODE_AO:
		{
			output.m_color = float4( surfaceParams.ao.xxx, 1.0 );
			return output;
		}
	}

	const float3 P = psInput.m_wpos;
	const float3 V = -normalize(P);
	const float3 N = surfaceParams.normal;

	float3 Lo = float3(0.0f, 0.0f, 0.0f);	// outgoing radiance

	const float NoV = max(dot(N, V), 0.0);

	if (m_lightingFlags&LIGHTING_ENABLE_SKYLIGHT)
	{
		float ao = surfaceParams.ao;//float aoWithIntensity = _ao  * Get(fAOIntensity) + (1.0f - Get(fAOIntensity)); // TODO: combine with SSAO
		const float3 R = reflect(-V, N);
		Lo += IndirectBRDF(R, NoV, surfaceParams) * m_skyboxLightIntensity * ao;
	}
	//else
	//{
	//	if(HasAOTexture(Get(textureConfig)))
	//		Lo += _albedo * (_ao  * Get(fAOIntensity) + (1.0f - Get(fAOIntensity))) * Get(fAmbientLightIntensity);
	//}

	for (uint i = 0; i < m_numPunctualLights; ++i)
	{
		const float3 lD = m_punctualLights[i].m_position - P;
		const float  invR2 = m_punctualLights[i].m_invRadiusSqr;
		const float3 L = normalize(lD);
		const float  attenuation = getDistanceAtt(lD, invR2) * getAngleAtt(L, m_punctualLights[i].m_dir, m_punctualLights[i].m_spotAngles);
		const float3 H = normalize(V + L);
		const float  NoL = max(dot(N, L), 0.0);
		const float  NoH = max(dot(N, H), 0.0);
		const float  VoH = max(dot(V, H), 0.0);

		Lo += BRDF(NoL, NoV, NoH, VoH, surfaceParams) * m_punctualLights[i].m_color * (attenuation * NoL);
	}

	if (m_lightingFlags&LIGHTING_ENABLE_SUN)
	{
		const float3 L = m_sunDir;
		const float3 H = normalize(V + L);
		const float NoL = max(dot(N, L), 0.0);
		const float NoH = max(dot(N, H), 0.0);
		const float VoH = max(dot(V, H), 0.0);
		float  shadowing = 1.0;
		if (m_lightingFlags & LIGHTING_ENABLE_SUN_SHADOW)
			shadowing = TestShadow(P);

		Lo += BRDF(NoL, NoV, NoH, VoH, surfaceParams) * m_sunColor.rgb * (NoL * shadowing);
	}

	// TODO: Tonemapper:)
	// Gamma correction - can be included in tonemapper
	output.m_color = float4( ToneMap( Lo ), 1.0f );
	return output;
}
