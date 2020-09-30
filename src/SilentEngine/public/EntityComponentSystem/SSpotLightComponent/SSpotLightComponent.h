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
The class represents a spot light source.
*/
class SSpotLightComponent : public SLightComponent
{
public:
	//@@Function
	SSpotLightComponent(std::string sComponentName);

	SSpotLightComponent() = delete;
	SSpotLightComponent(const SSpotLightComponent&) = delete;
	SSpotLightComponent& operator= (const SSpotLightComponent&) = delete;

	virtual ~SSpotLightComponent() override;


	//@@Function
	/*
	* desc: sets the light color in RGB.
	*/
	void setLightColor        (const SVector& vLightColorRGB);

	//@@Function
	/*
	* desc: sets the spotlight's range.
	*/
	void setLightRange        (float fSpotLightRange);

	//@@Function
	/*
	* desc: sets the spotlight direction.
	*/
	void setLightDirection    (const SVector& vLightDirection);

	//@@Function
	/*
	* desc: sets the distance from the light source (in the direction that the spotlight is shining) at which the light begins to falloff.
	*/
	void setLightFalloffStart (float fFalloffStart);
	//@@Function
	/*
	* desc: sets the distance from the light source (in the direction that the spotlight is shining) at which the light ends to falloff.
	*/
	void setLightFalloffEnd   (float fFalloffEnd);
};

