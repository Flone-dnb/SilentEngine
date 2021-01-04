// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#pragma once

// STL
#include <vector>
#include <string>

// DirectX
#include <d3d12.h>
#include <wrl.h> // smart pointers

// Custom
#include "SilentEngine/Private/SUploadBuffer/SUploadBuffer.h"
#include "SilentEngine/Private/SFrameResource/SFrameResource.h"

class SMaterial;

class SCustomShaderResources
{
public:
	SCustomShaderResources() = default;
	SCustomShaderResources(const SCustomShaderResources&) = delete;
	SCustomShaderResources& operator= (const SCustomShaderResources&) = delete;

	//@@Function
	/*
	* desc: used to create a bundle of custom materials.
	* param "vMaterialNames": pass the names of the new materials.
	* param "pOutCreatedMaterials": pass an empty pointer to get a reference to the created material vector (if successful).
	* return: false if successful, true otherwise.
	* remarks: (important!) note that if the first material will use textures then we assume that all other materials do too,
	if some materials don't use textures use dummy textures or don't use textures for all materials.
	*/
	bool createMaterialBundle(const std::vector<std::string>& vMaterialNames, std::vector<SMaterial*>*& pvOutCreatedMaterials);

	std::vector<SMaterial*>* getMaterialBundle();

private:

	friend class SApplication;

	std::vector<SMaterial*> vMaterials;
	// has D3D resources from SMaterialBundle for each SFRAME_RES_COUNT (so the size of the vector is SFRAME_RES_COUNT).
	std::vector<SUploadBuffer<SMaterialConstants>*> vFrameResourceBundles;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> pCustomRootSignature;
};

