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

// DirectX
#include <wrl.h> // smart pointers
#include <d3d12.h>

class SCustomShaderResources;

class SShader
{
public:

	SCustomShaderResources* getCustomShaderResources();

private:

	friend class SApplication;

	// Only SApplication can create instances of SShader.
	SShader(const std::wstring& sPathToShaderFile);
	SShader(const SShader&) = delete;
	SShader& operator= (const SShader&) = delete;


	void setCustomShaderResources(SCustomShaderResources* pCustomShaderResources);


	SCustomShaderResources* pCustomShaderResources = nullptr;


	std::wstring sPathToShaderFile;


	Microsoft::WRL::ComPtr<ID3DBlob> pVS;
	Microsoft::WRL::ComPtr<ID3DBlob> pPS;
	Microsoft::WRL::ComPtr<ID3DBlob> pAlphaPS;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> pOpaquePSO;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pTransparentPSO;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pTransparentAlphaToCoveragePSO;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> pOpaqueWireframePSO;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pTransparentWireframePSO;
};

class SComponent;

struct SShaderObjects
{
	SShader* pShader;

	std::vector<SComponent*> vMeshComponentsWithThisShader;
};