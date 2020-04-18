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
#include <unordered_map>

// DirectX
#include <wrl.h> // smart pointers
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXCollision.h>
#include <d3d12.h>
#include "SilentEngine/private/d3dx12.h"
#include <DirectXColors.h>

// Custom
#include "SilentEngine/private/SMath/SMath.h"

#pragma comment(lib, "D3D12.lib")



struct SVertex
{
	DirectX::XMFLOAT3 vPos;
	DirectX::XMFLOAT4 vColor;
};


struct SObjectConstants
{
	DirectX::XMFLOAT4X4 vWorldViewProj = SMath::getIdentityMatrix4x4();
};


// Defines a subrange of geometry in a SMeshGeometry. This is for when multiple
// geometries are stored in one vertex and index buffer. It provides the offsets
// and data needed to draw a subset of geometry stores in the vertex and index buffers.
struct SSubmeshGeometry
{
	UINT iIndexCount = 0;
	UINT iStartIndexLocation = 0;
	INT  iBaseVertexLocation = 0; 

	DirectX::BoundingBox bounds;
};

struct SMeshGeometry
{
	std::wstring sMeshName;

	// System memory copies. Use Blobs because the vertex/index format can be generic.
	// It is up to the client to cast appropriately.
	Microsoft::WRL::ComPtr<ID3DBlob> pVertexBufferCPU = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pIndexBufferCPU  = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> pVertexBufferGPU = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> pIndexBufferGPU = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> pVertexBufferUploader = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> pIndexBufferUploader = nullptr;


	// Data about the buffers.
	UINT iVertexByteStride = 0;
	UINT iVertexBufferSizeInBytes = 0;
	UINT iIndexBufferSizeInBytes = 0;
	DXGI_FORMAT indexFormat = DXGI_FORMAT_R16_UINT;


	// A MeshGeometry may store multiple geometries in one vertex/index buffer.
	// Use this container to define the Submesh geometries so we can draw the Submeshes individually.
	std::unordered_map<std::wstring, SSubmeshGeometry> mDrawArgs;


	D3D12_VERTEX_BUFFER_VIEW getVertexBufferView() const;
	D3D12_INDEX_BUFFER_VIEW  getIndexBufferView()  const;


	// We can free this memory after we finish upload to the GPU.
	void freeUploaders();
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
		const std::string& sShaderModel);
};

