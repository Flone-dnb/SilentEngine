// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "light_tools.hlsl"

cbuffer cbObject : register(b0)
{
    float4x4 vWorld; 
};

cbuffer cbMaterial : register(b1)
{
    float4 vDiffuseColor;
    // FresnelR0.
    float3 vSpecularColor;

    float fRoughness;

    float4x4 mMatTransform;
}

cbuffer cbPass : register(b2)
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
	
	float   pad2;
	
    Light    vLights[MAX_LIGHTS];
};

struct VertexIn
{
    float3 vPos   : POSITION;
    float3 vNormal: NORMAL;
};

struct VertexOut
{
    float4 vPosViewSpace  : SV_POSITION;
	float3 vPosWorldSpace : POSITION;
    float3 vNormal : NORMAL;
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
    
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    // Normals may be unnormalized after the rasterization (when they are interpolated).
    pin.vNormal = normalize(pin.vNormal);

    float3 vToCamera = normalize(vCameraPos - pin.vPosWorldSpace);

    float4 vAmbientDiffuseLight = vAmbientLight * vDiffuseColor;
    
    Material mat = {vDiffuseColor, vSpecularColor, fRoughness};
    float3 vShadowFactor = 1.0f;
	
    float4 vDirectLight = computeLightingToEye(vLights, iDirectionalLightCount, iPointLightCount, iSpotLightCount, mat, pin.vPosWorldSpace, pin.vNormal, vToCamera, vShadowFactor);

    float4 vLitColor = vDirectLight + vAmbientDiffuseLight;

    // Common convention to take alpha from diffuse material.
    vLitColor.a = vDiffuseColor.a;
	
    return vLitColor;
}

