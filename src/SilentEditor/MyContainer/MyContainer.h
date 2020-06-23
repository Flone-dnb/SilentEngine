// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#pragma once

// Custom
#include "SilentEngine/Public/EntityComponentSystem/SContainer/SContainer.h"

class MyContainer : public SContainer
{
public:

	MyContainer(const std::string& sContainerName);
	virtual ~MyContainer() override;
};

