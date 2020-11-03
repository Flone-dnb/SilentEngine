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
};

enum SLightComponentType
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

	void updateMyAndChildsLocationRotationScale() {};

	SLightProps lightProps;

	SLightComponentType lightType;

	bool bVisible;
};

