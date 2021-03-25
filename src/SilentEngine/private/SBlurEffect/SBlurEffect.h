// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#pragma once

// STL
#include <vector>

// DirectX
#include <wrl.h> // smart pointers
#include <d3d12.h>
#pragma warning(push, 0) // disable warnings from this header
#include "SilentEngine/private/d3dx12.h"
#pragma warning(pop)

#define BLUR_VIEW_COUNT 4 // 2 for srv, 2 for uav

class SBlurEffect
{
public:

	SBlurEffect(ID3D12Device* pDevice, UINT iTextureWidth, UINT iTextureHeight, DXGI_FORMAT textureFormat);
	SBlurEffect(const SBlurEffect&) = delete;
	SBlurEffect& operator=(const SBlurEffect&) = delete;

	void assignHeapHandles(CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHeapHandle, CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHeapHandle, UINT iDescriptorSize);
	void resizeResources(UINT iNewTextureWidth, UINT iNewTextureHeight);

	void addBlurToTexture(ID3D12GraphicsCommandList* pCommandList,
		ID3D12RootSignature* pComputeRootSignature,
		ID3D12PipelineState* pHorizontalBlurPSO,
		ID3D12PipelineState* pVerticalBlurPSO,
		ID3D12Resource* pInputTexture,
		size_t iBlurCount);

	ID3D12Resource* getOutput();

private:

	std::vector<float> calcGaussWeights();

	void createResources();
	void createDescriptors();


	ID3D12Device* pDevice = nullptr;


	UINT iTextureWidth = 0;
	UINT iTextureHeight = 0;
	DXGI_FORMAT textureFormat = DXGI_FORMAT_R8G8B8A8_UNORM;


	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuBlur0SRV;
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuBlur0UAV;

	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuBlur1SRV;
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuBlur1UAV;

	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuBlur0SRV;
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuBlur0UAV;

	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuBlur1SRV;
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuBlur1UAV;


	Microsoft::WRL::ComPtr<ID3D12Resource> pBlurMap0 = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> pBlurMap1 = nullptr;
};

