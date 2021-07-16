// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#pragma once

// STL
#include <string>
#include <vector>
#include <memory>

// DirectX
#include <d3d12.h>
#include "SilentEngine/Private/d3dx12.h"

// Custom
#include "SilentEngine/Public/SMouseKey/SMouseKey.h"
#include "SilentEngine/Public/SKeyboardKey/SKeyboardKey.h"
#include "SilentEngine/Public/SVector/SVector.h"


class SComponent;
class SFrameResource;
class SShaderObjects;

//@@Class
/*
The class represents an Entity (in Entity Component System) which can hold components.
The container is considered as a root component of all components inside the container.
*/
class SContainer
{
public:
	//@@Function
	SContainer(const std::string& sContainerName);

	SContainer() = delete;
	SContainer(const SContainer&) = delete;
	SContainer& operator= (const SContainer&) = delete;

	//@@Function
	/*
	* remarks: spawned container will be despawned, all components will be deleted.
	It's recommended to mark your SContainer's destructor as virtual and override.
	*/
	virtual ~SContainer();

	//@@Function
	/*
	* desc: sets the location for the container and its components maintaining the local position of the components relative to the container.
	* param "vNewLocation": location vector.
	* param "bLocationInWorldCoordinateSystem": if false, then the location is given in the container's local coordinate system.
	*/
	void setLocation(const SVector& vNewLocation, bool bLocationInWorldCoordinateSystem = true);
	//@@Function
	/*
	* desc: sets the rotation for the container and its components maintaining the local rotation of the components.
	* param "vNewRotation": rotation vector.
	*/
	void setRotation(const SVector& vNewRotation);
	//@@Function
	/*
	* desc: sets the scale for the container and its components maintaining the local scale of the components.
	* param "vNewScale": scale vector.
	*/
	void setScale   (const SVector& vNewScale);

	//@@Function
	/*
	* desc: adds component to container.
	* param "pComponent": component to add.
	* return: false if successful, true otherwise.
	* remarks: this function will reset component's local location.
	Don't delete added (addComponentToContainer()) components in destructor, they will be deleted when
	parent's destructor will be called (after your SContainer's destructor). Moreover, it's recommended to mark your SContainer's destructor
	as virtual and override.
	*/
	bool addComponentToContainer      (SComponent* pComponent);
	//@@Function
	/*
	* desc: removes component from container.
	* param "pComponent": component to add.
	* return: false if successful, true otherwise.
	* remarks: don't use removeComponentFromContainer() in destructor, don't worry, the components will be deleted.
	Moreover, it's recommended to mark your SContainer's destructor as virtual and override.
	*/
	bool removeComponentFromContainer (SComponent* pComponent);
	//@@Function
	/*
	* desc: determines if the container and all of its components should be visible (i.e. drawn).
	* param "bVisible": true by default.
	*/
	void setVisibility           (bool bVisible);
	//@@Function
	/*
	* desc: used to set the container name. This name should be unique when spawning the container in level.
	* param "sContainerName": the name of the container.
	*/
	bool setContainerName        (const std::string& sContainerName);

	//@@Function
	/*
	* desc: unbinds used materials from all components and child components.
	*/
	void unbindMaterialsFromAllComponents();
	//@@Function
	/*
	* desc: determines if the container and all of its components are visible (i.e. drawn).
	* return: true if visible, false otherwise.
	*/
	bool        isVisible        () const;
	//@@Function
	/*
	* desc: returns the container name.
	*/
	std::string getContainerName () const;

	//@@Function
	/*
	* desc: returns the location of the container.
	*/
	SVector     getLocation      () const;
	//@@Function
	/*
	* desc: returns the rotation of the container.
	*/
	SVector     getRotation      () const;
	//@@Function
	/*
	* desc: returns the scale of the container.
	*/
	SVector     getScale         () const;
	//@@Function
	/*
	* desc: returns the local axis vectors of the container.
	* param "pvXAxisVector": the pointer to your SVector instance, this pointer can be nullptr.
	* param "pvYAxisVector": the pointer to your SVector instance, this pointer can be nullptr.
	* param "pvZAxisVector": the pointer to your SVector instance, this pointer can be nullptr.
	*/
	void        getLocalAxis (SVector* pvXAxisVector, SVector* pvYAxisVector, SVector* pvZAxisVector) const;

	//@@Function
	/*
	* desc: returns the pointer to the component with the specified component name (searching through the entire component tree).
	The returned pointer can be nullptr.
	*/
	SComponent* getComponentByName(const std::string& sComponentName) const;
	//@@Function
	/*
	* desc: returns the std::vector containing the pointers to all "root" components (i.e. components that were added using addComponentToContainer()).
	*/
	std::vector<SComponent*> getComponents() const;

	//@@Function
	/*
	* desc: returns true if this container was created using Silent Editor.
	*/
	bool        isEditorObject   () const;

private:

	friend class SApplication;
	friend class SComponent;
	friend class SRuntimeMeshComponent;

	//@@Function
	/*
	* desc: true if the container is spawned in the level, false otherwise.
	*/
	void setSpawnedInLevel        (bool bSpawned);
	//@@Function
	/*
	* desc: sets the starting CB index for this container.
	*/
	void setStartIndexInCB        (size_t iStartIndex);
	//@@Function
	/*
	* desc: returns all opaque and transparent mesh components.
	*/
	void getAllMeshComponents(std::vector<SComponent*>* pvOpaqueComponents, std::vector<SComponent*>* pvTransparentComponents);
	//@@Function
	/*
	* desc: creates the vertex buffer for only runtime mesh components for given frame resource.
	*/
	void createVertexBufferForRuntimeMeshComponents (SFrameResource* pFrameResource);
	size_t getLightComponentsCount();
	//@@Function
	/*
	* desc: asks every light component on how much shadows maps they need.
	*/
	void getRequiredDSVCountForShadowMaps(size_t& iDSVCount);
	//@@Function
	/*
	* desc: creates the instancing data.
	*/
	void createInstancingDataForFrameResource       (std::vector<std::unique_ptr<SFrameResource>>* vFrameResources);
	//@@Function
	/*
	* desc: removes the vertex buffer for only runtime mesh components for given frame resources.
	*/
	void removeVertexBufferForRuntimeMeshComponents (std::vector<std::unique_ptr<SFrameResource>>* vFrameResources, size_t& iRemovedCount);
	//@@Function
	/*
	* desc: removes the instancing data.
	*/
	void removeInstancingDataForFrameResources(std::vector<std::unique_ptr<SFrameResource>>* vFrameResources);
	//@@Function
	/*
	* desc: removes the shadow map buffer for only light components for given frame resource.
	*/
	void deallocateShadowMapCBsForLightComponents(std::vector<std::unique_ptr<SFrameResource>>* vFrameResources);
	//@@Function
	/*
	* desc: cycles through all runtime mesh components and updates the iMaxIndex to be the maximum runtime mesh component CB index in this container.
	*/
	void getMaxVertexBufferIndexForRuntimeMeshComponents(size_t& iMaxIndex);
	//@@Function
	/*
	* desc: decreases the vertex buffer index for all runtime mesh components on specified value.
	*/
	void updateVertexBufferIndexForRuntimeMeshComponents(size_t iIfIndexMoreThatThisValue, size_t iMinusValue);
	//@@Function
	/*
	* desc: returns the number of mesh components (mesh and runtime mesh components) (even in child components).
	*/
	size_t getMeshComponentsCount () const;
	//@@Function
	/*
	* desc: returns iStartIndexCB.
	*/
	size_t getStartIndexInCB      () const;
	//@@Function
	/*
	* desc: adds meshes to vectors based on their transparency if they use custom shader.
	*/
	void addMeshesByShader(std::vector<SShaderObjects>* pOpaqueMeshesByShader, std::vector<SShaderObjects>* pTransparentMeshesByShader) const;
	//@@Function
	/*
	* desc: removes meshes from vectors based on their transparency if they use custom shader.
	*/
	void removeMeshesByShader(std::vector<SShaderObjects>* pOpaqueMeshesByShader, std::vector<SShaderObjects>* pTransparentMeshesByShader) const;
	//@@Function
	/*
	* desc: registers all SAudioComponents that use 3D sound to SAudioEngine so that we can update their 3D sound position in onTick().
	*/
	void registerAll3DSoundComponents();
	//@@Function
	/*
	* desc: unregisters all SAudioComponents that use 3D sound from SAudioEngine so that we know that we no longer need to update
	their 3D sound position in onTick().
	*/
	void unregisterAll3DSoundComponents();


	// -----------------------------------------------


	std::vector<SComponent*> vComponents;


	SVector     vLocation;
	SVector     vRotation;
	SVector     vScale;

	SVector     vLocalXAxisVector;
	SVector     vLocalYAxisVector;
	SVector     vLocalZAxisVector;


	std::string sContainerName;


	size_t iStartIndexCB;
	size_t iMeshComponentsCount;


	bool bVisible;
	bool bSpawnedInLevel;
	bool bIsEditorObject;
};