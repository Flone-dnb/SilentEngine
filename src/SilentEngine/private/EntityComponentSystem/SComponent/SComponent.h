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
#include <mutex>

// Custom
#include "SilentEngine/Public/SVector/SVector.h"
#include "SilentEngine/Private/SGeometry/SGeometry.h"
#include "SilentEngine/Public/SPrimitiveShapeGenerator/SPrimitiveShapeGenerator.h"

enum SComponentType
{
	SCT_NONE = 0,
	SCT_MESH = 1,
	SCT_RUNTIME_MESH = 2,
	SCT_LIGHT = 3
};

class SContainer;
class SFrameResource;

//@@Class
/*
The class represents a Component (in Entity Component System).
A component may have child components.
*/
class SComponent
{
public:
	//@@Function
	SComponent();
	SComponent(const SComponent&) = delete;
	SComponent& operator= (const SComponent&) = delete;
	virtual ~SComponent();


	//@@Function
	/*
	* desc: adds a child component.
	* return: false if successful, true otherwise.
	* remarks: 1) if the container, containing these components has already been spawned then you cannot add/remove components;
	2) if this component is not a part of any container then you can't add a child component to this component;
	3) if the child component does not have a unique name (within the boundaries of this container) then this function will fail.
	*/
	bool addChildComponent    (SComponent* pComponent);
	//@@Function
	/*
	* desc: removes a child component.
	* return: false if successful, true otherwise.
	* remarks: 1) if the container, containing these components has already been spawned then you cannot add/remove components;
	2) if this component is not a part of any container then you can't add a child component to this component;
	3) if the child component does not have a unique name (within the boundaries of this container) then this function will fail.
	*/
	bool removeChildComponent (SComponent* pComponent);


	//@@Function
	/*
	* desc: sets the location relative to the parent's coordinate system. The parent is a component if this component
	is a child component, and the parent is a container if this component is not a child component.
	* remarks: the local location/rotation/scale of any child components will be preserved.
	*/
	virtual void setLocalLocation (const SVector& location);
	//@@Function
	/*
	* desc: sets the rotation.
	* remarks: the local location/rotation/scale of any child components will be preserved.
	*/
	virtual void setLocalRotation (const SVector& rotation);
	//@@Function
	/*
	* desc: sets the scale.
	* remarks: the local location/rotation/scale of any child components will be preserved.
	*/
	virtual void setLocalScale    (const SVector& scale);

	//@@Function
	/*
	* desc: sets the name of this component. This name should be unique when adding this component to the container or to the other component.
	*/
	bool setComponentName               (const std::string& sComponentName);

	//@@Function
	/*
	* desc: returns the name of the component.
	*/
	std::string getComponentName               () const;
	//@@Function
	/*
	* desc: returns the pointer to the parent component (nullptr if there is no parent).
	*/
	SComponent* getParentComponent             () const;
	//@@Function
	/*
	* desc: returns the pointer to the child component with the specified component name (searching through the entire child component tree).
	The returned pointer can be nullptr.
	*/
	SComponent* getChildComponentByName        (const std::string& sComponentName) const;
	//@@Function
	/*
	* desc: returns the pointer to the container. The returned pointer can be nullptr.
	*/
	SContainer* getContainer                   () const;
	//@@Function
	/*
	* desc: returns the std::vector containing the pointers to all child components (i.e. components that were added using addChildComponent()
	to this component).
	*/
	std::vector<SComponent*> getChildComponents() const;

	//@@Function
	/*
	* desc: returns the location in world coordinate system.
	*/
	SVector     getLocationInWorld                                  ();
	//@@Function
	/*
	* desc: returns the local location.
	*/
	SVector     getLocation                                         () const;
	//@@Function
	/*
	* desc: returns the local scale.
	*/
	SVector     getScale                                            () const;
	//@@Function
	/*
	* desc: returns the local rotation.
	*/
	SVector     getRotation                                         () const;
	//@@Function
	/*
	* desc: returns the local axis vectors of the component.
	* param "pvXAxisVector": the pointer to your SVector instance, this pointer can be nullptr.
	* param "pvYAxisVector": the pointer to your SVector instance, this pointer can be nullptr.
	* param "pvZAxisVector": the pointer to your SVector instance, this pointer can be nullptr.
	*/
	void        getComponentLocalAxis                               (SVector* pvXAxisVector, SVector* pvYAxisVector, SVector* pvZAxisVector) const;

protected:

	//@@Function
	/*
	* desc: returns the SRenderItem data if this component has geometry in it (SMeshComponent for example), nullptr otherwise.
	*/
	virtual SRenderItem* getRenderData() {return nullptr;};

	//@@Function
	/*
	* desc: returns the number of mesh components (mesh and runtime mesh components) (even in child components).
	*/
	size_t      getMeshComponentsCount         () const;

	size_t      getLightComponentsCount        () const;

	void        addLightComponentsToVector     (std::vector<class SLightComponent*>& vLights);
	void        removeLightComponentsFromVector(std::vector<class SLightComponent*>& vLights);

	//@@Function
	/*
	* desc: true if the component is spawned in the level, false otherwise.
	*/
	void setSpawnedInLevel                (bool bSpawned);
	//@@Function
	/*
	* desc: sets the renderData.iUpdateCBInFrameResourceCount to SFRAME_RES_COUNT for all mesh components (runtime mesh and etc.)
	so they will update their constant buffer data in the next frame.
	*/
	void setUpdateCBForEveryMeshComponent ();
	//@@Function
	/*
	* desc: used to set the index of constant buffer for mesh-like components.
	*/
	void setCBIndexForMeshComponents      (size_t* iIndex, bool bCreateBuffers = true);
	//@@Function
	/*
	* desc: sets the container.
	*/
	void setContainer                     (SContainer* pContainer);
	//@@Function
	/*
	* desc: sets the parent component.
	*/
	void setParentComponent               (SComponent* pComponent);

	//@@Function
	/*
	* desc: creates the vertex buffer for only runtime mesh components for given frame resource.
	*/
	void createVertexBufferForRuntimeMeshComponents(SFrameResource* pFrameResource);
	//@@Function
	/*
	* desc: removes the vertex buffer for only runtime mesh components for given frame resources.
	*/
	void removeVertexBufferForRuntimeMeshComponents(std::vector<std::unique_ptr<SFrameResource>>* vFrameResources, size_t& iRemovedCount);
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
	* desc: called when parent's location/rotation/scale are changed.
	*/
	virtual void updateMyAndChildsLocationRotationScale () = 0;

	//@@Function
	/*
	* desc: returns the world matrix (that includes parents).
	*/
	DirectX::XMMATRIX XM_CALLCONV getWorldMatrix();


	friend class SApplication;
	friend class SContainer;
	friend class SMeshComponent;
	friend class SRuntimeMeshComponent;


	SComponent* pParentComponent;
	SContainer* pContainer;


	std::vector<SComponent*> vChildComponents;


	SVector     vLocation;
	SVector     vRotation;
	SVector     vScale;

	SVector     vLocalXAxisVector;
	SVector     vLocalYAxisVector;
	SVector     vLocalZAxisVector;

	SComponentType componentType;


	std::mutex  mtxComponentProps;


	SRenderItem renderData; // will be null for components that doesn't have mesh data
	SMeshData   meshData; // will be null for components that doesn't have mesh data


	std::string sComponentName;


	size_t iMeshComponentsCount;


	bool bSpawnedInLevel;
};

