// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#pragma once

// STL
#include <memory>
#include <vector>

// DirectX
#include <d3d12.h>
#include <wrl.h> // smart pointers
#include <DirectXMath.h>
#include <DirectXPackedVector.h>

// Custom
#include "SilentEngine/Private/SUploadBuffer/SUploadBuffer.h"
#include "SilentEngine/Private/SGeometry/SGeometry.h"
#include "SilentEngine/Public/SPrimitiveShapeGenerator/SPrimitiveShapeGenerator.h"
#include "SilentEngine/Private/EntityComponentSystem/SLightComponent/SLightComponent.h"
#include "SilentEngine/Public/SVector/SVector.h"

/*
* remarks: per-object constant buffer data. Every SComponentType::SCT_MESH, SCT_RUNTIME_MESH component has this.
*/
struct SObjectConstants
{
	DirectX::XMFLOAT4X4 vWorld = SMath::getIdentityMatrix4x4();
	DirectX::XMFLOAT4X4 vTexTransform = SMath::getIdentityMatrix4x4();
};

struct SMaterialConstants
{
	// Diffuse color.
	DirectX::XMFLOAT4 vDiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	// Specular color.
	DirectX::XMFLOAT3 vFresnelR0 = { 0.01f, 0.01f, 0.01f };

	float fRoughness = 0.5f;

	DirectX::XMFLOAT4X4 vMatTransform = SMath::getIdentityMatrix4x4();

	float fCustomTransparency = 1.0f;

	int bHasDiffuseTexture = 0;
	int bHasNormalTexture = 0;
};

enum TEX_FILTER_MODE
{
	TFM_POINT = 0,
	TFM_LINEAR = 1,
	TFM_ANISOTROPIC = 2
};

struct SRenderPassConstants
{
	DirectX::XMFLOAT4X4 vView               = SMath::getIdentityMatrix4x4();
	DirectX::XMFLOAT4X4 vInvView            = SMath::getIdentityMatrix4x4();
	DirectX::XMFLOAT4X4 vProj               = SMath::getIdentityMatrix4x4();
	DirectX::XMFLOAT4X4 vInvProj            = SMath::getIdentityMatrix4x4();
	DirectX::XMFLOAT4X4 vViewProj           = SMath::getIdentityMatrix4x4();
	DirectX::XMFLOAT4X4 vInvViewProj        = SMath::getIdentityMatrix4x4();

	DirectX::XMFLOAT3   vCameraPos          = { 0.0f, 0.0f, 0.0f };
	float pad1;

	DirectX::XMFLOAT2  vRenderTargetSize    = { 0.0f, 0.0f };
	DirectX::XMFLOAT2  vInvRenderTargetSize = { 0.0f, 0.0f };

	float fNearZ           = 0.0f;
	float fFarZ            = 0.0f;

	float fTotalTime       = 0.0f;
	float fDeltaTime       = 0.0f;

	DirectX::XMFLOAT4  vAmbientLightRGBA = { 0.0f, 0.0f, 0.0f, 1.0f };

	int iDirectionalLightCount = 0;
	int iPointLightCount = 0;
	int iSpotLightCount = 0;

	int iTextureFilterIndex = TFM_ANISOTROPIC;

	DirectX::XMFLOAT4 vFogColor = { 0.5f, 0.5f, 0.5f, 1.0f };
	float fFogStart = fFarZ / 2;
	float fFogRange = fFogStart;
	DirectX::XMFLOAT2 pad2;

	SLightProps lights[MAX_LIGHTS];
};

// Stuff from SRenderPassConstants that user can change.
struct SGlobalVisualSettings
{
	// Default: 0.0f, 0.0f, 0.0f.
	SVector vAmbientLightRGB = SVector(0.0f, 0.0f, 0.0f);

	// Default: 0.0f, 0.0f, 0.0f, 0.0f.
	SVector vDistantFogColorRGBA = SVector(0.0f, 0.0f, 0.0f, 0.0f);

	// Default: 1000.0f.
    // (fog start distance from camera)
	float fDistantFogStart = 1000.0f;

	// Default: 500.0f
	// (fog length from fDistantFogStart)
	float fDistantFogRange = 500.0f;
};

class SFrameResource
{
public:

	SFrameResource(ID3D12Device* pDevice, UINT iPassCount, UINT iObjectCount);

	SFrameResource(const SFrameResource&) = delete;
	SFrameResource& operator = (const SFrameResource&) = delete;

	~SFrameResource();


	// returns new cb start index
	size_t addNewObjectCB       (size_t iNewCBCount, bool* pbCBWasExpanded);
	void   removeObjectCB       (size_t iCBStartIndex, size_t iCBCount, bool* pbCBWasResized);

	// returns new cb start index
	size_t addNewMaterialCB     (bool* pbCBWasExpanded);
	void   removeMaterialCB     (size_t iCBIndex, bool* pbCBWasResized);

	// returns index of new buffer in vRuntimeMeshVertexBuffers
	size_t addRuntimeMeshVertexBuffer      (size_t iVertexCount);
	void   removeRuntimeMeshVertexBuffer   (size_t iVertexBufferIndex);
	void   recreateRuntimeMeshVertexBuffer (size_t iVertexBufferIndex, size_t iNewVertexCount);


	// -------------------------------------------------------------------------


	Microsoft::WRL::ComPtr<ID3D12CommandAllocator>   pCommandListAllocator;

	std::unique_ptr<SUploadBuffer<SRenderPassConstants>> pRenderPassCB = nullptr;
	std::unique_ptr<SUploadBuffer<SObjectConstants>>     pObjectsCB    = nullptr;
	std::unique_ptr<SUploadBuffer<SMaterialConstants>>   pMaterialCB   = nullptr;
	std::vector<std::unique_ptr<SUploadBuffer<SVertex>>> vRuntimeMeshVertexBuffers;

	ID3D12Device* pDevice = nullptr;

	UINT64 iFence = 0;

private:

	UINT roundUp                   (UINT iNum, UINT iMultiple);
	void createRenderObjectBuffers (UINT iRenderPassCBCount, UINT iObjectCBCount);
	void createMaterialBuffer      (UINT iMaterialCBCount);


	size_t iObjectsCBActualElementCount = 0;
	size_t iMaterialCBActualElementCount = 0;
	size_t iRenderPassCBCount = 0;
	int    iCBResizeMultiple = OBJECT_CB_RESIZE_MULTIPLE;
};

