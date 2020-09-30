// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************


#pragma once

// STL
#include <vector>
#include <string>

// DirectX
#include <wrl.h> // smart pointers
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXCollision.h>
#include <d3d12.h>
#include "SilentEngine/Private/d3dx12.h"
#include <DirectXColors.h>

// Custom
#include "SilentEngine/Private/SMath/SMath.h"


#pragma comment(lib, "D3D12.lib")


#define SFRAME_RES_COUNT 3
#define OBJECT_CB_RESIZE_MULTIPLE 20


struct SMeshGeometry
{
	// System memory copies. Use Blobs because the vertex/index format can be generic.
	// It is up to the client to cast appropriately.
	Microsoft::WRL::ComPtr<ID3DBlob> pVertexBufferCPU = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pIndexBufferCPU  = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> pVertexBufferGPU = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> pIndexBufferGPU = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> pVertexBufferUploader = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> pIndexBufferUploader = nullptr;


	// Data about the buffers.
	size_t iVertexGraphicsObjectSizeInBytes = 0;
	size_t iVertexBufferSizeInBytes = 0;
	size_t iIndexBufferSizeInBytes = 0;
	DXGI_FORMAT indexFormat = DXGI_FORMAT_R16_UINT;

	DirectX::BoundingBox bounds;


	D3D12_VERTEX_BUFFER_VIEW getVertexBufferView() const;
	D3D12_INDEX_BUFFER_VIEW  getIndexBufferView()  const;


	// We can free this memory after we finish upload to the GPU.
	void freeUploaders();
};

struct SRenderItem
{
	// World matrix of the shape that describes the object's local space
	// relative to the world space, which defines the position, orientation,
	// and scale of the object in the world.
	DirectX::XMFLOAT4X4 vWorld = SMath::getIdentityMatrix4x4();

	int iUpdateCBInFrameResourceCount = SFRAME_RES_COUNT;

	size_t iObjCBIndex = 0;

	SMeshGeometry* pGeometry = nullptr;

	D3D12_PRIMITIVE_TOPOLOGY primitiveTopologyType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// DrawIndexedInstanced parameters.
	UINT iIndexCount = 0;
	UINT iStartIndexLocation = 0;
	int iStartVertexLocation = 0;
};

class SGeometry
{
public:

	// pUploadBuffer has to be kept alive because the command list has not been executed yet that performs the actual copy.
	// The caller can Release the pUploadBuffer after it knows the copy has been executed.
	static Microsoft::WRL::ComPtr<ID3D12Resource> createDefaultBuffer(
		ID3D12Device* pDevice,
		ID3D12GraphicsCommandList* pCommandList,
		const void* pInitBufferData,
		UINT64 iDataSizeInBytes,
		Microsoft::WRL::ComPtr<ID3D12Resource>& pUploadBuffer);


	static Microsoft::WRL::ComPtr<ID3DBlob> compileShader(
		const std::wstring& sPathToShader,
		const D3D_SHADER_MACRO* defines,
		const std::string& sShaderEntryPoint,
		const std::string& sShaderModel,
		bool bCompileShadersInRelease);
};

