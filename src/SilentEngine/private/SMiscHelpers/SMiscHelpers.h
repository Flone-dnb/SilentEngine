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
#include <wrl.h> // smart pointers
#include <d3d12.h>


#define SE_VS_SM "vs_5_1"
#define SE_PS_SM "ps_5_1"
#define SE_CS_SM "cs_5_1"


class SMiscHelpers
{
public:

	// pUploadBuffer has to be kept alive because the command list has not been executed yet that performs the actual copy.
	// The caller can Release the pUploadBuffer after it knows the copy has been executed.
	static Microsoft::WRL::ComPtr<ID3D12Resource> createBufferWithData(
		ID3D12Device* pDevice,
		ID3D12GraphicsCommandList* pCommandList,
		const void* pInitBufferData,
		UINT64 iDataSizeInBytes,
		Microsoft::WRL::ComPtr<ID3D12Resource>& pOutUploadBuffer, bool bCreateUAVBuffer = false);


	static Microsoft::WRL::ComPtr<ID3DBlob> compileShader(
		const std::wstring& sPathToShader,
		const D3D_SHADER_MACRO* defines,
		const std::string& sShaderEntryPoint,
		const std::string& sShaderModel,
		bool bCompileShadersInRelease);
};

