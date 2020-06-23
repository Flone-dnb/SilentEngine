// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "SLevel.h"

// Custom
#include "SilentEngine/Public/SApplication/SApplication.h"
#include "SilentEngine/Public/EntityComponentSystem/SContainer/SContainer.h"
#include "SilentEngine/Public/EntityComponentSystem/SComponent/SComponent.h"
#include "SilentEngine/Public/EntityComponentSystem/SMeshComponent/SMeshComponent.h"

SLevel::SLevel(SApplication* pApp)
{
	this->pApp = pApp;
}

SLevel::~SLevel()
{
	std::vector<SContainer*> vAllContainers = vRenderableContainers;
	vAllContainers.insert(vAllContainers.end(), vNotRenderableContainers.begin(), vNotRenderableContainers.end());

	for (size_t i = 0; i < vAllContainers.size(); i++)
	{
		delete vAllContainers[i];
	}
}

bool SLevel::spawnContainerInLevel(SContainer* pContainer)
{
	return pApp->spawnContainerInLevel(pContainer);
}

void SLevel::despawnContainerFromLevel(SContainer* pContainer)
{
	pApp->despawnContainerFromLevel(pContainer);
}

void SLevel::getRenderableContainers(std::vector<SContainer*>*& pvRenderableContainers)
{
	pvRenderableContainers = &vRenderableContainers;
}

void SLevel::getNotRenderableContainers(std::vector<SContainer*>*& pvNotRenderableContainers)
{
	pvNotRenderableContainers = &vNotRenderableContainers;
}
