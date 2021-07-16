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
	* desc: constructor.
	* param "sComponentName": name of this component.
	* param "pLevelBounds": use SLevel::getLevelBounds() for level bounds.
	* param "iShadowMapOneDimensionSize": the bigger this value, the better the quality of the shadows from this light source
	(use might need to change the depth bias using SVideoSettings::setShadowMappingBias() to avoid some shadow artifacts).
	*/
	SDirectionalLightComponent(std::string sComponentName, DirectX::BoundingSphere* pLevelBounds, UINT iShadowMapOneDimensionSize = 512);

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

	virtual class SRenderPassConstants* getShadowMapConstants() override;

	virtual void allocateShadowMaps(std::vector<std::unique_ptr<SFrameResource>>* vFrameResources, ID3D12Device* pDevice,
		CD3DX12_CPU_DESCRIPTOR_HANDLE& dsvHeapHandle, UINT iDSVDescriptorSize,
		CD3DX12_CPU_DESCRIPTOR_HANDLE& srvCpuHeapHandle, CD3DX12_GPU_DESCRIPTOR_HANDLE& srvGpuHeapHandle,
		UINT iSRVDescriptorSize) override;
	virtual void deallocateShadowMaps(std::vector<std::unique_ptr<SFrameResource>>* vFrameResources) override;
	virtual void updateCBData(SFrameResource* pCurrentFrameResource) override;
	virtual void getRequiredDSVCountForShadowMaps(size_t& iDSVCount) override;
	virtual void renderToShadowMaps(ID3D12GraphicsCommandList* pCommandList, class SFrameResource* pCurrentFrameResource, class SRenderPassConstants* renderPassCB) override;
	virtual void finishRenderToShadowMaps(ID3D12GraphicsCommandList* pCommandList) override;

	UINT64 iIndexInFrameResourceShadowMapBuffer;

	SShadowMap* pShadowMap = nullptr;
	DirectX::BoundingSphere* pLevelBounds = nullptr;

	UINT iShadowMapOneDimensionSize;
};

