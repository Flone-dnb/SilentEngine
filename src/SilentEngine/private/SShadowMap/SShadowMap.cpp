// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "SShadowMap.h"

#include "SilentEngine/Private/SError/SError.h"

SShadowMap::SShadowMap(ID3D12Device* pDevice, CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDSV, CD3DX12_CPU_DESCRIPTOR_HANDLE cpuSRV, CD3DX12_GPU_DESCRIPTOR_HANDLE gpuSRV, UINT iSizeOfOneDimension)
{
	this->iSizeOfOneDimension = iSizeOfOneDimension;
	this->pDevice = pDevice;
	this->cpuDSV = cpuDSV;
	this->cpuSRV = cpuSRV;
	this->gpuSRV = gpuSRV;

	viewport.Width = static_cast<float>(iSizeOfOneDimension);
	viewport.Height = static_cast<float>(iSizeOfOneDimension);
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = iSizeOfOneDimension;
	scissorRect.bottom = iSizeOfOneDimension;

	createResourceAndDescriptors();
}

void SShadowMap::updateDSV(CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDSV)
{
	this->cpuDSV = cpuDSV;

	createDescriptors();
}

void SShadowMap::updateSRV(CD3DX12_CPU_DESCRIPTOR_HANDLE cpuSRV, CD3DX12_GPU_DESCRIPTOR_HANDLE gpuSRV)
{
	this->cpuSRV = cpuSRV;
	this->gpuSRV = gpuSRV;

	createDescriptors();
}

UINT SShadowMap::getOneDimensionSize() const
{
	return iSizeOfOneDimension;
}

ID3D12Resource* SShadowMap::getResource()
{
	return pShadowMap.Get();
}

CD3DX12_GPU_DESCRIPTOR_HANDLE* SShadowMap::getSRV()
{
	return &gpuSRV;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE* SShadowMap::getDSV()
{
	return &cpuDSV;
}

D3D12_VIEWPORT* SShadowMap::getViewport()
{
	return &viewport;
}

D3D12_RECT* SShadowMap::getScissorRect()
{
	return &scissorRect;
}

void SShadowMap::createResourceAndDescriptors()
{
	pShadowMap.Reset();

	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));

	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = iSizeOfOneDimension;
	texDesc.Height = iSizeOfOneDimension;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = shadowMapFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;

	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;

	auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	HRESULT hresult = pDevice->CreateCommittedResource(&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&optClear,
		IID_PPV_ARGS(&pShadowMap));
	if (FAILED(hresult))
	{
		SError::showErrorMessageBoxAndLog(hresult);
	}

	createDescriptors();
}

void SShadowMap::createDescriptors()
{
	// Create SRV to resource so we can sample the shadow map in a shader.
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc.Texture2D.PlaneSlice = 0;
	pDevice->CreateShaderResourceView(pShadowMap.Get(), &srvDesc, cpuSRV);

	// Create DSV to resource so we can render to the shadow map.
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.Texture2D.MipSlice = 0;

	pDevice->CreateDepthStencilView(pShadowMap.Get(), &dsvDesc, cpuDSV);
}
