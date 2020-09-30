// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "SLightComponent.h"

SLightComponent::SLightComponent(std::string sComponentName)
{
	componentType = SCT_LIGHT;

	this->sComponentName = sComponentName;

	bVisible = true;
}

SLightComponent::~SLightComponent()
{
}

void SLightComponent::setVisibility(bool bIsVisible)
{
	bVisible = bIsVisible;
}

bool SLightComponent::isVisible() const
{
	return bVisible;
}
