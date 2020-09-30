// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "SDirectionalLightComponent.h"

SDirectionalLightComponent::SDirectionalLightComponent(std::string sComponentName) : SLightComponent(sComponentName)
{
	lightType = SLightComponentType::SLCT_DIRECTIONAL;
}

SDirectionalLightComponent::~SDirectionalLightComponent()
{
}

void SDirectionalLightComponent::setLightColor(const SVector & vLightColorRGB)
{
	lightProps.vLightColor.x = vLightColorRGB.getX();
	lightProps.vLightColor.y = vLightColorRGB.getY();
	lightProps.vLightColor.z = vLightColorRGB.getZ();
}

void SDirectionalLightComponent::setLightDirection(const SVector & vLightDirection)
{
	lightProps.vDirection = { vLightDirection.getX(), vLightDirection.getY(), vLightDirection.getZ() };
}
