// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#pragma once

// STL
#include <vector>

// Custom
#include "SilentEngine/Public/SVector/SVector.h"

class SApplication;
class SContainer;

//@@Class
/*
The class represents a level that can have containers in it.
*/
class SLevel
{
public:
	//@@Function
	SLevel(SApplication* pApp);
	SLevel() = delete;
	SLevel(const SLevel&) = delete;
	SLevel& operator= (const SLevel&) = delete;
	virtual ~SLevel();


	// Set functions

		//@@Function
		/*
		* desc: used to spawn container in level.
		* param "pContainer": the pointer to a container that you want to spawn.
		* return: false if successful, true otherwise.
		* remarks: this function is thread-safe.
		It's recommended to use this function in loading moments of your application (ex. loading screen)
		as this function may drop the framerate a little.
		*/
		bool spawnContainerInLevel          (SContainer* pContainer);
		
		//@@Function
		/*
		* desc: used to despawn container in level.
		* param "pContainer": the pointer to a container that you want to despawn.
		* remarks: this function is thread-safe.
		It's recommended to use this function in loading moments of your application (ex. loading screen)
		as this function may drop the framerate a little.
		*/
		void despawnContainerFromLevel      (SContainer* pContainer);

		//@@Function
		/*
		* desc: used to set the ambient light intensity.
		* param "vRBG": rgb color vector.
		*/
		void setAmbientLight                (SVector vRGB);


	// Get functions

		//@@Function
		/*
		* desc: returns all renderable containers (containers that have components with
		geometry in it, for example, SMeshComponent) in level.
		* param "pvRenderableContainers": the pointer to your std::vector that will be filled with
		pointers to the renderable containers in level.
		*/
		void getRenderableContainers    (std::vector<SContainer*>*& pvRenderableContainers);

		//@@Function
		/*
		* desc: returns all non-renderable containers (containers that don't have components with
		geometry in it, for example, STargetComponent) in level.
		* param "pvNotRenderableContainers": the pointer to your std::vector that will be filled with
		pointers to the non-renderable containers in level.
		*/
		void getNotRenderableContainers (std::vector<SContainer*>*& pvNotRenderableContainers);

		//@@Function
		/*
		* desc: used to retrieve the ambient light intensity.
		* return: rgb color vector.
		*/
		SVector getAmbientLight() const;


private:

	friend class SApplication;

	//@@Variable
	/* pointer to the SApplication that owns the level. */
	SApplication* pApp;


	//@@Variable
	/* ambient light on the level. */
	SVector vAmbientLight;



	// Containers.

	//@@Variable
	/* renderable containers (containers that have components with geometry in it, for example, SMeshComponent). */
	std::vector<SContainer*> vRenderableContainers;
	//@@Variable
	/* non-renderable containers (containers that don't have components with geometry in it, for example, STargetComponent). */
	std::vector<SContainer*> vNotRenderableContainers;

	std::vector<class SLightComponent*> vSpawnedLightComponents;
};

