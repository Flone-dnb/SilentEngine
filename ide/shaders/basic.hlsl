// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

cbuffer cbPerObject : register(b0)
{
	float4x4 vWorld; 
};

cbuffer cbPerPass : register(b1)
{
    float4x4 vView;
    float4x4 vInvView;
    float4x4 vProj;
    float4x4 vInvProj;
    float4x4 vViewProj;
    float4x4 vInvViewProj;
	
    float3   vEyePosW;
	
    float2   vRenderTargetSize;
    float2   vInvRenderTargetSize;
	
	float    cbPerObjectPad1; // this line may change later
	
    float    fNearZ;
    float    fFarZ;
    float    fTotalTime;
    float    fDeltaTime;
};

struct VertexIn
{
	float3 vPos   : POSITION;
    float4 vColor : COLOR;
};

struct VertexOut
{
	float4 vPos   : SV_POSITION;
    float4 vColor : COLOR;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
	// Transform to homogeneous clip space.
    float4 vPos = mul(float4(vin.vPos, 1.0f), vWorld);
    vout.vPos   = mul(vPos, vViewProj);
    
    vout.vColor = vin.vColor;
    
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    return pin.vColor;
}

