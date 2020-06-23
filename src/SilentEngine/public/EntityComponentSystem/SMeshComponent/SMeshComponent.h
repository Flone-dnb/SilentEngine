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
#include "SilentEngine/Public/EntityComponentSystem/SComponent/SComponent.h"
#include "SilentEngine/Public/SPrimitiveShapeGenerator/SPrimitiveShapeGenerator.h"

//@@Class
/*
This class represents the component that has some mesh data (3D-geometry) inside of it.
*/
class SMeshComponent : public SComponent
{
public:

	//@@Function
	SMeshComponent(std::string sComponentName);

	SMeshComponent() = delete;
	SMeshComponent(const SMeshComponent&) = delete;
	SMeshComponent& operator= (const SMeshComponent&) = delete;

	virtual ~SMeshComponent() override;



	//@@Function
	/*
	* desc: determines if the component should be visible (i.e. drawn).
	* param "bVisible": true by default.
	*/
	void setVisibility         (bool bVisible);

	//@@Function
	/*
	* desc: used to set the 3D-geometry that will be drawn ones the container containing this component is spawned and visible.
	* param "meshData": 3D-geometry data.
	* param "bAddedVerticesOrUpdatedIndicesCount": this should be set to false if the new mesh data
	contains the SAME AMOUNT of indices and vertices as the previous one. So if the new mesh data contains the same amount of
	vertices but they have different positions in space then of course this value should be false. So if you have a SMeshComponent
	with a box 3D-data and you want to only change the color or the size of this box then this value should be false.
	This is just an optimization not to create a new index buffer on every setMeshData() call. You can just set this value to true every time, but it's
	recommended to consider this optimization.
	* param "bUpdateBoundingBox": TODO.
	* remarks: this function is thread-safe (you can call it from any thread).
	*/
	void setMeshData           (const SMeshData& meshData, bool bAddedVerticesOrUpdatedIndicesCount, bool bUpdateBoundingBox = false);

	//@@Function
	/*
	* desc: used to retrieve the mesh data.
	*/
	SMeshData getMeshData      () const;
	//@@Function
	/*
	* desc: determines if the component is visible (i.e. drawn).
	* return: true if visible, false otherwise.
	*/
	bool isVisible             () const;

private:

	//@@Function
	/*
	* desc: returns the SRenderItem data.
	*/
	virtual SRenderItem* getRenderData() override;

	friend class SApplication;
	friend class SComponent;
	friend class SContainer;

	//@@Function
	/*
	* desc: creates vertex & index buffer (if bAddedVerticesOrUpdatedIndicesCount is true).
	*/
	void createGeometryBuffers (bool bAddedVerticesOrUpdatedIndicesCount);
	//@@Function
	/*
	* desc: updates the world matrix using getWorldMatrix().
	*/
	void updateWorldMatrix     ();

	//@@Function
	/*
	* desc: called when parent's location/rotation/scale are changed.
	*/
	void updateMyAndChildsLocationRotationScale();

	// ------------------------------------------

	SMeshData   meshData;
	DirectX::BoundingBox bounds;

	bool        bVisible;
};

