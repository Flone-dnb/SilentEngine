// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#pragma once

// Custom
#include "SilentEngine/Private/EntityComponentSystem/SComponent/SComponent.h"


#define MAX_LIGHTS 16 // ALSO CHANGE IN SHADERS

struct SLightProps
{
	DirectX::XMFLOAT3 vLightColor = { 1.0f, 1.0f, 1.0f };
	float fFalloffStart = 1.0f; // point/spot light only
	DirectX::XMFLOAT3 vDirection = { 0.0f, 0.0f, -1.0f }; // directional/spot light only
	float fFalloffEnd = 30.0f; // point/spot light only
	DirectX::XMFLOAT3 vPosition = { 0.0f, 0.0f, 0.0f }; // point/spot light only
	float fSpotLightRange = 128.0f; // spot light only
	DirectX::XMFLOAT4X4 mLightViewProjTex = SMath::getIdentityMatrix4x4();
};

enum class SLightComponentType
{
	SLCT_DIRECTIONAL = 0,
	SLCT_SPOT = 1,
	SLCT_POINT = 2
};

class SLightComponent : public SComponent
{
public:
	//@@Function
	SLightComponent(std::string sComponentName);

	SLightComponent() = delete;
	SLightComponent(const SLightComponent&) = delete;
	SLightComponent& operator= (const SLightComponent&) = delete;

	virtual ~SLightComponent() override;

	//@@Function
	/*
	* desc: determines if the component should be visible (i.e. drawn).
	* param "bVisible": true by default.
	*/
	void setVisibility(bool bIsVisible);

	//@@Function
	/*
	* desc: determines if the component is visible (i.e. drawn).
	* return: true if visible, false otherwise.
	*/
	bool isVisible() const;

protected:

	friend class SApplication;
	friend class SComponent;

	virtual class SRenderPassConstants* getShadowMapConstants() = 0;

	virtual void updateMyAndChildsLocationRotationScale(bool bCalledOnSelf) override;
	virtual void allocateShadowMaps(std::vector<std::unique_ptr<SFrameResource>>* vFrameResources, ID3D12Device* pDevice,
		CD3DX12_CPU_DESCRIPTOR_HANDLE& dsvHeapHandle, UINT iDSVDescriptorSize,
		CD3DX12_CPU_DESCRIPTOR_HANDLE& srvCpuHeapHandle, CD3DX12_GPU_DESCRIPTOR_HANDLE& srvGpuHeapHandle,
		UINT iSRVDescriptorSize) = 0;
	virtual void deallocateShadowMaps(std::vector<std::unique_ptr<SFrameResource>>* vFrameResources) = 0;
	virtual void updateCBData(SFrameResource* pCurrentFrameResource) = 0;
	virtual void getRequiredDSVCountForShadowMaps(size_t& iDSVCount) = 0;
	virtual void renderToShadowMaps(ID3D12GraphicsCommandList* pCommandList, class SFrameResource* pCurrentFrameResource, class SRenderPassConstants* renderPassCB) = 0;
	virtual void finishRenderToShadowMaps(ID3D12GraphicsCommandList* pCommandList) = 0;

	SLightProps lightProps;

	SLightComponentType lightType;

	// for shadow maps (childs will override this)
	size_t iRequiredDSVs = 0;
	size_t iRequiredSRVs = 0;
};

