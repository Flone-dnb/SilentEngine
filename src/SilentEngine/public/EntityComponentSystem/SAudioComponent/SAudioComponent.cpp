// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "SAudioComponent.h"

// Engine
#include "SilentEngine/Public/SApplication/SApplication.h"
#include "SilentEngine/Private/SError/SError.h"

SAudioComponent::SAudioComponent(const std::string& sComponentName, bool bIs3DSound)
{
	componentType = SComponentType::SCT_AUDIO;

	this->sComponentName = sComponentName;

	if (SApplication::getApp()->getAudioEngine() == nullptr)
	{
		SError::showErrorMessageBoxAndLog("can't create the sound because the audio engine is not created.");
		return;
	}

	pSound = new SSound(SApplication::getApp()->getAudioEngine(), bIs3DSound, this);
}

SSound* SAudioComponent::getSound()
{
	return pSound;
}

SAudioComponent::~SAudioComponent()
{
	if (pSound) delete pSound;
}

void SAudioComponent::updateMyAndChildsLocationRotationScale(bool bCalledOnSelf)
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
