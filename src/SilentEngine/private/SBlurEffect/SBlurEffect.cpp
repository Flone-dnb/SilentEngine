// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "SBlurEffect.h"

// Custom
#include "SilentEngine/Private/SError/SError.h"

SBlurEffect::SBlurEffect(ID3D12Device* pDevice, UINT iTextureWidth, UINT iTextureHeight, DXGI_FORMAT textureFormat)
{
	this->pDevice = pDevice;

	this->iTextureWidth = iTextureWidth;
	this->iTextureHeight = iTextureHeight;
	this->textureFormat = textureFormat;

	createResources();
}

ID3D12Resource* SBlurEffect::getOutput()
{
	return pBlurMap0.Get();
}

void SBlurEffect::assignHeapHandles(CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHeapHandle, CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHeapHandle, UINT iDescriptorSize)
{
	cpuBlur0SRV = cpuHeapHandle;
	cpuBlur0UAV = cpuHeapHandle.Offset(1, iDescriptorSize);
	cpuBlur1SRV = cpuHeapHandle.Offset(1, iDescriptorSize);
	cpuBlur1UAV = cpuHeapHandle.Offset(1, iDescriptorSize);

	gpuBlur0SRV = gpuHeapHandle;
	gpuBlur0UAV = gpuHeapHandle.Offset(1, iDescriptorSize);
	gpuBlur1SRV = gpuHeapHandle.Offset(1, iDescriptorSize);
	gpuBlur1UAV = gpuHeapHandle.Offset(1, iDescriptorSize);

	createDescriptors();
}

void SBlurEffect::resizeResources(UINT iNewTextureWidth, UINT iNewTextureHeight)
{
	if ((iTextureWidth != iNewTextureWidth) || (iTextureHeight != iNewTextureHeight))
	{
		iTextureWidth = iNewTextureWidth;
		iTextureHeight = iNewTextureHeight;

		createResources();
		createDescriptors();
	}
}

void SBlurEffect::addBlurToTexture(ID3D12GraphicsCommandList* pCommandList, ID3D12RootSignature* pComputeRootSignature, ID3D12PipelineState* pHorizontalBlurPSO, ID3D12PipelineState* pVerticalBlurPSO, ID3D12Resource* pInputTexture, size_t iBlurCount)
{
	std::vector<float> vWeights = calcGaussWeights();

	int iBlurRadius = static_cast<int>(vWeights.size()) / 2;

	pCommandList->SetComputeRootSignature(pComputeRootSignature);

	pCommandList->SetComputeRoot32BitConstants(0, static_cast<UINT>(vWeights.size()), vWeights.data(), 0);

	pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pInputTexture,
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_SOURCE));

	pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pBlurMap0.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));

	// Copy the input texture to blur map 0.
	pCommandList->CopyResource(pBlurMap0.Get(), pInputTexture);

	pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pBlurMap0.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

	// Write to blur map 1.

	pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pBlurMap1.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	for (size_t i = 0; i < iBlurCount; i++)
	{
		// Write horizontal blur to map 1.

		pCommandList->SetPipelineState(pHorizontalBlurPSO);

		pCommandList->SetComputeRootDescriptorTable(1, gpuBlur0SRV); // read from
		pCommandList->SetComputeRootDescriptorTable(2, gpuBlur1UAV); // write to

		// 256 threads in a group (defined in shader).
		UINT iThreadGroupCountX = static_cast<UINT>(ceilf(iTextureWidth / 256.0f));
		pCommandList->Dispatch(iThreadGroupCountX, iTextureHeight, 1);


		// Now write to blur map 0 and read from map 1.

		pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pBlurMap0.Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

		pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pBlurMap1.Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));

		// Write bertical blur to map 0.

		pCommandList->SetPipelineState(pVerticalBlurPSO);

		pCommandList->SetComputeRootDescriptorTable(1, gpuBlur1SRV); // read from
		pCommandList->SetComputeRootDescriptorTable(2, gpuBlur0UAV); // write to

		// 256 threads in a group (defined in shader).
		UINT iThreadGroupCountY = static_cast<UINT>(ceilf(iTextureHeight / 256.0f));
		pCommandList->Dispatch(iTextureWidth, iThreadGroupCountY, 1);

		if (i < iBlurCount - 1)
		{
			// Now write to blur map 1 again for next iteration.

			pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pBlurMap0.Get(),
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));

			pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pBlurMap1.Get(),
				D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
		}
		else
		{
			pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pBlurMap0.Get(),
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON));

			pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pBlurMap1.Get(),
				D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COMMON));
		}
	}
}

std::vector<float> SBlurEffect::calcGaussWeights()
{
	float fSigma = 2.5f; // don't touch

	int iBlurRadius = 5; // don't touch also (root signature uses 11 weights: 2 * iBlurRadius + 1, and shader has a #define with this value)

	std::vector<float> vWeights(2 * iBlurRadius + 1);

	float fWeightSum = 0.0f;
	float fTwoSigma2 = 2.0f * fSigma * fSigma;

	for (int i = -iBlurRadius; i <= iBlurRadius; i++)
	{
		float fX = static_cast<float>(i);

		vWeights[i + iBlurRadius] = expf(-fX * fX / (fTwoSigma2));

		fWeightSum += vWeights[i + iBlurRadius];
	}


	// Normalize weights.
	for (size_t i = 0; i < vWeights.size(); i++)
	{
		vWeights[i] /= fWeightSum;
	}

	return vWeights;
}

void SBlurEffect::createResources()
{
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = iTextureWidth;
	texDesc.Height = iTextureHeight;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = textureFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	HRESULT hresult = pDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&pBlurMap0));
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SBlurEffect::createResources::ID3D12Device::CreateCommittedResource() (map 0)");
		return;
	}

	hresult = pDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&pBlurMap1));
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SBlurEffect::createResources::ID3D12Device::CreateCommittedResource() (map 1)");
		return;
	}
}

void SBlurEffect::createDescriptors()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = textureFormat;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = textureFormat;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;

	pDevice->CreateShaderResourceView(pBlurMap0.Get(), &srvDesc, cpuBlur0SRV);
	pDevice->CreateUnorderedAccessView(pBlurMap0.Get(), nullptr, &uavDesc, cpuBlur0UAV);

	pDevice->CreateShaderResourceView(pBlurMap1.Get(), &srvDesc, cpuBlur1SRV);
	pDevice->CreateUnorderedAccessView(pBlurMap1.Get(), nullptr, &uavDesc, cpuBlur1UAV);
}
