// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "light_tools.hlsl"

Texture2D diffuseTexture : register(t0);

SamplerState samplerPointWrap       : register(s0);
SamplerState samplerLinearWrap      : register(s1);
SamplerState samplerAnisotropicWrap : register(s2);

cbuffer cbPass : register(b0)
{
    float4x4 vView;
    float4x4 vInvView;
    float4x4 vProj;
    float4x4 vInvProj;
    float4x4 vViewProj;
    float4x4 vInvViewProj;
	
    float3   vCameraPos;
	float    pad1;
	
    float2   vRenderTargetSize;
    float2   vInvRenderTargetSize;
	
    float    fNearZ;
    float    fFarZ;
    float    fTotalTime;
    float    fDeltaTime;

    float4   vAmbientLight;

	int iDirectionalLightCount;
	int iPointLightCount;
	int iSpotLightCount;
	
	int iTextureFilterIndex;

    float4 vFogColor;
	float  fFogStart;
	float  fFogRange;
	float2 pad2;
	
    Light    vLights[MAX_LIGHTS];
};

cbuffer cbObject : register(b1)
{
    float4x4 vWorld; 
    float4x4 vTexTransform;
};

cbuffer cbMaterial : register(b2)
{
    float4 vDiffuseColor;
    // FresnelR0.
    float3 vSpecularColor;

    float fRoughness;

    float4x4 vMatTransform;

    float fCustomTransparency;

    int bHasDiffuseTexture;
    int bHasNormalTexture;
}

struct VertexIn
{
    float3 vPos   : POSITION;
    float3 vNormal: NORMAL;
    float2 vUV    : UV;
};

struct VertexOut
{
    float4 vPosViewSpace  : SV_POSITION;
	float3 vPosWorldSpace : POSITION;
    float3 vNormal : NORMAL;
    float2 vUV     : UV;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
	
    // Apply world matrix.
    float4 vPos = mul(float4(vin.vPos, 1.0f), vWorld);

    vout.vNormal = mul(vin.vNormal, (float3x3)vWorld);


	vout.vPosWorldSpace = vPos;
	
    // Transform to homogeneous clip space.
    vout.vPosViewSpace  = mul(vPos, vViewProj);

    float4 vTexUV = mul(float4(vin.vUV, 0.0f, 1.0f), vTexTransform);
    vout.vUV = mul(vTexUV, vMatTransform).xy;
    
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    float4 vDiffuse = float4(1.0f, 1.0f, 1.0f, 1.0f);


    // Apply texture.

    if (bHasDiffuseTexture)
    {
        if (iTextureFilterIndex == 0)
        {
            vDiffuse = diffuseTexture.Sample(samplerPointWrap, pin.vUV);
        }
        else if (iTextureFilterIndex == 1)
        {
            vDiffuse = diffuseTexture.Sample(samplerLinearWrap, pin.vUV);
        }
        else
        {
            vDiffuse = diffuseTexture.Sample(samplerAnisotropicWrap, pin.vUV);
        }
    }

    vDiffuse *= vDiffuseColor;

#ifdef ALPHA_TEST // only for components with bEnableTransparency
	clip(vDiffuse.a - 0.02f);
#endif



    // Calculate lighting.

    // Normals may be unnormalized after the rasterization (when they are interpolated).
    pin.vNormal = normalize(pin.vNormal);

    float3 vToCamera = vCameraPos - pin.vPosWorldSpace;
    float3 vDistToCamera = length(vToCamera);
    vToCamera = normalize(vToCamera);

    float4 vAmbientDiffuseLight = vAmbientLight * vDiffuse;
    
    Material mat = {vDiffuse, vSpecularColor, fRoughness};
    float3 vShadowFactor = 1.0f;
	
    float4 vDirectLight = computeLightingToEye(vLights, iDirectionalLightCount, iPointLightCount, iSpotLightCount, mat, pin.vPosWorldSpace, pin.vNormal, vToCamera, vShadowFactor);

    float4 vLitColor = vDirectLight + vAmbientDiffuseLight;



    // Apply distant fog.

    float fFogAmount = saturate(((vDistToCamera - fFogStart) / fFogRange) * vFogColor.a);
	vLitColor = lerp(vLitColor, vFogColor, fFogAmount);



    // Common convention to take alpha from diffuse material.
    vLitColor.a = vDiffuse.a;
	
#ifdef ALPHA_TEST // only for components with bEnableTransparency
	vLitColor.a *= fCustomTransparency;
#endif
	
    
    return vLitColor;
}

