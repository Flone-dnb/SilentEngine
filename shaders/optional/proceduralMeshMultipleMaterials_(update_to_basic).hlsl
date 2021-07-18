// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "light_tools.hlsl"

//Texture2D diffuseTexture : register(t0);
Texture2D diffuseTexture[2] : register(t0);

Texture2D shadowMaps[MAX_LIGHTS * 6] : register(t0, space2);

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

struct MaterialData
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

StructuredBuffer<MaterialData> materialData : register(t0, space1);



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
	float4 vPosWorldSpace : POSITION;
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


	vout.vPosWorldSpace = vPos;
	
    // Transform to homogeneous clip space.
    vout.vPosViewSpace  = mul(vPos, vViewProj);

    float4 vTexUV = mul(float4(vin.vUV, 0.0f, 1.0f), vTexTransform);
    vout.vUV = vTexUV.xy;
    
    return vout;
}



float calcShadowFactor(float4 vPosWorldSpace, int iLightIndex, int iShadowMapIndex)
{
    float4 vPosShadowMapTexSpace = mul(vPosWorldSpace, vLights[iLightIndex].mLightViewProjTex);

    // Complete projection by doing division by w.
    vPosShadowMapTexSpace.xyz /= vPosShadowMapTexSpace.w;

    // Depth in NDC space.
    float fCurrentPixelDepth = vPosShadowMapTexSpace.z;

    uint iShadowMapWidth, iShadowMapHeight, numMips;
    shadowMaps[iShadowMapIndex].GetDimensions(0, iShadowMapWidth, iShadowMapHeight, numMips);

    // Texel size.
    float dx = 1.0f / (float)iShadowMapWidth;

    float fPercentLit = 0.0f;
    const float2 offsets[9] =
    {
        float2(-dx,  -dx), float2(0.0f,  -dx), float2(dx,  -dx),
        float2(-dx, 0.0f), float2(0.0f, 0.0f), float2(dx, 0.0f),
        float2(-dx,  +dx), float2(0.0f,  +dx), float2(dx,  +dx)
    };

    // compare depth with value in shadow map
    [unroll]
    for(int i = 0; i < 9; i++)
    {
        fPercentLit += shadowMaps[iShadowMapIndex].SampleCmpLevelZero(samplerShadowMap,
        vPosShadowMapTexSpace.xy + offsets[i], fCurrentPixelDepth).r;
    }
    
    return fPercentLit / 9.0f;
}


float4 PS(VertexOut pin) : SV_Target
{
	// All VertexOut params are linearly interpolated now.
	
	MaterialData smoothMaterial;
	
	smoothMaterial.bHasDiffuseTexture = materialData[0].bHasDiffuseTexture;
	smoothMaterial.bHasNormalTexture = materialData[0].bHasNormalTexture;
	

    float fMergeCoef = 0.0f;

    int iMaterialCount = 2;

	for (int i = 0; i < iMaterialCount - 1; i++)
	{
        fMergeCoef = pin.vCustomVec4.x - i;

        if (fMergeCoef <= 1.0f)
        {
            smoothMaterial.vDiffuseColor     = lerp(materialData[i].vDiffuseColor, materialData[i + 1].vDiffuseColor, fMergeCoef);
		    smoothMaterial.vSpecularColor    = lerp(materialData[i].vSpecularColor, materialData[i + 1].vSpecularColor, fMergeCoef);
		    smoothMaterial.fRoughness        = lerp(materialData[i].fRoughness, materialData[i + 1].fRoughness, fMergeCoef);
		    smoothMaterial.vMatTransform     = lerp(materialData[i].vMatTransform, materialData[i + 1].vMatTransform, fMergeCoef);
		    smoothMaterial.vFinalDiffuseMult = lerp(materialData[i].vFinalDiffuseMult, materialData[i + 1].vFinalDiffuseMult, fMergeCoef);
		    smoothMaterial.fCustomTransparency = lerp(materialData[i].fCustomTransparency, materialData[i + 1].fCustomTransparency, fMergeCoef);

            float4x4 vMatTrans;

            for (int k = 0; k < 4; k++)
            {
                vMatTrans[k] = lerp(materialData[i].vMatTransform[k], materialData[i + 1].vMatTransform[k], fMergeCoef);
            }

            pin.vUV = mul(float4(pin.vUV, 0.0f, 1.0f), vMatTrans).xy;

            break;
        }
	}
	

    float4 vDiffuse = float4(1.0f, 1.0f, 1.0f, 1.0f);


    // Apply texture.

    if (materialData[0].bHasDiffuseTexture)
    {
        for (int i = 0; i < iMaterialCount - 1; i++)
	    {
            fMergeCoef = pin.vCustomVec4.x - i;

            if (fMergeCoef <= 1.0f)
            {
                if (iTextureFilterIndex == 0)
                {
                    smoothMaterial.vDiffuseColor = lerp(diffuseTexture[i].Sample(samplerPointWrap, pin.vUV), diffuseTexture[i + 1].Sample(samplerPointWrap, pin.vUV), fMergeCoef);
                }
                else if (iTextureFilterIndex == 1)
                {
                    smoothMaterial.vDiffuseColor = lerp(diffuseTexture[i].Sample(samplerLinearWrap, pin.vUV), diffuseTexture[i + 1].Sample(samplerLinearWrap, pin.vUV), fMergeCoef);
                }
                else
                {
                    smoothMaterial.vDiffuseColor = lerp(diffuseTexture[i].Sample(samplerAnisotropicWrap, pin.vUV), diffuseTexture[i + 1].Sample(samplerAnisotropicWrap, pin.vUV), fMergeCoef);
                }

                break;
            }
	    }
    }
	
    vDiffuse *= smoothMaterial.vDiffuseColor;

#ifdef ALPHA_TEST // only for components with bEnableTransparency
	clip(vDiffuse.a - 0.02f);
#endif

    vDiffuse *= 1.0f / fGamma;



    // Calculate lighting.

    // Normals may be unnormalized after the rasterization (when they are interpolated).
    pin.vNormal = normalize(pin.vNormal);

    float3 vToCamera = vCameraPos - pin.vPosWorldSpace.xyz;
    float3 vDistToCamera = length(vToCamera);
    vToCamera = normalize(vToCamera);

    float4 vAmbientDiffuseLight = vAmbientLight * vDiffuse;
    
    Material mat = {vDiffuse, smoothMaterial.vSpecularColor, smoothMaterial.fRoughness};
    

    // Calc shadow factor for all lights.
    float vLightShadowFactors[MAX_LIGHTS];
    int iLightIndex = 0;
    int iShadowMapIndex = 0;
    for (; iLightIndex < iDirectionalLightCount; iLightIndex++)
    {
        vLightShadowFactors[iLightIndex] = calcShadowFactor(pin.vPosWorldSpace, iShadowMapIndex, vLights[iLightIndex].mLightViewProjTex[0]);
        iShadowMapIndex++;
    }
	
    for (; iLightIndex < iDirectionalLightCount + iPointLightCount; iLightIndex++)
    {
        float vShadowFactors[6];

        [unroll]
        for (int m = 0; m < 6; m++)
        {
            float4 vPosShadowMapTexSpace = mul(pin.vPosWorldSpace, vLights[iLightIndex].mLightViewProjTex[m]);

            // Complete projection by doing division by w.
            vPosShadowMapTexSpace.xyz /= vPosShadowMapTexSpace.w;

            if (vPosShadowMapTexSpace.x < FLOAT_DELTA || vPosShadowMapTexSpace.x > (1.0f - FLOAT_DELTA)
            || vPosShadowMapTexSpace.y < FLOAT_DELTA || vPosShadowMapTexSpace.y > (1.0f - FLOAT_DELTA)
            || vPosShadowMapTexSpace.z < FLOAT_DELTA || vPosShadowMapTexSpace.z > (1.0f - FLOAT_DELTA))
            {
                vShadowFactors[m] = 1.0f;
                continue;
            }

            vShadowFactors[m] = calcShadowFactor(pin.vPosWorldSpace, iShadowMapIndex + m, vLights[iLightIndex].mLightViewProjTex[m]);
        }

        float fMinValue = vShadowFactors[0];

        [unroll]
        for (int i = 1; i < 6; i++)
        {
            if (vShadowFactors[i] < fMinValue)
            {
                fMinValue = vShadowFactors[i];
            }
        }

        vLightShadowFactors[iLightIndex] = fMinValue;

        iShadowMapIndex += 6;
    }
	
    for (; iLightIndex < iDirectionalLightCount + iPointLightCount + iSpotLightCount; iLightIndex++)
    {
        vLightShadowFactors[iLightIndex] = calcShadowFactor(pin.vPosWorldSpace, iShadowMapIndex, vLights[iLightIndex].mLightViewProjTex[0]);
        iShadowMapIndex++;
    }
	
    float4 vDirectLight = computeLightingToEye(vLights, vLightShadowFactors, iDirectionalLightCount,
    iPointLightCount, iSpotLightCount,
    mat, pin.vPosWorldSpace.xyz,
    pin.vNormal, vToCamera);

    float4 vLitColor = vDirectLight + vAmbientDiffuseLight;



    // Apply distant fog.

    float fFogAmount = saturate(((vDistToCamera - fFogStart) / fFogRange));
	vLitColor = lerp(vLitColor, vFogColor, fFogAmount);



    // Common convention to take alpha from diffuse material.
    vLitColor.a = vDiffuse.a;
	
#ifdef ALPHA_TEST // only for components with bEnableTransparency
	vLitColor.a *= smoothMaterial.fCustomTransparency;
#endif
	
    // Apply vFinalDiffuseMult.
    vLitColor += (vDiffuse * smoothMaterial.vFinalDiffuseMult);

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

