// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#pragma once


// Custom
#include "SilentEngine/Private/EntityComponentSystem/SLightComponent/SLightComponent.h"
#include "SilentEngine/Private/SShadowMap/SShadowMap.h"

//@@Class
/*
The class represents a directional light source.
*/
class SDirectionalLightComponent : public SLightComponent
{
public:
	//@@Function
	/*
	* desc: use SLevel::getLevelBounds() for level bounds.
	*/
	SDirectionalLightComponent(std::string sComponentName, DirectX::BoundingSphere* pLevelBounds, UINT iShadowMapOneDimensionSize);

	SDirectionalLightComponent() = delete;
	SDirectionalLightComponent(const SDirectionalLightComponent&) = delete;
	SDirectionalLightComponent& operator= (const SDirectionalLightComponent&) = delete;

	virtual ~SDirectionalLightComponent() override;


	//@@Function
	/*
	* desc: sets the light color in RGB.
	*/
	void setLightColor     (const SVector& vLightColorRGB);

	//@@Function
	/*
	* desc: sets the spotlight/directional light direction.
	*/
	void setLightDirection (const SVector& vLightDirection);

private:

	virtual void allocateShadowMaps(std::vector<std::unique_ptr<SFrameResource>>* vFrameResources, ID3D12Device* pDevice,
		CD3DX12_CPU_DESCRIPTOR_HANDLE& dsvHeapHandle, UINT iDSVDescriptorSize,
		CD3DX12_CPU_DESCRIPTOR_HANDLE& srvCpuHeapHandle, CD3DX12_GPU_DESCRIPTOR_HANDLE& srvGpuHeapHandle,
		UINT iSRVDescriptorSize) override;
	virtual void deallocateShadowMaps(std::vector<std::unique_ptr<SFrameResource>>* vFrameResources) override;
	virtual void updateCBData(SFrameResource* pCurrentFrameResource) override;
	virtual void getRequiredShadowMapCount(size_t& iDSVCount) override;
	virtual void renderToShadowMaps(ID3D12GraphicsCommandList* pCommandList, class SFrameResource* pCurrentFrameResource, class SRenderPassConstants* renderPassCB) override;
	virtual void finishRenderToShadowMaps(ID3D12GraphicsCommandList* pCommandList) override;

	UINT64 iIndexInFrameResourceShadowMapBuffer;

	SShadowMap* pShadowMap = nullptr;
	DirectX::BoundingSphere* pLevelBounds = nullptr;

	UINT iShadowMapOneDimensionSize;
};

