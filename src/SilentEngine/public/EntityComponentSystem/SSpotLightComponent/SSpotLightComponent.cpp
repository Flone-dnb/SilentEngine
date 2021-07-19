// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "SSpotLightComponent.h"

#include "SilentEngine/Public/SApplication/SApplication.h"

SSpotLightComponent::SSpotLightComponent(std::string sComponentName, UINT iShadowMapOneDimensionSize) : SLightComponent(sComponentName)
{
	lightType = SLightComponentType::SLCT_SPOT;
	
	this->iShadowMapOneDimensionSize = iShadowMapOneDimensionSize;

	iIndexInFrameResourceShadowMapBuffer = 0;

	iRequiredDSVs = 1;
	iRequiredSRVs = 1;
}

SSpotLightComponent::~SSpotLightComponent()
{
}

void SSpotLightComponent::setLightColor(const SVector & vLightColorRGB)
{
	lightProps.vLightColor.x = vLightColorRGB.getX();
	lightProps.vLightColor.y = vLightColorRGB.getY();
	lightProps.vLightColor.z = vLightColorRGB.getZ();
}

void SSpotLightComponent::setLightRange(float fSpotLightRange)
{
	lightProps.fSpotLightRange = fSpotLightRange;
}

void SSpotLightComponent::setLightDirection(const SVector & vLightDirection)
{
	lightProps.vDirection = { vLightDirection.getX(), vLightDirection.getY(), vLightDirection.getZ() };

	DirectX::XMFLOAT3 vUPf;
	DirectX::XMStoreFloat3(&vUPf, DirectX::XMVector3Orthogonal(DirectX::XMVectorSet(vLightDirection.getX(), vLightDirection.getY(), vLightDirection.getZ(), 0.0f)));
	vUP = SVector(vUPf.x, vUPf.y, vUPf.z);
}

void SSpotLightComponent::setLightFalloffStart(float fFalloffStart)
{
	lightProps.fFalloffStart = fFalloffStart;
}

void SSpotLightComponent::setLightFalloffEnd(float fFalloffEnd)
{
	lightProps.fFalloffEnd = fFalloffEnd;
}

SRenderPassConstants* SSpotLightComponent::getShadowMapConstants()
{
	return &pShadowMap->shadowMapCB;
}

void SSpotLightComponent::allocateShadowMaps(std::vector<std::unique_ptr<SFrameResource>>* vFrameResources, ID3D12Device* pDevice, CD3DX12_CPU_DESCRIPTOR_HANDLE& dsvHeapHandle, UINT iDSVDescriptorSize, CD3DX12_CPU_DESCRIPTOR_HANDLE& srvCpuHeapHandle, CD3DX12_GPU_DESCRIPTOR_HANDLE& srvGpuHeapHandle, UINT iSRVDescriptorSize)
{
	if (pShadowMap)
	{
		// update old dsv
		pShadowMap->updateDSV(dsvHeapHandle);
		pShadowMap->updateSRV(srvCpuHeapHandle, srvGpuHeapHandle);
	}
	else
	{
		for (size_t i = 0; i < vFrameResources->size(); i++)
		{
			bool bExpanded; // don't care, we anyway update CB on every frame
			iIndexInFrameResourceShadowMapBuffer = vFrameResources->operator[](i)->addNewShadowMapCB(iRequiredDSVs, &bExpanded);
			// index will be the same because we only use 1 map here
		}
		pShadowMap = new SShadowMap(pDevice, dsvHeapHandle, srvCpuHeapHandle, srvGpuHeapHandle, iShadowMapOneDimensionSize);
		pShadowMap->iShadowMapCBIndex = iIndexInFrameResourceShadowMapBuffer;
	}

	dsvHeapHandle.Offset(static_cast<UINT>(iRequiredDSVs), iDSVDescriptorSize);
	srvCpuHeapHandle.Offset(static_cast<UINT>(iRequiredSRVs), iSRVDescriptorSize);
	srvGpuHeapHandle.Offset(static_cast<UINT>(iRequiredSRVs), iSRVDescriptorSize);
}

void SSpotLightComponent::deallocateShadowMaps(std::vector<std::unique_ptr<SFrameResource>>* vFrameResources)
{
	for (size_t i = 0; i < vFrameResources->size(); i++)
	{
		bool bExpanded; // don't care, we anyway update CB on every frame
		vFrameResources->operator[](i)->removeShadowMapCB(iIndexInFrameResourceShadowMapBuffer, iRequiredDSVs, &bExpanded);
	}

	delete pShadowMap;
	pShadowMap = nullptr;
}

void SSpotLightComponent::updateCBData(SFrameResource* pCurrentFrameResource)
{
	using namespace DirectX;

	XMVECTOR lightDir = XMLoadFloat3(&lightProps.vDirection);
	XMVECTOR lightPos = XMLoadFloat3(&lightProps.vPosition);
	XMVECTOR lightUp = XMVectorSet(vUP.getX(), vUP.getY(), vUP.getZ(), 0.0f);
	XMMATRIX view = XMMatrixLookToLH(lightPos, lightDir, lightUp);

	float n = SApplication::getApp()->getCamera()->getCameraNearClipPlane();
	float f = lightProps.fFalloffEnd;// SApplication::getApp()->getCamera()->getCameraFarClipPlane();

	XMMATRIX proj = XMMatrixPerspectiveFovLH(XMConvertToRadians(90.0f), 1.0f, n, f);

	// Transform NDC space [-1,+1]^2 to texture space [0,1]^2
	XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	XMMATRIX shadowTransform = view * proj * T;


	XMMATRIX viewProj = XMMatrixMultiply(view, proj);

	XMVECTOR viewDet = XMMatrixDeterminant(view);
	XMVECTOR projDet = XMMatrixDeterminant(proj);
	XMVECTOR viewProjDet = XMMatrixDeterminant(viewProj);

	XMMATRIX invView = XMMatrixInverse(&viewDet, view);
	XMMATRIX invProj = XMMatrixInverse(&projDet, proj);
	XMMATRIX invViewProj = XMMatrixInverse(&viewProjDet, viewProj);

	UINT iMapOneDimensionSize = pShadowMap->getOneDimensionSize();


	SRenderPassConstants shadowMapCB;

	XMStoreFloat4x4(&shadowMapCB.vView, XMMatrixTranspose(view));
	XMStoreFloat4x4(&shadowMapCB.vInvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&shadowMapCB.vProj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&shadowMapCB.vInvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&shadowMapCB.vViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&shadowMapCB.vInvViewProj, XMMatrixTranspose(invViewProj));

	XMStoreFloat4x4(&lightProps.mLightViewProjTex[0], XMMatrixTranspose(shadowTransform));

	XMStoreFloat3(&shadowMapCB.vCameraPos, lightPos);

	shadowMapCB.vRenderTargetSize = DirectX::XMFLOAT2(static_cast<float>(iMapOneDimensionSize), static_cast<float>(iMapOneDimensionSize));
	shadowMapCB.vInvRenderTargetSize = DirectX::XMFLOAT2(1.0f / iMapOneDimensionSize, 1.0f / iMapOneDimensionSize);
	shadowMapCB.fNearZ = n;
	shadowMapCB.fFarZ = f;

	pShadowMap->shadowMapCB = shadowMapCB;

	pCurrentFrameResource->pShadowMapsCB->copyDataToElement(iIndexInFrameResourceShadowMapBuffer, shadowMapCB);
}

void SSpotLightComponent::getRequiredDSVCountForShadowMaps(size_t& iDSVCount)
{
	iDSVCount += iRequiredDSVs;
}

void SSpotLightComponent::renderToShadowMaps(ID3D12GraphicsCommandList* pCommandList, SFrameResource* pCurrentFrameResource, SRenderPassConstants* renderPassCB)
{
	pCommandList->RSSetViewports(1, pShadowMap->getViewport());
	pCommandList->RSSetScissorRects(1, pShadowMap->getScissorRect());

	auto toDepthWrite
		= CD3DX12_RESOURCE_BARRIER::Transition(pShadowMap->getResource(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	pCommandList->ResourceBarrier(1,
		&toDepthWrite);

	// Clear the shadow map.
	pCommandList->ClearDepthStencilView(*pShadowMap->getDSV(),
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Set null render target because we are only going to draw to
	// depth buffer. Setting a null render target will disable color writes.
	// Note the active PSO also must specify a render target count of 0.
	pCommandList->OMSetRenderTargets(0, nullptr, false, pShadowMap->getDSV());

	// change render pass cb (with light source view/proj)
	pCommandList->SetGraphicsRootConstantBufferView(0, pCurrentFrameResource->pShadowMapsCB.get()->getResource()->GetGPUVirtualAddress()
		+ pShadowMap->iShadowMapCBIndex * pCurrentFrameResource->pShadowMapsCB->getElementSize());
}

void SSpotLightComponent::finishRenderToShadowMaps(ID3D12GraphicsCommandList* pCommandList)
{
	// Change back to GENERIC_READ so we can read the texture in a shader.
	auto toGenericRead
		= CD3DX12_RESOURCE_BARRIER::Transition(pShadowMap->getResource(),
			D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ);
	pCommandList->ResourceBarrier(1, &toGenericRead);
}
