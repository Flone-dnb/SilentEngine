// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#pragma once


// Custom
#include "SilentEngine/Private/EntityComponentSystem/SLightComponent/SLightComponent.h"

//@@Class
/*
The class represents a directional light source.
*/
class SDirectionalLightComponent : public SLightComponent
{
public:
	//@@Function
	SDirectionalLightComponent(std::string sComponentName);

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
};

