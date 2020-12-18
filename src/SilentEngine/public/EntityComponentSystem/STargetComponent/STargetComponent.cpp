// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "STargetComponent.h"

#include "SilentEngine/Private/EntityComponentSystem/SComponent/SComponent.h"

STargetComponent::STargetComponent(std::string sComponentName)
{
	componentType = SCT_NONE;

	this->sComponentName = sComponentName;
}

STargetComponent::~STargetComponent()
{
}

void STargetComponent::updateMyAndChildsLocationRotationScale(bool bCalledOnSelf)
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
