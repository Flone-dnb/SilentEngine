// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "STargetComponent.h"

STargetComponent::STargetComponent(std::string sComponentName)
{
	componentType = SCT_NONE;

	this->sComponentName = sComponentName;
}

STargetComponent::~STargetComponent()
{
}
