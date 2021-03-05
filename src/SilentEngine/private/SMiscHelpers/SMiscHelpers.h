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

// DXC
#include "dxc/dxcapi.h"
#include <atlbase.h> // Common COM helpers.

#define SE_VS_SM L"vs_6_0"
#define SE_PS_SM L"ps_6_0"
#define SE_CS_SM L"cs_6_0"


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


	static ATL::CComPtr<IDxcBlob> compileShader(
		const std::wstring& sPathToShader,
		const D3D_SHADER_MACRO* defines,
		const std::wstring& sShaderEntryPoint,
		const std::wstring& sShaderModel,
		bool bCompileShadersInRelease);
};

