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
#include "SilentEngine/Public/SVector/SVector.h"


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
	SVector vTexUVOffset = SVector(1.0f, 1.0f, 1.0f);
	float   fTexRotation = 0.0f;
	SVector vTexUVScale  = SVector(1.0f, 1.0f, 1.0f);

	//@@Function
	/*
	* desc: used to set the UV offset to the texture.
	* return: true if the UVs are not in [0, 1] range, false otherwise.
	* remarks: should be in [0, 1] range.
	*/
	bool setTextureUVOffset(const SVector& vTextureUVOffset)
	{
		if (vTextureUVOffset.getX() < 0.0f || vTextureUVOffset.getX() > 1.0f)
		{
			return true;
		}

		if (vTextureUVOffset.getY() < 0.0f || vTextureUVOffset.getY() > 1.0f)
		{
			return true;
		}

		vTexUVOffset = vTextureUVOffset;

		updateTexTransform();

		return false;
	}

	//@@Function
	/*
	* desc: used to set the UV scale to the texture.
	*/
	void setTextureUVScale(const SVector& vTextureUVScale)
	{
		vTexUVScale = vTextureUVScale;

		updateTexTransform();
	}

	//@@Function
	/*
	* desc: used to set the UV rotation to the texture.
	*/
	void setTextureUVRotation(float fRotation)
	{
		fTexRotation = fRotation;

		updateTexTransform();
	}

	//@@Function
	/*
	* desc: returns the UV offset of the texture.
	*/
	SVector getTextureUVOffset() const
	{
		return vTexUVOffset;
	}

	//@@Function
	/*
	* desc: returns the UV scale of the texture.
	*/
	SVector getTextureUVScale() const
	{
		return vTexUVScale;
	}

	//@@Function
	/*
	* desc: returns the UV rotation of the texture.
	*/
	float getTextureUVRotation() const
	{
		return fTexRotation;
	}

	void updateTexTransform()
	{
		DirectX::XMMATRIX transform = DirectX::XMMatrixIdentity() *
			DirectX::XMMatrixTranslation(-0.5f, -0.5f, 0.0f) * // move center to the origin point
			DirectX::XMMatrixScaling(vTexUVScale.getX(), vTexUVScale.getY(), vTexUVScale.getZ()) *
			DirectX::XMMatrixRotationZ(DirectX::XMConvertToRadians(fTexRotation)) *
			DirectX::XMMatrixTranslation(vTexUVOffset.getX(), vTexUVOffset.getY(), vTexUVOffset.getZ()) *
			DirectX::XMMatrixTranslation(0.5f, 0.5f, 0.0f); // move center back.

		DirectX::XMStoreFloat4x4(&vTexTransform, transform);
	}



	// World matrix of the shape that describes the object's local space
	// relative to the world space, which defines the position, orientation,
	// and scale of the object in the world.
	DirectX::XMFLOAT4X4 vWorld = SMath::getIdentityMatrix4x4();

	DirectX::XMFLOAT4X4 vTexTransform = SMath::getIdentityMatrix4x4();

	unsigned int iCustomShaderProperty = 0;

	int iUpdateCBInFrameResourceCount = SFRAME_RES_COUNT;

	size_t iObjCBIndex = 0;

	SMeshGeometry* pGeometry = nullptr;

	D3D12_PRIMITIVE_TOPOLOGY primitiveTopologyType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// DrawIndexedInstanced parameters.
	UINT iIndexCount = 0;
	UINT iStartIndexLocation = 0;
	int iStartVertexLocation = 0;
};