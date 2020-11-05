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
#include "SilentEngine/Public/SVector/SVector.h"

// DirectX
#include <DirectXMath.h>
#include <d3d12.h>
#include <wrl.h> // smart pointers

class STextureInternal;

struct STextureHandle
{
	//@@Function
	/*
	* desc: returns the name of the texture.
	*/
	std::string getTextureName() const;

	//@@Function
	/*
	* desc: returns the path to the texture file on disk.
	*/
	std::wstring getPathToTextureFile() const;

	//@@Function
	/*
	* desc: returns the allocated GPU memory size that this texture is taking.
	* remarks: returns 0 if the texture is not loaded via SApplication::loadTextureFromDiskToGPU().
	*/
	unsigned long long getTextureSizeInBytesOnGPU() const;

private:

	friend class SApplication;
	friend class SMaterialProperties;

	std::string sTextureName = "";

	std::wstring sPathToTexture = L"";

	STextureInternal* pRefToTexture = nullptr;

	bool bRegistered  = false;
};

struct STextureInternal
{
	std::string sTextureName = "";
	std::wstring sPathToTexture = L"";

	unsigned long long iResourceSizeInBytesOnGPU = 0;

	Microsoft::WRL::ComPtr<ID3D12Resource> pResource = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> pUploadHeap = nullptr;

	int iTexSRVHeapIndex = -1;
};

struct SMaterialProperties
{
	//@@Function
	/*
	* desc: sets the custom transparency.
	* remarks: it's recommended to change this value only in onTick() function.
	If this transparency in the component is enabled then
	the material will use this value as an alpha for this material on top of the diffuse texture alpha channel.
	*/
	void    setCustomTransparency(float fCustomTransparency = 1.0f);

	void    setRoughness     (float fRoughness);

	//@@Function
	/*
	* desc: sets the diffuse color.
	* remarks: changing the diffuse color (if the material has a texture (setDiffuseTexture()) will affect
	the final look of the material as texture and color will blend.
	*/
	void    setDiffuseColor  (const SVector& vRGBA);
	void    setSpecularColor (const SVector& vRGB);

	//@@Function
	/*
	* desc: sets the texture for diffuse color.
	* return: true (error) if the texture is not loaded by the SApplication::loadTextureFromDiskToGPU(), false otherwise.
	* remarks: alpha channel of the diffuse texture controls the transparency (see setCustomTransparency() for more).
	Changing the diffuse color (setDiffuseColor()) will affect the final look of the material as texture and color will blend.
	*/
	bool    setDiffuseTexture(STextureHandle textureHandle);

	//@@Function
	/*
	* desc: unbinds the texture from the material if the texture with the given name exists.
	*/
	void    unbindTexture    (STextureHandle textureHandle);

	//@@Function
	/*
	* desc: returns the custom transparency of the material.
	*/
	float   getCustomTransparency() const;

	//@@Function
	/*
	* desc: used to retrieve the diffuse texture of the material.
	* param "pTextureHandle": valid pointer if the material has a texture.
	* return: true if the material has no texture, false otherwise.
	*/
	bool    getDiffuseTexture(STextureHandle* pTextureHandle);

	float   getRoughness     () const;

	//@@Function
	/*
	* desc: used to retrieve the diffuse color of the material.
	*/
	SVector getDiffuseColor  () const;
	SVector getSpecularColor () const;

private:

	friend class SApplication;

	float fRoughness = 0.5f;
	float fCustomTransparency = 1.0f;

	// Diffuse albedo.
	DirectX::XMFLOAT4 vDiffuseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	// FresnelR0.
	DirectX::XMFLOAT3 vSpecularColor = { 0.01f, 0.01f, 0.01f };

	bool bHasDiffuseTexture = false;
	bool bHasNormalTexture = false;

	STextureHandle diffuseTexture;
	// if added new texture then add new texture to unbindTexture()
	// and in SMaterialConstants 'bHasTexture'
	// and in SApplication::unloadTextureFromGPU where it says "// ADD OTHER TEXTURES HERE".
};

class SMaterial
{
public:

	//@@Function
	/*
	* desc: used to set the properties (settings) of the material.
	*/
	void setMaterialProperties(const SMaterialProperties& matProps);

	//@@Function
	/*
	* desc: used to set the UV offset to the material.
	* return: true if the UVs are not in [0, 1] range or if the material is not registered, false otherwise.
	* remarks: should be in [0, 1] range.
	*/
	bool setMaterialUVOffset(const SVector& vMaterialUVOffset);

	//@@Function
	/*
	* desc: used to set the UV scale to the material.
	*/
	void setMaterialUVScale(const SVector& vMaterialUVScale);

	//@@Function
	/*
	* desc: used to set the UV rotation to the material.
	*/
	void setMaterialUVRotation(float fRotation);

	//@@Function
	/*
	* desc: returns the UV offset of the material.
	*/
	SVector getMaterialUVOffset() const;

	//@@Function
	/*
	* desc: returns the UV scale of the material.
	*/
	SVector getMaterialUVScale() const;

	//@@Function
	/*
	* desc: returns the UV rotation of the material.
	*/
	float getMaterialUVRotation() const;

	//@@Function
	/*
	* desc: returns the name of this material.
	*/
	std::string getMaterialName() const;

	//@@Function
	/*
	* desc: returns the properties of the material.
	*/
	SMaterialProperties getMaterialProperties() const;

private:

	friend class SApplication;
	friend class SMeshComponent;
	friend class SRuntimeMeshComponent;
	friend class SMeshData;


	// Only SApplication can create instances of SMaterial.

	SMaterial();
	SMaterial(std::string sMaterialName);
	//@@Function
	/*
	* desc: only copies the material properties and UV offset/rotation/scale.
	*/
	SMaterial(const SMaterial& mat);
	//@@Function
	/*
	* desc: only copies the material properties and UV offset/rotation/scale.
	*/
	SMaterial& operator= (const SMaterial& mat);



	void updateMatTransform();

	std::string sMaterialName;


	size_t iMatCBIndex;

	int iUpdateCBInFrameResourceCount;
	int iFrameResourceIndexLastUpdated;
	bool bLastFrameResourceIndexValid;


	SMaterialProperties matProps;


	SVector             vMatUVOffset;
	float               fMatRotation;
	SVector             vMatUVScale;


	DirectX::XMFLOAT4X4 vMatTransform;


	bool               bRegistered;
};

