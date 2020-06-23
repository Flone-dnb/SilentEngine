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

/*
* remarks: every SComponentType::SCT_MESH, SCT_RUNTIME_MESH component has this.
*/
struct SObjectConstants
{
	DirectX::XMFLOAT4X4 vWorld = SMath::getIdentityMatrix4x4();
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

	DirectX::XMFLOAT2  vRenderTargetSize    = { 0.0f, 0.0f };
	DirectX::XMFLOAT2  vInvRenderTargetSize = { 0.0f, 0.0f };

	float fcbPerObjectPad1 = 0.0f; // this line may change later

	float fNearZ           = 0.0f;
	float fFarZ            = 0.0f;

	float fTotalTime       = 0.0f;
	float fDeltaTime       = 0.0f;
};

class SFrameResource
{
public:

	SFrameResource(ID3D12Device* pDevice, UINT iPassCount, UINT iObjectCount);
	SFrameResource(const SFrameResource&) = delete;
	SFrameResource& operator = (const SFrameResource&) = delete;
	~SFrameResource();

	// returns new cb start index
	size_t addNewObjectCB(size_t iNewCBCount, bool* pbCBWasExpanded);
	void   removeObjectCB(size_t iCBStartIndex, size_t iCBCount, bool* pbCBWasResized);

	// returns index of new buffer in vRuntimeMeshVertexBuffers
	size_t addRuntimeMeshVertexBuffer(size_t iVertexCount);
	void   removeRuntimeMeshVertexBuffer(size_t iVertexBufferIndex);
	void   recreateRuntimeMeshVertexBuffer(size_t iVertexBufferIndex, size_t iNewVertexCount);


	// -------------------------------------------------------------------------


	Microsoft::WRL::ComPtr<ID3D12CommandAllocator>   pCommandListAllocator;

	std::unique_ptr<SUploadBuffer<SRenderPassConstants>> pRenderPassCB = nullptr;
	std::unique_ptr<SUploadBuffer<SObjectConstants>>     pObjectsCB    = nullptr;
	std::vector<std::unique_ptr<SUploadBuffer<SVertex>>> vRuntimeMeshVertexBuffers;

	ID3D12Device* pDevice = nullptr;

	UINT64 iFence = 0;

private:

	UINT roundUp(UINT iNum, UINT iMultiple);
	void createBuffers(UINT iRenderPassCBCount, UINT iObjectCBCount);


	size_t iObjectsCBActualElementCount = 0;
	size_t iRenderPassCBCount = 0;
	int    iObjectCBResizeMultiple = OBJECT_CB_RESIZE_MULTIPLE;
};

