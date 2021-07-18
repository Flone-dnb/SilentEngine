// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#pragma once

// DirectX
#include <d3d12.h>
#include "SilentEngine/Private/d3dx12.h"
#include <wrl.h> // smart pointers

// Custom
#include "SilentEngine/Private/SFrameResource/SFrameResource.h"

class SShadowMap
{
public:
	SShadowMap(ID3D12Device* pDevice, CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDSV, CD3DX12_CPU_DESCRIPTOR_HANDLE cpuSRV, CD3DX12_GPU_DESCRIPTOR_HANDLE gpuSRV, UINT iSizeOfOneDimension = 512);
	SShadowMap() = delete;
	SShadowMap(const SShadowMap&) = delete;
	SShadowMap& operator= (const SShadowMap&) = delete;
	~SShadowMap() = default;

	void updateDSV(CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDSV);
	void updateSRV(CD3DX12_CPU_DESCRIPTOR_HANDLE cpuSRV, CD3DX12_GPU_DESCRIPTOR_HANDLE gpuSRV);

	UINT getOneDimensionSize() const;

	ID3D12Resource* getResource();
	CD3DX12_GPU_DESCRIPTOR_HANDLE* getSRV();
	CD3DX12_CPU_DESCRIPTOR_HANDLE* getDSV();
	D3D12_VIEWPORT* getViewport();
	D3D12_RECT* getScissorRect();

	SRenderPassConstants shadowMapCB;
	UINT iShadowMapCBIndex = 0;

private:

	void createResourceAndDescriptors();
	void registerDSV();
	void registerSRV();

	ID3D12Device* pDevice = nullptr;
	D3D12_VIEWPORT viewport;
	D3D12_RECT scissorRect;

	UINT iSizeOfOneDimension;
	DXGI_FORMAT shadowMapFormat = DXGI_FORMAT_R24G8_TYPELESS;
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuSRV;
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuSRV;
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDSV;
	Microsoft::WRL::ComPtr<ID3D12Resource> pShadowMap = nullptr;
};

