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

class SShader;

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
	void setUseCustomShader   (SShader* pCustomShader, bool bForceChangeEvenIfSpawned = false);

	//@@Function
	/*
	* desc: used to set the 3D-geometry that will be drawn ones the container containing this component is spawned and visible.
	* param "meshData": 3D-geometry data.
	* param "bAddedRemovedIndices": this should be set to false if the new mesh data
	contains the SAME AMOUNT of indices as the previous one. If the new mesh data contains the same amount of
	indices but they have different values then of course this value should be false.
	This is just an optimization not to create a new index buffer on every setMeshData() call. You can just set this value to true every time, but it's
	recommended to consider this optimization. Here we don't care about vertices because we create a new vertex buffer anyway (unlike SRuntimeMeshComponent).
	* remarks: this function is thread-safe (you can call it from any thread).
	*/
	void setMeshData           (const SMeshData& meshData, bool bAddedRemovedIndices);

	//@@Function
	/*
	* desc: unbinds the material from the component so that this component will use default engine material.
	* remarks: note that this function will not unregister the material, you should do it by yourself.
	*/
	void unbindMaterial        ();

	//@@Function
	/*
	* desc: used to switch the custom used shader to a default one.
	* param "bForceUseDefaultEvenIfSpawned": forces the engine to change the shader even if this mesh is spawned (causes small fps drop).
	* return: false if successful, true if this mesh is spawned and the 'bForceUseDefaultEvenIfSpawned' is false.
	*/
	bool setUseDefaultShader   (bool bForceUseDefaultEvenIfSpawned = false);

	//@@Function
	/*
	* desc: used to set the material of the mesh.
	* return: false if successful, true if the material is not registered using SApplication::registerMaterial() function.
	*/
	bool setMeshMaterial       (SMaterial* pMaterial);

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
	* desc: every 2 indices will be considered as a line.
	*/
	void      setDrawAsLines(bool bDrawAsLines);

	//@@Function
	/*
	* desc: used to retrieve the material of the mesh.
	* return: valid pointer if the material was assigned before, nullptr otherwise.
	*/
	SMaterial* getMeshMaterial();

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
	* desc: used to retrieve the mesh data as a resource for SComputeShader.
	* param "bGetVertexBuffer": set true to get vertex buffer as a resource for SComputeShader, false for index buffer. 'True' will also
	disable the frustum culling.
	* return: nullptr if the component is not spawned (i.e. no buffer was created), valid pointer otherwise.
	* remarks: pass this pointer to your compute shader in setAddMeshResource(), do not 'delete' this pointer (compute shader needs
	this pointer for some time, it will delete it for you in its destructor). You can delete this mesh even if the compute shader is working,
	the resource will not be released until at least somebody is using it (in this case in compute shader destructor the resource will be released).
	*/
	SMeshDataComputeResource* getMeshDataAsComputeResource(bool bGetVertexBuffer);

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
	friend class SComputeShader;

	virtual void unbindMaterialsIncludingChilds() override;

	Microsoft::WRL::ComPtr<ID3D12Resource> getResource(bool bVertexBuffer);

	//@@Function
	/*
	* desc: returns the SRenderItem data.
	*/
	virtual SRenderItem* getRenderData() override;

	//@@Function
	/*
	* desc: creates vertex & index buffer (if bAddedRemovedIndices is true).
	*/
	void createGeometryBuffers (bool bAddedRemovedIndices);
	//@@Function
	/*
	* desc: updates the world matrix using getWorldMatrix().
	*/
	void updateWorldMatrix     ();

	//@@Function
	/*
	* desc: called when parent's location/rotation/scale are changed.
	*/
	virtual void updateMyAndChildsLocationRotationScale(bool bCalledOnSelf) override;


	// ------------------------------------------------------------------------------------

	bool        bVisible;
	bool        bVertexBufferUsedInComputeShader;
};

