// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************


#include "SPointLightComponent.h"

// Custom
#include "SilentEngine/Public/SApplication/SApplication.h"

SPointLightComponent::SPointLightComponent(std::string sComponentName, UINT iShadowMapOneDimensionSize) : SLightComponent(sComponentName)
{
	lightType = SLightComponentType::SLCT_POINT;

	this->iShadowMapOneDimensionSize = iShadowMapOneDimensionSize;

	iIndexInFrameResourceShadowMapBuffer = 0;

	iRequiredDSVs = 6;
	iRequiredSRVs = 6;
}

SPointLightComponent::~SPointLightComponent()
{
}

void SPointLightComponent::setLightColor(const SVector & vLightColorRGB)
{
	lightProps.vLightColor.x = vLightColorRGB.getX();
	lightProps.vLightColor.y = vLightColorRGB.getY();
	lightProps.vLightColor.z = vLightColorRGB.getZ();
}

void SPointLightComponent::setLightFalloffStart(float fFalloffStart)
{
	lightProps.fFalloffStart = fFalloffStart;
}

void SPointLightComponent::setLightFalloffEnd(float fFalloffEnd)
{
	lightProps.fFalloffEnd = fFalloffEnd;
}

SRenderPassConstants* SPointLightComponent::getShadowMapConstants()
{
	SError::showErrorMessageBoxAndLog("use other getShadowMapConstants() implementation.");
	return nullptr;
}

SRenderPassConstants* SPointLightComponent::getShadowMapConstants(size_t iShadowMapIndex)
{
	return &vShadowMaps[iShadowMapIndex]->shadowMapCB;
}

void SPointLightComponent::allocateShadowMaps(std::vector<std::unique_ptr<SFrameResource>>* vFrameResources, ID3D12Device* pDevice, CD3DX12_CPU_DESCRIPTOR_HANDLE& dsvHeapHandle, UINT iDSVDescriptorSize, CD3DX12_CPU_DESCRIPTOR_HANDLE& srvCpuHeapHandle, CD3DX12_GPU_DESCRIPTOR_HANDLE& srvGpuHeapHandle, UINT iSRVDescriptorSize)
{
	if (vShadowMaps.size() > 0)
	{
		for (size_t i = 0; i < iRequiredDSVs; i++)
		{
			// update old dsv
			vShadowMaps[i]->updateDSV(dsvHeapHandle);
			vShadowMaps[i]->updateSRV(srvCpuHeapHandle, srvGpuHeapHandle);

			dsvHeapHandle.Offset(static_cast<UINT>(1), iDSVDescriptorSize);
			srvCpuHeapHandle.Offset(static_cast<UINT>(1), iSRVDescriptorSize);
			srvGpuHeapHandle.Offset(static_cast<UINT>(1), iSRVDescriptorSize);
		}
		
	}
	else
	{
		for (size_t i = 0; i < vFrameResources->size(); i++)
		{
			bool bExpanded; // don't care, we anyway update CB on every frame
			iIndexInFrameResourceShadowMapBuffer = vFrameResources->operator[](i)->addNewShadowMapCB(iRequiredDSVs, &bExpanded);
			// returns the start index
		}

		UINT iIndex = iIndexInFrameResourceShadowMapBuffer;

		for (size_t i = 0; i < iRequiredDSVs; i++)
		{
			vShadowMaps.push_back(new SShadowMap(pDevice, dsvHeapHandle, srvCpuHeapHandle, srvGpuHeapHandle, iShadowMapOneDimensionSize));
			vShadowMaps.back()->iShadowMapCBIndex = iIndex;

			iIndex++;
			dsvHeapHandle.Offset(static_cast<UINT>(1), iDSVDescriptorSize);
			srvCpuHeapHandle.Offset(static_cast<UINT>(1), iSRVDescriptorSize);
			srvGpuHeapHandle.Offset(static_cast<UINT>(1), iSRVDescriptorSize);
		}
	}
}

void SPointLightComponent::deallocateShadowMaps(std::vector<std::unique_ptr<SFrameResource>>* vFrameResources)
{
	for (size_t i = 0; i < vFrameResources->size(); i++)
	{
		bool bExpanded; // don't care, we anyway update CB on every frame
		vFrameResources->operator[](i)->removeShadowMapCB(iIndexInFrameResourceShadowMapBuffer, iRequiredDSVs, &bExpanded);
	}

	for (size_t i = 0; i < iRequiredDSVs; i++)
	{
		delete vShadowMaps[i];
	}

	vShadowMaps.clear();
}

void SPointLightComponent::updateCBData(SFrameResource* pCurrentFrameResource)
{
	using namespace DirectX;

	XMFLOAT3 vDirections[6] = {
		{1.0f, 0.0f, 0.0f},
		{-1.0f, 0.0f, 0.0f},
		{0.0f, 1.0f, 0.0f},
		{0.0f, -1.0f, 0.0f},
		{0.0f, 0.0f, 1.0f},
		{0.0f, 0.0f, -1.0f},
	};

	XMFLOAT3 vUps[6] =
	{
		XMFLOAT3(0.0f, 0.0f, 1.0f), // +X
		XMFLOAT3(0.0f, 0.0f, 1.0f), // -X
		XMFLOAT3(0.0f, 0.0f, 1.0f), // +Y
		XMFLOAT3(0.0f, 0.0f, 1.0f), // -Y
		XMFLOAT3(0.0f, -1.0f, 0.0f), // +Z
		XMFLOAT3(0.0f, 1.0f, 0.0f) // -Z
	};

	float n = SApplication::getApp()->getCamera()->getCameraNearClipPlane();
	float f = lightProps.fFalloffEnd;// SApplication::getApp()->getCamera()->getCameraFarClipPlane();

	for (size_t i = 0; i < iRequiredDSVs; i++)
	{
		XMVECTOR lightDir = XMLoadFloat3(&vDirections[i]);

		XMVECTOR lightPos = XMLoadFloat3(&lightProps.vPosition);
		
		//XMVECTOR lightUp = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
		XMVECTOR lightUp = XMLoadFloat3(&vUps[i]);
		XMMATRIX view = XMMatrixLookToLH(lightPos, lightDir, lightUp);

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

		UINT iMapOneDimensionSize = vShadowMaps[0]->getOneDimensionSize(); // all have the same size


		SRenderPassConstants shadowMapCB;

		XMStoreFloat4x4(&shadowMapCB.vView, XMMatrixTranspose(view));
		XMStoreFloat4x4(&shadowMapCB.vInvView, XMMatrixTranspose(invView));
		XMStoreFloat4x4(&shadowMapCB.vProj, XMMatrixTranspose(proj));
		XMStoreFloat4x4(&shadowMapCB.vInvProj, XMMatrixTranspose(invProj));
		XMStoreFloat4x4(&shadowMapCB.vViewProj, XMMatrixTranspose(viewProj));
		XMStoreFloat4x4(&shadowMapCB.vInvViewProj, XMMatrixTranspose(invViewProj));

		XMStoreFloat4x4(&lightProps.mLightViewProjTex[i], XMMatrixTranspose(shadowTransform));

		XMStoreFloat3(&shadowMapCB.vCameraPos, lightPos);

		shadowMapCB.vRenderTargetSize = DirectX::XMFLOAT2(static_cast<float>(iMapOneDimensionSize), static_cast<float>(iMapOneDimensionSize));
		shadowMapCB.vInvRenderTargetSize = DirectX::XMFLOAT2(1.0f / iMapOneDimensionSize, 1.0f / iMapOneDimensionSize);
		shadowMapCB.fNearZ = n;
		shadowMapCB.fFarZ = f;

		vShadowMaps[i]->shadowMapCB = shadowMapCB;

		pCurrentFrameResource->pShadowMapsCB->copyDataToElement(iIndexInFrameResourceShadowMapBuffer + i, shadowMapCB);
	}
}

void SPointLightComponent::getRequiredDSVCountForShadowMaps(size_t& iDSVCount)
{
	iDSVCount += iRequiredDSVs;
}

void SPointLightComponent::renderToShadowMaps(ID3D12GraphicsCommandList* pCommandList, SFrameResource* pCurrentFrameResource, SRenderPassConstants* renderPassCB)
{
	SError::showErrorMessageBoxAndLog("use other renderToShadowMaps() implementation.");
}

void SPointLightComponent::renderToShadowMaps(ID3D12GraphicsCommandList* pCommandList, SFrameResource* pCurrentFrameResource,
	SRenderPassConstants* renderPassCB, size_t iShadowMapIndex)
{
	pCommandList->RSSetViewports(1, vShadowMaps[iShadowMapIndex]->getViewport());
	pCommandList->RSSetScissorRects(1, vShadowMaps[iShadowMapIndex]->getScissorRect());

	auto toDepthWrite
		= CD3DX12_RESOURCE_BARRIER::Transition(vShadowMaps[iShadowMapIndex]->getResource(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	pCommandList->ResourceBarrier(1,
		&toDepthWrite);

	// Clear the shadow map.
	pCommandList->ClearDepthStencilView(*vShadowMaps[iShadowMapIndex]->getDSV(),
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Set null render target because we are only going to draw to
	// depth buffer. Setting a null render target will disable color writes.
	// Note the active PSO also must specify a render target count of 0.
	pCommandList->OMSetRenderTargets(0, nullptr, false, vShadowMaps[iShadowMapIndex]->getDSV());

	// change render pass cb (with light source view/proj)
	pCommandList->SetGraphicsRootConstantBufferView(0, pCurrentFrameResource->pShadowMapsCB.get()->getResource()->GetGPUVirtualAddress()
		+ vShadowMaps[iShadowMapIndex]->iShadowMapCBIndex * pCurrentFrameResource->pShadowMapsCB->getElementSize());
}

void SPointLightComponent::finishRenderToShadowMaps(ID3D12GraphicsCommandList* pCommandList)
{
	SError::showErrorMessageBoxAndLog("use other finishRenderToShadowMaps() implementation.");
}

void SPointLightComponent::finishRenderToShadowMaps(ID3D12GraphicsCommandList* pCommandList, size_t iShadowMapIndex)
{
	// Change back to GENERIC_READ so we can read the texture in a shader.
	auto toGenericRead
		= CD3DX12_RESOURCE_BARRIER::Transition(vShadowMaps[iShadowMapIndex]->getResource(),
			D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ);
	pCommandList->ResourceBarrier(1, &toGenericRead);
}
