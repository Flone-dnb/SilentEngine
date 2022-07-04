// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#pragma once


// Custom
#include "SilentEngine/Public/EntityComponentSystem/SContainer/SContainer.h"

class SMeshComponent;
class SDirectionalLightComponent;
class SPointLightComponent;

class MyContainer : public SContainer
{
public:

	MyContainer(const std::string& sContainerName);
	virtual ~MyContainer() override;

	void onTick(float fDeltaTime);

private:

	SMeshComponent* pFloorMeshComponent;
	SMeshComponent* pSuzanneMeshComponent;
	SMeshComponent* pSkyboxMeshComponent;

	SDirectionalLightComponent* pDirectionalLightComponent;
	SPointLightComponent* pPointLightComponent;

	SVector vInitialPointLightLocation;

	float fTotalTime = 0.0f;
	const float fPointLightRotationSpeed = 0.1f;
};

