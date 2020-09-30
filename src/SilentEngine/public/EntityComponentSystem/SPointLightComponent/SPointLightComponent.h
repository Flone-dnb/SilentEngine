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
The class represents a point light source.
*/
class SPointLightComponent : public SLightComponent
{
public:
	//@@Function
	SPointLightComponent(std::string sComponentName);

	SPointLightComponent() = delete;
	SPointLightComponent(const SPointLightComponent&) = delete;
	SPointLightComponent& operator= (const SPointLightComponent&) = delete;

	virtual ~SPointLightComponent() override;

	//@@Function
	/*
	* desc: sets the light color in RGB.
	*/
	void setLightColor        (const SVector& vLightColorRGB);
	//@@Function
	/*
	* desc: sets the distance from the light source at which the light begins to falloff.
	*/
	void setLightFalloffStart (float fFalloffStart);
	//@@Function
	/*
	* desc: sets the distance from the light source at which the light ends to falloff.
	*/
	void setLightFalloffEnd   (float fFalloffEnd);
};

