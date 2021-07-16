#include "light_tools.hlsl"

SamplerState samplerPointWrap       : register(s0);
SamplerState samplerLinearWrap      : register(s1);
SamplerState samplerAnisotropicWrap : register(s2);
SamplerComparisonState samplerShadowMap : register(s3);

cbuffer cbPass : register(b0)
{
    float4x4 vView;
    float4x4 vInvView;
    float4x4 vProj;
    float4x4 vInvProj;
    float4x4 vViewProj;
    float4x4 vInvViewProj;
	
    float3   vCameraPos;
	float    fSaturation;
	
    float2   vRenderTargetSize;
    float2   vInvRenderTargetSize;
	
    float    fNearZ;
    float    fFarZ;
    float    fTotalTime;
    float    fDeltaTime;

    float4   vAmbientLight;
    float3   vCameraMultiplyColor;
    float    fGamma;

	int iDirectionalLightCount;
	int iPointLightCount;
	int iSpotLightCount;
	
	int iTextureFilterIndex;

    float4 vFogColor;
	float  fFogStart;
	float  fFogRange;

    int iMainWindowWidth;
	int iMainWindowHeight;
	
    Light    vLights[MAX_LIGHTS];
};

cbuffer cbObject : register(b1)
{
    float4x4 vWorld; 
    float4x4 vTexTransform;
	uint iCustomProperty;
	
	float pad1obj;
	float pad2obj;
	float pad3obj;
};

TextureCube cubeMap : register(t0);

struct VertexIn
{
    float3 vPos   : POSITION;
    float3 vNormal: NORMAL;
    float2 vUV    : UV;
	float4 vCustomVec4 : CUSTOM;
};

struct VertexOut
{
    float4 vPosViewSpace  : SV_POSITION;
	float3 vPosLocalSpace : POSITION;
    float3 vNormal : NORMAL;
    float2 vUV     : UV;
	float4 vCustomVec4 : CUSTOM;
};


VertexOut VS(VertexIn vin)
{
    VertexOut vout;
	vout.vCustomVec4 = vin.vCustomVec4;

    // Use local vertex position as cubemap lookup vector.
    vout.vPosLocalSpace = vin.vPos;
	
    // Apply world matrix.
    float4 vPos = mul(float4(vin.vPos, 1.0f), vWorld);

    //vout.vNormal = mul(vin.vNormal, (float3x3)vWorld);

    // Always center sky about camera.
    vPos.xyz += vCameraPos.xyz;
	
    // Transform to homogeneous clip space.
    // Set z = w so that z/w = 1 (i.e., skydome always on far plane).
    vout.vPosViewSpace  = mul(vPos, vViewProj).xyww;

    //vout.vUV = vin.vUV;
    
    return vout;
}


float4 PS(VertexOut pin) : SV_Target
{
    if (iTextureFilterIndex == 0)
    {
        return cubeMap.Sample(samplerPointWrap, pin.vPosLocalSpace.xyz);
    }
    else if (iTextureFilterIndex == 1)
    {
        return cubeMap.Sample(samplerLinearWrap, pin.vPosLocalSpace.xyz);
    }
    else
    {
        return cubeMap.Sample(samplerAnisotropicWrap, pin.vPosLocalSpace.xyz);
    }
}

