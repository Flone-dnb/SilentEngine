#define MAX_LIGHTS 16
#define FLOAT_DELTA 0.0000001f

struct Light
{
    float3 vColor;
    float  fFalloffStart; // point/spot light only
    float3 vDirection;    // directional/spot light only
    float  fFalloffEnd;   // point/spot light only
    float3 vPosition;     // point light only
    float  fSpotlightRange; // spot light only
	float4x4 mLightViewProjTex[6];
};

struct Material
{
    float4 vDiffuseColor;
    // FresnelR0.
    float3 vSpecularColor;
    float  fRoughness;
};

float calcLinearAttenuation(float fDistance, float fFalloffStart, float fFalloffEnd)
{
    // Linear falloff.

    return saturate( (fFalloffEnd - fDistance) / (fFalloffEnd - fFalloffStart) );
}

float3 reflectSpecularLight(float3 vSpecularColor, float3 vSurfaceNormal, float3 vLightVec)
{
    // Schlick approximation to Fresnel effect.

    float fCosIncidentLight = saturate(dot(vSurfaceNormal, vLightVec));

    float fOneMinusCos = 1.0f - fCosIncidentLight;

    float3 fReflectPercent = vSpecularColor + (1.0f - vSpecularColor) * (fOneMinusCos * fOneMinusCos * fOneMinusCos * fOneMinusCos * fOneMinusCos);

    return fReflectPercent;
}

float3 calcReflectedDiffuseAndSpecularLightToEye(float3 vLightColor, float3 vToLightSourceVec, float3 vSurfaceNormal, float3 vToEyeVec, Material mat)
{
    // Blinn–Phong reflection model.

    float m = (1.0f - mat.fRoughness) * 256.0f;
    float3 vHalfWayVec = normalize(vToEyeVec + vToLightSourceVec);

    float fRoughnessFactor = (m + 8.0f) * pow(max( dot(vHalfWayVec, vSurfaceNormal), 0.0f ), m) / 8.0f;
    float3 vFresnelFactor  = reflectSpecularLight(mat.vSpecularColor, vHalfWayVec, vToLightSourceVec);

    // Combine roughness with reflected specular light.
    float3 vSpecularAlbedo = vFresnelFactor * fRoughnessFactor;

    // Our spec formula goes outside [0.0; 1.0] range, but we are 
    // doing LDR rendering. So scale it down a bit.
    vSpecularAlbedo = vSpecularAlbedo / (vSpecularAlbedo + 1.0f);

    return (mat.vDiffuseColor.rgb + vSpecularAlbedo) * vLightColor;
}

float3 computeDirectionalLightReflectedToEye(Light light, Material mat, float3 vSurfaceNormal, float3 vToEyeVec)
{
    float3 vToLightSourceVec = -light.vDirection;

    // Scale light down by Lambert's cosine law.
    // (consider the angle of incidence of the light)
    float fCosIncidentLight = max( dot(vToLightSourceVec, vSurfaceNormal), 0.0f );
    float3 vLightColorStrength = light.vColor * fCosIncidentLight;

    return calcReflectedDiffuseAndSpecularLightToEye(vLightColorStrength, vToLightSourceVec, vSurfaceNormal, vToEyeVec, mat);
}

float3 computePointLightReflectedToEye(Light light, Material mat, float3 vObjectPosition, float3 vSurfaceNormal, float3 vToEyeVec)
{
    float3 vToLightSourceVec = light.vPosition - vObjectPosition;

    float fFromLightDistance = length(vToLightSourceVec);

    // Are we outside of the lighting range?
    if (fFromLightDistance > light.fFalloffEnd)
    {
        return 0.0f;
    }

    vToLightSourceVec = normalize(vToLightSourceVec);

    // Scale light down by Lambert's cosine law.
    // (consider the angle of incidence of the light)
    float fCosIncidentLight = max( dot(vToLightSourceVec, vSurfaceNormal), 0.0f );
    float3 vLightColorStrength = light.vColor * fCosIncidentLight;

    // Consider the point light attenuation.
    float fAttenuationMult = calcLinearAttenuation(fFromLightDistance, light.fFalloffStart, light.fFalloffEnd);
    vLightColorStrength *= fAttenuationMult;

    return calcReflectedDiffuseAndSpecularLightToEye(vLightColorStrength, vToLightSourceVec, vSurfaceNormal, vToEyeVec, mat);
}

float3 computeSpotLightReflectedToEye(Light light, Material mat, float3 vObjectPosition, float3 vSurfaceNormal, float3 vToEyeVec)
{
    float3 vToLightSourceVec = light.vPosition - vObjectPosition;

    float fFromLightDistance = length(vToLightSourceVec);

    // Are we outside of the lighting range?
    if (fFromLightDistance > light.fFalloffEnd)
    {
        return 0.0f;
    }

    vToLightSourceVec = normalize(vToLightSourceVec);

    // Scale light down by Lambert's cosine law.
    // (consider the angle of incidence of the light)
    float fCosIncidentLight = max( dot(vToLightSourceVec, vSurfaceNormal), 0.0f );
    float3 vLightColorStrength = light.vColor * fCosIncidentLight;

    // Consider the spot light attenuation.
    float fAttenuationMult = calcLinearAttenuation(fFromLightDistance, light.fFalloffStart, light.fFalloffEnd);
    vLightColorStrength *= fAttenuationMult;

    // Scale by spotlight range.
    float fLightCosAngle = max(dot(-vToLightSourceVec, light.vDirection), 0.0f);
    float fSpotlightFactor = pow( fLightCosAngle, light.fSpotlightRange );
    vLightColorStrength *= fSpotlightFactor;

    return calcReflectedDiffuseAndSpecularLightToEye(vLightColorStrength, vToLightSourceVec, vSurfaceNormal, vToEyeVec, mat);
}

float4 computeLightingToEye(Light vLights[MAX_LIGHTS], float vLightShadowFactors[MAX_LIGHTS], int iDirectionalLightCount, int iPointLightCount, int iSpotLightCount,
Material mat, float3 vObjectPosition, float3 vSurfaceNormal, float3 vToEyeVec)
{
    float3 vResult = 0.0f;
	
	int i = 0;
	
	for (; i < iDirectionalLightCount; i++)
	{
		vResult += vLightShadowFactors[i] * computeDirectionalLightReflectedToEye(vLights[i], mat, vSurfaceNormal, vToEyeVec);
	}
	
	for (; i < iDirectionalLightCount + iPointLightCount; i++)
	{
		vResult += vLightShadowFactors[i] * computePointLightReflectedToEye(vLights[i], mat, vObjectPosition, vSurfaceNormal, vToEyeVec);
	}
	
	for (; i < iDirectionalLightCount + iPointLightCount + iSpotLightCount; i++)
	{
		vResult += vLightShadowFactors[i] * computeSpotLightReflectedToEye(vLights[i], mat, vObjectPosition, vSurfaceNormal, vToEyeVec);
	}

    return float4(vResult, 0.0f);
}