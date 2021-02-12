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
#include "SilentEngine/Private/EntityComponentSystem/SComponent/SComponent.h"
#include "SilentEngine/Private/SRenderItem/SRenderItem.h"
#include "SilentEngine/Public/SPrimitiveShapeGenerator/SPrimitiveShapeGenerator.h"
#include "SilentEngine/Public/SMaterial/SMaterial.h"

//@@Class
/*
This class represents the component that has some mesh data (3D-geometry) inside of it, but unlike the SMeshComponent,
SRuntimeMeshComponent is used for 3D-geometry that is changing its data very often (using setMeshData(), every frame for example).
This component has some optimizations so changing mesh data very often will be more efficient than SMeshComponent and will have
less influence on the performance. Please note that if you're using compute shader to update mesh data, it may be more efficient to use SMeshComponent 
(and so it's recommended). SRuntimeMeshComponent has CPU optimizations so updating mesh data from CPU is faster with this component.
*/
class SRuntimeMeshComponent : public SComponent
{
public:

	//@@Function
	/*
	* desc: constructor function.
	* param "bDisableFrustumCulling": set to true if the mesh data of this component will be changing very rapidly (like moving particles for example),
	and so we won't recalculate the object's bounds on every setMeshData() call for frustum culling (which will disable the frustum culling
	for this component only, but slightly reduce the CPU load and... well... increase the GPU load - you should probably benchmark
	this and see what's better for your game). You can change this setting using the setDisableFrustumCulling() function.
	*/
	SRuntimeMeshComponent(std::string sComponentName, bool bDisableFrustumCulling);

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
	* desc: set to true if the mesh data of this component will be changing very rapidly (like moving particles for example),
	and so we won't recalculate the object's bounds on every setMeshData() call for frustum culling (which will disable the frustum culling
	for this component only, but slightly reduce the CPU load and... well... increase the GPU load - you should probably benchmark
	this and see what's better for your game). Passing the 'false' will recalculate mesh bounds if there are any mesh data.
	*/
	void setDisableFrustumCulling(bool bDisable);

	//@@Function
	/*
	* desc: determines if the material on this component should consider the alpha channel
	of the diffuse texture or a custom value from SMaterial::setCustomTransparency().
	* return: false if successful, true if this component is spawned.
	* remarks: this value can only be changed before this component is spawned.
	Transparency is disabled by default.
	*/
	bool setEnableTransparency(bool bEnable);

	//@@Function
	/*
	* desc: used to assign the custom shader that this mesh will use.
	* param "bForceChangeEvenIfSpawned": forces the engine to change the shader even if this mesh is spawned (might cause small fps drop).
	* remarks: use SApplication::compileCustomShader() to compile custom shaders first.
	*/
	void setUseCustomShader(SShader* pCustomShader, bool bForceChangeEvenIfSpawned = false);

	//@@Function
	/*
	* desc: used to set the 3D-geometry that will be drawn ones the container containing this component is spawned and visible.
	* param "meshData": 3D-geometry data.
	* param "bAddedRemovedVerticesOrAddedRemovedIndices": this should be set to false if the new mesh data
	contains the SAME AMOUNT of indices and vertices as the previous one. So if the new mesh data contains the same amount of
	vertices but they have different positions in space then of course this value should be false. Example: if you have a
	SRuntimeMeshComponent with a terrain plane mesh and you want to only change the positions of the vertices then
	this value should be false. Setting this value to true all the time will almost fully nullify all optimizations that SRuntimeMeshComponent has.
	Passing the 'true' will pause frame drawing as we need to recreate some GPU buffers (and so will cause small fps drops).
	* remarks: this function is thread-safe (you can call it from any thread).
	*/
	void setMeshData           (const SMeshData& meshData, bool bAddedRemovedVerticesOrAddedRemovedIndices);

	//@@Function
	/*
	* desc: unbinds the material from the component so that this component will use default engine material.
	* remarks: note that this function will not unregister the material, you should do it by yourself.
	*/
	void unbindMaterial();

	//@@Function
	/*
	* desc: used to switch the custom used shader to a default one.
	* param "bForceUseDefaultEvenIfSpawned": forces the engine to change the shader even if this mesh is spawned (causes small fps drop).
	* return: false if successful, true if this mesh is spawned and the 'bForceUseDefaultEvenIfSpawned' is false.
	*/
	bool setUseDefaultShader(bool bForceUseDefaultEvenIfSpawned = false);

	//@@Function
	/*
	* desc: used to set the material of the mesh.
	* return: false if successful, true if the material is not registered using SApplication::registerMaterial() function.
	*/
	bool setMeshMaterial       (SMaterial* pMaterial);

	//@@Function
	/*
	* desc: used to set the cull distance, if the distance between the camera and the mesh origin point will be
	equal or more that this value then the mesh will not be drawn.
	*/
	void setCullDistance(float fCullDistance);

	//@@Function
	/*
	* desc: used to set the UV offset to the mesh texture. Only affects how the textures will look for THIS mesh.
	This setting will affect material UV settings only for this mesh.
	* return: true if the UVs are not in [0, 1] range, false otherwise.
	* remarks: should be in [0, 1] range.
	*/
	bool      setMeshTextureUVOffset(const SVector& vMeshTexUVOffset);

	//@@Function
	/*
	* desc: used to set the UV scale to the mesh texture. Only affects how the textures will look for THIS mesh.
	This setting will affect material UV settings only for this mesh.
	*/
	void      setTextureUVScale(const SVector& vTextureUVScale);

	//@@Function
	/*
	* desc: used to set the UV rotation to the mesh texture. Only affects how the textures will look for THIS mesh.
	This setting will affect material UV settings only for this mesh.
	*/
	void      setTextureUVRotation(float fRotation);

	//@@Function
	/*
	* desc: used to set the iCustomProperty member of the object's constant buffer (in HLSL).
	*/
	void      setCustomShaderProperty(unsigned int iCustomProperty);

	//@@Function
	/*
	* desc: used to retrieve the mesh material.
	* return: valid pointer if the material was assigned before, nullptr otherwise.
	*/
	SMaterial* getMeshMaterial();

	//@@Function
	/*
	* desc: used to retrieve the cull distance.
	* return: returns a negative value if the cull distance was not set.
	*/
	float      getCullDistance() const;

	//@@Function
	/*
	* desc: returns true if the transparency for this component is enabled.
	* remarks: transparency is disabled by default.
	*/
	bool     getEnableTransparency() const;

	//@@Function
	/*
	* desc: returns the UV offset of the mesh texture.
	*/
	SVector  getTextureUVOffset() const;

	//@@Function
	/*
	* desc: returns the UV scale of the mesh texture.
	*/
	SVector   getTextureUVScale() const;

	//@@Function
	/*
	* desc: returns the UV rotation of the mesh texture.
	*/
	float     getTextureUVRotation() const;

	//@@Function
	/*
	* desc: used to retrieve the mesh data.
	*/
	SMeshData* getMeshData();

	//@@Function
	/*
	* desc: used to retrieve the custom shader.
	* return: nullptr if the custom shader was not assigned, valid pointer otherwise.
	*/
	SShader* getCustomShader() const;

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

	virtual void unbindMaterialsIncludingChilds() override;

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
	virtual void updateMyAndChildsLocationRotationScale(bool bCalledOnSelf) override;

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

	void updateObjectCenter();

	// ------------------------------------------

	std::mutex  mtxDrawComponent;

	size_t      iIndexInFrameResourceVertexBuffer;

	bool        bIndexOfVertexBufferValid;
	bool        bNoMeshDataOnSpawn;
	bool        bNewMeshData;
	bool        bVisible;
	bool        bDisableFrustumCulling;
};

