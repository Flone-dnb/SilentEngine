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

cbuffer cbMaterial : register(b2)
{
    float4 vDiffuseColor;
    // FresnelR0.
    float3 vSpecularColor;

    float fRoughness;

    float4x4 vMatTransform;

    float4 vFinalDiffuseMult;

    float fCustomTransparency;

    int bHasDiffuseTexture;
    int bHasNormalTexture;
	
	int pad1mat;
};

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
	float3 vPosWorldSpace : POSITION;
    float3 vNormal : NORMAL;
    float2 vUV     : UV;
	float4 vCustomVec4 : CUSTOM;
};


VertexOut VS(VertexIn vin)
{
    VertexOut vout;
	vout.vCustomVec4 = vin.vCustomVec4;
	
    // Apply world matrix.
    float4 vPos = mul(float4(vin.vPos, 1.0f), vWorld);

    vout.vNormal = mul(vin.vNormal, (float3x3)vWorld);


	vout.vPosWorldSpace = vPos.xyz;
	
    // Transform to homogeneous clip space.
    vout.vPosViewSpace  = mul(vPos, vViewProj);

    float4 vTexUV = mul(float4(vin.vUV, 0.0f, 1.0f), vTexTransform);
    vout.vUV = mul(vTexUV, vMatTransform).xy;
    
    return vout;
}


float4 PS(VertexOut pin) : SV_Target
{
	// All VertexOut params are linearly interpolated now.
	
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

    vDiffuse *= 1.0f / fGamma;



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

    float fFogAmount = saturate(((vDistToCamera - fFogStart) / fFogRange)).x;
	vLitColor = lerp(vLitColor, vFogColor, fFogAmount);



    // Common convention to take alpha from diffuse material.
    vLitColor.a = vDiffuse.a;
	
#ifdef ALPHA_TEST // only for components with bEnableTransparency
	vLitColor.a *= fCustomTransparency;
#endif
	
    // Apply vFinalDiffuseMult.
    vLitColor += (vDiffuse * vFinalDiffuseMult);

    // Apply vCameraMultiplyColor.
    vLitColor *= float4(vCameraMultiplyColor, 1.0f);

    // Apply saturation.
    float fLuminance = 0.2126f * vLitColor.r + 0.7152f * vLitColor.g + 0.0722f * vLitColor.b;
    float3 vColorDiff = vLitColor.xyz - float3(fLuminance, fLuminance, fLuminance);

    vColorDiff *= fSaturation;

    float3 vSaturatedColor = clamp(float3(fLuminance, fLuminance, fLuminance) + vColorDiff, 0.0f, 1.0f);

    vLitColor.rgb = lerp(vLitColor.rgb, vSaturatedColor, 0.5f);



    // Apply gamma.
    vLitColor = pow(vLitColor, fGamma);

    vLitColor = saturate(vLitColor);
    
    return vLitColor;
}

