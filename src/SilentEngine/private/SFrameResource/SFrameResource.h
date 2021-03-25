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
#include "SilentEngine/Private/SRenderItem/SRenderItem.h"
#include "SilentEngine/Public/SPrimitiveShapeGenerator/SPrimitiveShapeGenerator.h"
#include "SilentEngine/Private/EntityComponentSystem/SLightComponent/SLightComponent.h"
#include "SilentEngine/Public/SVector/SVector.h"

/*
* remarks: per-object constant buffer data. Every mesh component has this.
*/
struct SObjectConstants
{
	DirectX::XMFLOAT4X4 vWorld = SMath::getIdentityMatrix4x4();
	DirectX::XMFLOAT4X4 vTexTransform = SMath::getIdentityMatrix4x4();
	unsigned int iCustomProperty = 0;

	// update SMeshComponent::convertInstancePropsToConstants() if this struct changed

	float pad1 = 0.0f;
	float pad2 = 0.0f;
	float pad3 = 0.0f;
};

struct SMaterialConstants
{
	// Diffuse color.
	DirectX::XMFLOAT4 vDiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	// Specular color.
	DirectX::XMFLOAT3 vFresnelR0 = { 0.01f, 0.01f, 0.01f };

	float fRoughness = 0.5f;

	DirectX::XMFLOAT4X4 vMatTransform = SMath::getIdentityMatrix4x4();

	DirectX::XMFLOAT4 vFinalDiffuseMult = { 0.0f, 0.0f, 0.0f, 0.0f };

	float fCustomTransparency = 1.0f;

	int bHasDiffuseTexture = 0;
	int bHasNormalTexture = 0;

	int pad1 = 0;
};

enum class TEX_FILTER_MODE
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

	float fSaturation = 1.0f;

	DirectX::XMFLOAT2  vRenderTargetSize    = { 0.0f, 0.0f };
	DirectX::XMFLOAT2  vInvRenderTargetSize = { 0.0f, 0.0f };

	float fNearZ           = 0.0f;
	float fFarZ            = 0.0f;

	float fTotalTime       = 0.0f;
	float fDeltaTime       = 0.0f;

	DirectX::XMFLOAT4  vAmbientLightRGBA = { 0.0f, 0.0f, 0.0f, 1.0f };
	DirectX::XMFLOAT3  vCameraMultiplyColor = { 1.0f, 1.0f, 1.0f };

	float fGamma = 1.0f;

	int iDirectionalLightCount = 0;
	int iPointLightCount = 0;
	int iSpotLightCount = 0;

	int iTextureFilterIndex = static_cast<std::underlying_type<TEX_FILTER_MODE>::type>(TEX_FILTER_MODE::TFM_ANISOTROPIC);

	DirectX::XMFLOAT4 vFogColor = { 0.5f, 0.5f, 0.5f, 1.0f };
	float fFogStart = fFarZ / 2;
	float fFogRange = fFogStart;

	int iMainWindowWidth = 800;
	int iMainWindowHeight = 600;

	SLightProps lights[MAX_LIGHTS];
};

struct SDistantFog
{
	// Default: 0.0f, 0.0f, 0.0f, 0.0f.
	// (color of the fog)
	SVector vDistantFogColorRGBA = SVector(0.0f, 0.0f, 0.0f, 0.0f);

	// Default: 1000.0f (no fog).
	// (fog start distance from camera)
	float fDistantFogStart = 1000.0f;

	// Default: 500.0f
	// (fog length from fDistantFogStart)
	float fDistantFogRange = 500.0f;
};

// Stuff from SRenderPassConstants that user can change.
struct SGlobalVisualSettings
{
	// Default: 0.0f, 0.0f, 0.0f.
	// (constant light that affects every object - ambient light)
	SVector vAmbientLightRGB = SVector(0.0f, 0.0f, 0.0f);

	// (use to control the distant fog)
	SDistantFog distantFog;
};

class SShader;

struct SMaterialBundle
{
	SMaterialBundle(SShader* pShader, ID3D12Device* pDevice, UINT64 iElementCount, bool bIsCBUFFER)
	{
		pResource = std::make_unique<SUploadBuffer<SMaterialConstants>>(pDevice, iElementCount, bIsCBUFFER);
		this->pShaderUsingThisResource = pShader;
	}

	std::unique_ptr<SUploadBuffer<SMaterialConstants>> pResource;

	SShader* pShaderUsingThisResource;
};

class SFrameResource
{
public:

	SFrameResource(ID3D12Device* pDevice, UINT iObjectCount);

	SFrameResource(const SFrameResource&) = delete;
	SFrameResource& operator = (const SFrameResource&) = delete;

	~SFrameResource();


	// returns new cb start index
	UINT64 addNewObjectCB       (UINT64 iNewCBCount, bool* pbCBWasExpanded);
	void   removeObjectCB       (UINT64 iCBStartIndex, UINT64 iCBCount, bool* pbCBWasResized);


	// returns new cb start index
	size_t addNewMaterialCB     (bool* pbCBWasExpanded);
	void   removeMaterialCB     (UINT64 iCBIndex, bool* pbCBWasResized);


	// returns the pointer to the created bundle
	SUploadBuffer<SMaterialConstants>* addNewMaterialBundleResource(SShader* pShader, size_t iResourceCount);
	void                               removeMaterialBundle(SShader* pShader);


	// instanced data
	SUploadBuffer<SObjectConstants>*   addNewInstancedMesh(std::vector<SObjectConstants>* pInitData);
	// returns the pointer to the new buffer with +1 instance
	SUploadBuffer<SObjectConstants>*   addNewInstanceToMesh(SUploadBuffer<SObjectConstants>* pInstancedData, const SObjectConstants& newInstanceData);
	void                               removeInstancedMesh(SUploadBuffer<SObjectConstants>* pInstancedDataToDelete);


	// returns index of new buffer in vRuntimeMeshVertexBuffers
	size_t addRuntimeMeshVertexBuffer      (size_t iVertexCount);
	void   removeRuntimeMeshVertexBuffer   (size_t iVertexBufferIndex);
	void   recreateRuntimeMeshVertexBuffer (size_t iVertexBufferIndex, size_t iNewVertexCount);


	// -------------------------------------------------------------------------


	Microsoft::WRL::ComPtr<ID3D12CommandAllocator>   pCommandListAllocator;

	// We cannot update a buffer until the GPU is done processing the commands that reference it. So each frame needs their own buffers.
	std::unique_ptr<SUploadBuffer<SRenderPassConstants>> pRenderPassCB = nullptr;
	std::unique_ptr<SUploadBuffer<SObjectConstants>>     pObjectsCB    = nullptr;
	std::unique_ptr<SUploadBuffer<SMaterialConstants>>   pMaterialCB   = nullptr;
	std::vector<std::unique_ptr<SMaterialBundle>>        vMaterialBundles;
	std::vector<std::unique_ptr<SUploadBuffer<SObjectConstants>>> vInstancedMeshes;
	std::vector<std::unique_ptr<SUploadBuffer<SVertex>>> vRuntimeMeshVertexBuffers;

	ID3D12Device* pDevice = nullptr;

	UINT64 iFence = 0;

private:

	size_t roundUp                 (size_t iNum, size_t iMultiple);
	void createRenderObjectBuffers (UINT64 iObjectCBCount);
	void createMaterialBuffer      (UINT64 iMaterialCBCount);


	UINT64 iObjectsCBActualElementCount = 0;
	UINT64 iMaterialCBActualElementCount = 0;
	UINT64 iRenderPassCBCount = 1;
	UINT64  iCBResizeMultiple = OBJECT_CB_RESIZE_MULTIPLE;
};

