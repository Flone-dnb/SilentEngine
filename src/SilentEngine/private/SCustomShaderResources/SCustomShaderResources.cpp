// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "SCustomShaderResources.h"

#include "SilentEngine/Public/SApplication/SApplication.h"

std::vector<SMaterial*>* SCustomShaderResources::getMaterialBundle()
{
	return &vMaterials;
}
