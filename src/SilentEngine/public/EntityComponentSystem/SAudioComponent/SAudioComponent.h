// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#pragma once

// STL
#include <string>

// Engine
#include "SilentEngine/Private/EntityComponentSystem/SComponent/SComponent.h"
#include "SilentEngine/Private/AudioEngine/SSound/ssound.h"
#include "SilentEngine/Private/AudioEngine/SSoundMix/ssoundmix.h"

//@@Class
/*
This class represents an audio emitter for 2D and 3D sound.
*/
class SAudioComponent : public SComponent
{
public:

	//@@Function
	SAudioComponent(const std::string& sComponentName, bool bIs3DSound);

	SAudioComponent() = delete;
	SAudioComponent(const SAudioComponent&) = delete;
	SAudioComponent& operator= (const SAudioComponent&) = delete;

	//@@Function
	/*
	* desc: returns the sound that this component controls.
	*/
	class SSound* getSound();

	virtual ~SAudioComponent() override;

private:

	friend class SAudioEngine;
	friend class SSound;

	//@@Function
	/*
	* desc: called when parent's location/rotation/scale are changed.
	*/
	virtual void updateMyAndChildsLocationRotationScale(bool bCalledOnSelf) override;


	// --------------------------------------------------


	SSound* pSound = nullptr;
};

