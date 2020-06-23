// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#pragma once

// STL
#include <string>
#include <mutex>
#include <memory>

// Custom
#include "SilentEngine/Public/EntityComponentSystem/SComponent/SComponent.h"
#include "SilentEngine/Private/SGeometry/SGeometry.h"
#include "SilentEngine/Public/SPrimitiveShapeGenerator/SPrimitiveShapeGenerator.h"

//@@Class
/*
This class represents the component that has some mesh data (3D-geometry) inside of it, but unlike the SMeshComponent,
SRuntimeMeshComponent is used for 3D-geometry that is changing its data very often (using setMeshData(), every frame for example).
This component has some optimizations so changing mesh data very often will be more efficient than SMeshComponent and will have
less influence on the performance.
*/
class SRuntimeMeshComponent : public SComponent
{
public:

	//@@Function
	SRuntimeMeshComponent(std::string sComponentName);

	SRuntimeMeshComponent() = delete;
	SRuntimeMeshComponent(const SRuntimeMeshComponent&) = delete;
	SRuntimeMeshComponent& operator= (const SRuntimeMeshComponent&) = delete;

	virtual ~SRuntimeMeshComponent() override;


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
	vertices but they have different positions in space then of course this value should be false. So if you have a
	SRuntimeMeshComponent with a box 3D-data and you want to only change the color or the size of this box then
	this value should be false. Setting this value to true all the time will almost fully nullify all optimizations that SRuntimeMeshComponent has.
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

	friend class SApplication;
	friend class SComponent;
	friend class SContainer;

	//@@Function
	/*
	* desc: returns the SRenderItem data.
	*/
	virtual SRenderItem* getRenderData() override;

	//@@Function
	/*
	* desc: creates index buffer.
	*/
	void createIndexBuffer     ();
	//@@Function
	/*
	* desc: updates world matrix using getWorldMatrix().
	*/
	void updateWorldMatrix     ();

	//@@Function
	/*
	* desc: called when parent's location/rotation/scale are changed.
	*/
	void updateMyAndChildsLocationRotationScale();

	//@@Function
	/*
	* desc: add a vertex buffer to the specified frame resources.
	*/
	void addVertexBuffer       (SFrameResource* pFrameResource);
	//@@Function
	/*
	* desc: add the vertex buffers from all frame resources.
	*/
	void removeVertexBuffer    (std::vector<std::unique_ptr<SFrameResource>>* vFrameResources);
	//@@Function
	/*
	* desc: decreases the vertex buffer index value if index (iIndexInFrameResourceVertexBuffer) is more than
	iIfIndexMoreThatThisValue on iMinusValue value.
	*/
	void updateVertexBufferIndex(size_t iIfIndexMoreThatThisValue, size_t iMinusValue);
	//@@Function
	/*
	* desc: updates the iCurrentIndex to iIndexInFrameResourceVertexBuffer if iIndexInFrameResourceVertexBuffer > iCurrentIndex.
	This function is used to then later we can call updateVertexBufferIndex() with this max value.
	*/
	void updateVertexBufferMaxIndex(size_t& iCurrentIndex);

	// ------------------------------------------

	SMeshData   meshData;
	SRenderItem renderData;
	DirectX::BoundingBox bounds;

	std::mutex  mtxDrawComponent;

	size_t      iIndexInFrameResourceVertexBuffer;

	bool        bIndexOfVertexBufferValid;
	bool        bNoMeshDataOnSpawn;
	bool        bVisible;
};

