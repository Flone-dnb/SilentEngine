// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "MyContainer.h"

MyContainer::MyContainer(const std::string& sContainerName) : SContainer(sContainerName)
{
	// Root component is container itself.
}

MyContainer::~MyContainer()
{
	// If the component was added to the container (addComponentToContainer()) then
	// here DO NOT:
	// - delete pComponent;
	// - pComponent->removeFromContainer();
	// the added component will be deleted in the SContainer's (parent) destructor.
}
