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
	// Remember: 1 unit - 1 meter (near/far clip planes was picked so that 1 unit - 1 meter).

	// If you're making a space game for example, then you might consider 1 unit as 10 meters (for example), but this will
	// require you to change the near clip plane (in this example to 1.0 at least) to avoid z-fighting.
	// In the case of a space game also do not forget about float precision in terms of position (as it's stored in floats).


}

MyContainer::~MyContainer()
{
	// If the component was added to the container (addComponentToContainer()) then
	// here DO NOT:
	// - delete pComponent;
	// - pComponent->removeFromContainer();
	// the added component will be deleted in the SContainer's (parent) destructor.
}