// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "SLightComponent.h"

#include "SilentEngine/Private/EntityComponentSystem/SComponent/SComponent.h"

SLightComponent::SLightComponent(std::string sComponentName)
{
	componentType = SCT_LIGHT;

	this->sComponentName = sComponentName;

	bVisible = true;
}

SLightComponent::~SLightComponent()
{
}

void SLightComponent::setVisibility(bool bIsVisible)
{
	bVisible = bIsVisible;
}

bool SLightComponent::isVisible() const
{
	return bVisible;
}

void SLightComponent::updateMyAndChildsLocationRotationScale(bool bCalledOnSelf)
{
	mtxComponentProps.lock();

	XMStoreFloat4x4(&renderData.vWorld, getWorldMatrix());

	mtxComponentProps.unlock();


	if ((bCalledOnSelf == false) && onParentLocationRotationScaleChangedCallback)
	{
		onParentLocationRotationScaleChangedCallback(this);
	}


	renderData.iUpdateCBInFrameResourceCount = SFRAME_RES_COUNT;

	for (size_t i = 0; i < vChildComponents.size(); i++)
	{
		vChildComponents[i]->updateMyAndChildsLocationRotationScale(false);
	}
}
