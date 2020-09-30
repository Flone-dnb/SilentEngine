// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************


#include "SPointLightComponent.h"

SPointLightComponent::SPointLightComponent(std::string sComponentName) : SLightComponent(sComponentName)
{
	lightType = SLightComponentType::SLCT_POINT;
}

SPointLightComponent::~SPointLightComponent()
{
}

void SPointLightComponent::setLightColor(const SVector & vLightColorRGB)
{
	lightProps.vLightColor.x = vLightColorRGB.getX();
	lightProps.vLightColor.y = vLightColorRGB.getY();
	lightProps.vLightColor.z = vLightColorRGB.getZ();
}

void SPointLightComponent::setLightFalloffStart(float fFalloffStart)
{
	lightProps.fFalloffStart = fFalloffStart;
}

void SPointLightComponent::setLightFalloffEnd(float fFalloffEnd)
{
	lightProps.fFalloffEnd = fFalloffEnd;
}
