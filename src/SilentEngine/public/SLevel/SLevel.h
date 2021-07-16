// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#pragma once

// STL
#include <vector>
#include <mutex>

// DirectX
#include <DirectXCollision.h>

// Custom
#include "SilentEngine/Public/SVector/SVector.h"

class SApplication;
class SContainer;
class SMeshComponent;
class SComponent;

class SRayCastHit
{
public:
	SComponent* pHitComponent;
	float fHitDistanceFromRayOrigin; // distance between ray origin and collision bound (not mesh origin)
	SVector vHitNormal; // normal vector of hit point
	size_t vHitTriangleIndices[3]; // indices of hit triangle, use pMeshData->getVertices() with those indices to get triangle
};

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


		//@@Function
		/*
		* desc: used to emit a ray that hits the components with collision.
		* param "vRayStartPos": start position of the ray.
		* param "vRayStopPos": stop (end) position of the ray.
		* param "vHitResult": objects that were hit by the ray, use SComponent::getComponentType() to cast to correct type.
		* param "vIgnoreList": (optional) list of component that will be ignored in this ray cast.
		* remarks: instanced mesh components are currently ignored (TODO).
		*/
		void rayCast(SVector& vRayStartPos, SVector& vRayStopPos, std::vector<SRayCastHit>& vHitResult, const std::vector<SComponent*>& vIgnoreList = std::vector<SComponent*>());


	// Set functions

		//@@Function
		/*
		* desc: used to spawn container in level.
		* param "pContainer": the pointer to a container that you want to spawn.
		* return: false if successful (container is spawned), true otherwise.
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


	// Get functions

		//@@Function
		/*
		* desc: returns bounding sphere of the level, level bounds are used in directional lights
		(there will be no shadow calculations outside of this bounding sphere).
		* param "bRecalculateLevelBounds": 'true' will recalculate the level bounds iterating over every object in the scene
		(that may take a while if there are many objects in this level so be careful, it's recommended to call this function once per level
		after the level was fully constructed). 'False' will just return the last bouding sphere calculated (no calculations will be performed).
		*/
		DirectX::BoundingSphere* getLevelBounds(bool bRecalculateLevelBounds);

		//@@Function
		/*
		* desc: returns all renderable containers (containers that have components with
		geometry in it, for example, SMeshComponent) in level.
		*/
		std::vector<SContainer*>* getRenderableContainers    ();

		//@@Function
		/*
		* desc: returns all non-renderable containers (containers that don't have components with
		geometry in it, for example, STargetComponent) in level.
		*/
		std::vector<SContainer*>* getNotRenderableContainers ();


private:

	friend class SApplication;

	//@@Variable
	/* pointer to the SApplication that owns the level. */
	SApplication* pApp;


	// Containers.

	//@@Variable
	/* renderable containers (containers that have components with geometry in it, for example, SMeshComponent). */
	std::vector<SContainer*> vRenderableContainers;
	//@@Variable
	/* non-renderable containers (containers that don't have components with geometry in it, for example, STargetComponent). */
	std::vector<SContainer*> vNotRenderableContainers;

	std::vector<class SLightComponent*> vSpawnedLightComponents;


	std::mutex mtxLevelBounds;
	DirectX::BoundingSphere levelBounds;
};

