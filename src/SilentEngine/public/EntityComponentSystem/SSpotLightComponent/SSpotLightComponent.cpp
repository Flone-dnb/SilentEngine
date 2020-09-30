// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "SSpotLightComponent.h"

SSpotLightComponent::SSpotLightComponent(std::string sComponentName) : SLightComponent(sComponentName)
{
	lightType = SLightComponentType::SLCT_SPOT;
}

SSpotLightComponent::~SSpotLightComponent()
{
}

void SSpotLightComponent::setLightColor(const SVector & vLightColorRGB)
{
	lightProps.vLightColor.x = vLightColorRGB.getX();
	lightProps.vLightColor.y = vLightColorRGB.getY();
	lightProps.vLightColor.z = vLightColorRGB.getZ();
}

void SSpotLightComponent::setLightRange(float fSpotLightRange)
{
	lightProps.fSpotLightRange = fSpotLightRange;
}

void SSpotLightComponent::setLightDirection(const SVector & vLightDirection)
{
	lightProps.vDirection = { vLightDirection.getX(), vLightDirection.getY(), vLightDirection.getZ() };
}

void SSpotLightComponent::setLightFalloffStart(float fFalloffStart)
{
	lightProps.fFalloffStart = fFalloffStart;
}

void SSpotLightComponent::setLightFalloffEnd(float fFalloffEnd)
{
	lightProps.fFalloffEnd = fFalloffEnd;
}
