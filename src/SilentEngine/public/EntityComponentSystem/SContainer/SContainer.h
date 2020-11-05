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

// Custom
#include "SilentEngine/Public/SMouseKey/SMouseKey.h"
#include "SilentEngine/Public/SKeyboardKey/SKeyboardKey.h"
#include "SilentEngine/Public/SVector/SVector.h"


class SComponent;
class SFrameResource;

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
	* remarks: don't delete added (addComponentToContainer()) components in destructor, they will be deleted when
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
	* desc: determines if the overridable functions like onKeyboardButtonDown() will be called in the container.
	* param "bEnableUserInputCalls": false by default. True to enable, false otherwise.
	*/
	void setEnableUserInputCalls (bool bEnableUserInputCalls);
	//@@Function
	/*
	* desc: determines if the overridable function onTick() will be called in the container.
	* param "bCallTick": false by default. True to enable, false otherwise.
	*/
	void setCallTick             (bool bCallTick);
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
	* desc: determines if the overridable functions like onKeyboardButtonDown() are called in the container.
	* return: true if enabled, false otherwise.
	*/
	bool        isUserInputCallsEnabled() const;
	//@@Function
	/*
	* desc: determines if the container and all of its components are visible (i.e. drawn).
	* return: true if visible, false otherwise.
	*/
	bool        isVisible        () const;
	//@@Function
	/*
	* desc: determines if the overridable function onTick() is called in the container.
	* return: true if onTick() is called, false otherwise.
	*/
	bool        getCallTick      () const;
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


protected:

	// Game

		//@@Function
		/*
		* desc: an overridable function, called (if setCallTick() was set to true) every time before a frame is drawn.
		* param "fDeltaTime": the time that has passed since the last onTick() call.
		This value will be valid even if you just enabled onTick() calls.
		*/
		virtual void onTick(float fDeltaTime) {};

	// Input

		//@@Function
		/*
		* desc: an overridable function, called (if setEnableUserInputCalls() was set to true) when the user presses a mouse key.
		* param "mouseKey": the mouse key that has been pressed.
		* param "iMouseXPos": mouse X position in pixels.
		* param "iMouseYPos": mouse Y position in pixels.
		*/
		virtual void onMouseDown(SMouseKey mouseKey, int iMouseXPos, int iMouseYPos) {};
		//@@Function
		/*
		* desc: an overridable function, called (if setEnableUserInputCalls() was set to true) when the user unpresses a mouse key.
		* param "mouseKey": the mouse key that has been unpressed.
		* param "iMouseXPos": mouse X position in pixels.
		* param "iMouseYPos": mouse Y position in pixels.
		*/
		virtual void onMouseUp  (SMouseKey mouseKey, int iMouseXPos, int iMouseYPos) {};
		//@@Function
		/*
		* desc: an overridable function, called (if setEnableUserInputCalls() was set to true) when the user moves the mouse.
		* param "iMouseXMove": the number in pixels by which the mouse was moved (by X-axis).
		* param "iMouseYMove": the number in pixels by which the mouse was moved (by Y-axis).
		*/
		virtual void onMouseMove(int iMouseXMove, int iMouseYMove) {};
		//@@Function
		/*
		* desc: an overridable function, called (if setEnableUserInputCalls() was set to true) when the user moves the mouse wheel.
		* param "bUp": true if the mouse wheel was moved up (forward, away from the user), false otherwise.
		* param "iMouseXPos": mouse X position in pixels.
		* param "iMouseYPos": mouse Y position in pixels.
		*/
		virtual void onMouseWheelMove(bool bUp, int iMouseXPos, int iMouseYPos) {};

		//@@Function
		/*
		* desc: an overridable function, called (if setEnableUserInputCalls() was set to true) when the user presses a keyboard key.
		* param "keyboardKey": a keyboard key that was pressed.
		*/
		virtual void onKeyboardButtonDown  (SKeyboardKey keyboardKey) {};
		//@@Function
		/*
		* desc: an overridable function, called (if setEnableUserInputCalls() was set to true) when the user unpresses a keyboard key.
		* param "keyboardKey": a keyboard key that was unpressed.
		*/
		virtual void onKeyboardButtonUp    (SKeyboardKey keyboardKey) {};

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
	//@@Function
	/*
	* desc: removes the vertex buffer for only runtime mesh components for given frame resources.
	*/
	void removeVertexBufferForRuntimeMeshComponents (std::vector<std::unique_ptr<SFrameResource>>* vFrameResources, size_t& iRemovedCount);
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


	bool bEnableUserInputCalls;
	bool bVisible;
	bool bSpawnedInLevel;
	bool bCallTick;
	bool bIsEditorObject;
};

