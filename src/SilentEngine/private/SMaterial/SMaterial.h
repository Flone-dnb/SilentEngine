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

struct SMaterialProperties
{
	void    setRoughness     (float fRoughness);
	void    setDiffuseColor  (const SVector& vRGBA);
	void    setSpecularColor (const SVector& vRGB);

	float   getRoughness     () const;
	SVector getDiffuseColor  () const;
	SVector getSpecularColor () const;

private:

	float fRoughness = 0.5f;

	// Diffuse albedo.
	DirectX::XMFLOAT4 vDiffuseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	// FresnelR0.
	DirectX::XMFLOAT3 vSpecularColor = { 0.01f, 0.01f, 0.01f };
};

class SMaterial
{
public:

	SMaterial();
	SMaterial(std::string sMaterialName);


	//@@Function
	/*
	* desc: sets the name of this material, this should be unique, otherwise SApplication::registerMaterial will fail.
	*/
	void setMaterialName(const std::string& sName);

	//@@Function
	/*
	* desc: used to set the properties (settings) of the material.
	*/
	void setMaterialProperties(const SMaterialProperties& matProps);

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

	std::string sMaterialName;


	SMaterial* pRegisteredOriginal;


	size_t iMatCBIndex;
	int iDiffuseSRVHeapIndex;
	int iNormalSRVHeapIndex;

	int iUpdateCBInFrameResourceCount;
	int iFrameResourceIndexLastUpdated;
	bool bLastFrameResourceIndexValid;


	SMaterialProperties matProps;


	DirectX::XMFLOAT4X4 mMatTransform;


	bool               bRegistered;
};

