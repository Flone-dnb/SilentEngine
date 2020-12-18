// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#pragma once

// STL
#include <string>

// Custom
#include "SilentEngine/Private/EntityComponentSystem/SComponent/SComponent.h"
#include "SilentEngine/Public/SPrimitiveShapeGenerator/SPrimitiveShapeGenerator.h"


//@@Class
/*
This class is a dummy component that can be used to, for example, track local location offset from some component,
or, for example, represent the forward vector direction of a parent component and etc.
*/
class STargetComponent : public SComponent
{
public:
	//@@Function
	STargetComponent(std::string sComponentName);

	STargetComponent() = delete;
	STargetComponent(const STargetComponent&) = delete;
	STargetComponent& operator= (const STargetComponent&) = delete;

	virtual ~STargetComponent() override;

private:

	//@@Function
	/*
	* desc: called when parent's location/rotation/scale are changed.
	*/
	virtual void updateMyAndChildsLocationRotationScale(bool bCalledOnSelf) override;
};

