// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "SMiscHelpers.h"

// STL
#include <fstream>
#include <filesystem>

// DirectX
#include <D3Dcompiler.h>
#include "SilentEngine/Private/d3dx12.h"

// DXC
#include "dxc/d3d12shader.h"
#pragma comment(lib, "dxc/dxcompiler.lib")

// Custom
#include "SilentEngine/private/SError/SError.h"


#pragma comment(lib,"d3dcompiler.lib")


Microsoft::WRL::ComPtr<ID3D12Resource> SMiscHelpers::createBufferWithData(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList,
	const void* pInitBufferData, UINT64 iDataSizeInBytes, Microsoft::WRL::ComPtr<ID3D12Resource>& pOutUploadBuffer, bool bCreateUAVBuffer)
{
	Microsoft::WRL::ComPtr<ID3D12Resource> pDefaultBuffer;

	// Create the actual default buffer resource.

	HRESULT hresult;
	if (bCreateUAVBuffer)
	{
		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
		CD3DX12_RESOURCE_DESC buf = CD3DX12_RESOURCE_DESC::Buffer(iDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

		hresult = pDevice->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&buf,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			nullptr,
			IID_PPV_ARGS(pDefaultBuffer.GetAddressOf()));
		if (FAILED(hresult))
		{
			SError::showErrorMessageBox(hresult, L"SMiscHelpers::createDefaultBuffer::ID3D12Device::CreateCommittedResource() (default buffer)");
			return nullptr;
		}
	}
	else
	{
		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
		CD3DX12_RESOURCE_DESC buf = CD3DX12_RESOURCE_DESC::Buffer(iDataSizeInBytes);

		hresult = pDevice->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&buf,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(pDefaultBuffer.GetAddressOf()));
		if (FAILED(hresult))
		{
			SError::showErrorMessageBox(hresult, L"SMiscHelpers::createDefaultBuffer::ID3D12Device::CreateCommittedResource() (default buffer)");
			return nullptr;
		}
	}



	// In order to copy CPU memory data into our default buffer, we need to create an intermediate upload heap.

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC buf = CD3DX12_RESOURCE_DESC::Buffer(iDataSizeInBytes);

	hresult = pDevice->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&buf,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(pOutUploadBuffer.GetAddressOf()));
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SMiscHelpers::createDefaultBuffer::ID3D12Device::CreateCommittedResource() (upload heap)");
		return nullptr;
	}


	// Describe the data we want to copy into the default buffer.

	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = pInitBufferData;
	subResourceData.RowPitch = iDataSizeInBytes;
	subResourceData.SlicePitch = subResourceData.RowPitch;



	// Copy the data to the default buffer resource.

	if (bCreateUAVBuffer)
	{
		CD3DX12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(pDefaultBuffer.Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST);

		pCommandList->ResourceBarrier(1, &transition);
	}
	else
	{
		CD3DX12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(pDefaultBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

		pCommandList->ResourceBarrier(1, &transition);
	}



	// Copy CPU memory into the intermediate upload heap. Using ID3D12CommandList::CopySubresourceRegion.

	UpdateSubresources<1>(pCommandList, pDefaultBuffer.Get(), pOutUploadBuffer.Get(), 0, 0, 1, &subResourceData);


	if (bCreateUAVBuffer)
	{
		CD3DX12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(pDefaultBuffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		pCommandList->ResourceBarrier(1, &transition);
	}
	else
	{
		CD3DX12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(pDefaultBuffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);

		pCommandList->ResourceBarrier(1, &transition);
	}


	// pUploadBuffer has to be kept alive because the command list has not been executed yet that performs the actual copy.
	// The caller can Release the pUploadBuffer after it knows the copy has been executed.


	return pDefaultBuffer;
}

ATL::CComPtr<IDxcBlob> SMiscHelpers::compileShader(const std::wstring& sPathToShader,
	const D3D_SHADER_MACRO* defines,
	const std::wstring& sShaderEntryPoint,
	const std::wstring& sShaderModel,
	bool bCompileShadersInRelease)
{
	// Check if the file exist.

	std::ifstream sShaderFile(sPathToShader.c_str());

	if (sShaderFile.is_open())
	{
		sShaderFile.close();
	}
	else
	{
		SError::showErrorMessageBox(L"SMiscHelpers::compileShader()", L"file at " + sPathToShader + L" does not exist.");
		return nullptr;
	}


	// --- FXC ---
//	HRESULT hresult = S_OK;
//
//	Microsoft::WRL::ComPtr<ID3DBlob> pByteCode = nullptr;
//	Microsoft::WRL::ComPtr<ID3DBlob> pErrors = nullptr;
//
//	hresult = D3DCompileFromFile(sPathToShader.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
//		sShaderEntryPoint.c_str(), sShaderModel.c_str(), iCompileFlags, 0, &pByteCode, &pErrors);
//	if (pErrors != nullptr)
//	{
//		OutputDebugStringA((char*)pErrors->GetBufferPointer());
//	}
//
//	if (FAILED(hresult))
//	{
//		SError::showErrorMessageBox(hresult, L"SMiscHelpers::compileShader::D3DCompileFromFile()");
//		return nullptr;
//	}
	// --- FXC ---


	// Create compiler and utils.
	CComPtr<IDxcUtils> pUtils;
	CComPtr<IDxcCompiler3> pCompiler;
	DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&pUtils));
	DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&pCompiler));

	// Create default include handler.
	CComPtr<IDxcIncludeHandler> pIncludeHandler;
	pUtils->CreateDefaultIncludeHandler(&pIncludeHandler);



	std::vector<LPCWSTR> vArgs;
	vArgs.push_back(sPathToShader.c_str());
	vArgs.push_back(L"-E");
	vArgs.push_back(sShaderEntryPoint.c_str());
	vArgs.push_back(L"-T");
	vArgs.push_back(sShaderModel.c_str());
#if defined(DEBUG) || defined(_DEBUG) 
	if (bCompileShadersInRelease == false)
	{
		vArgs.push_back(DXC_ARG_DEBUG);
		vArgs.push_back(DXC_ARG_SKIP_OPTIMIZATIONS);
		//vArgs.push_back(L"-Fd");
		//vArgs.push_back((sPathToShader + L".pdb").c_str());
	}
	else
	{
		vArgs.push_back(DXC_ARG_OPTIMIZATION_LEVEL3);
	}
#elif
	vArgs.push_back(DXC_ARG_OPTIMIZATION_LEVEL3);
#endif


	// Open source file.  
	CComPtr<IDxcBlobEncoding> pSource = nullptr;
	pUtils->LoadFile(sPathToShader.c_str(), nullptr, &pSource);
	DxcBuffer Source;
	Source.Ptr = pSource->GetBufferPointer();
	Source.Size = pSource->GetBufferSize();
	Source.Encoding = DXC_CP_ACP;

	// Compile it with specified arguments.
	CComPtr<IDxcResult> pResults;
	pCompiler->Compile(
		&Source,
		vArgs.data(),
		vArgs.size(),
		pIncludeHandler,
		IID_PPV_ARGS(&pResults)
	);


	// Print errors if present.
	CComPtr<IDxcBlobUtf8> pErrors = nullptr;
	pResults->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr);
#if defined(DEBUG) || defined(_DEBUG) 
	if (pErrors != nullptr && pErrors->GetStringLength() != 0)
	{
		OutputDebugStringA("\n--------------------------\n");
		OutputDebugStringA("\n");
		std::wstring sLogText = L"There were errors/warnings encountered while compiling the shader \"" + sPathToShader + L"\".\n";
		OutputDebugStringW(sLogText.c_str());
		OutputDebugStringA("\n");
		OutputDebugStringA(pErrors->GetStringPointer());
		OutputDebugStringA("\n");
		OutputDebugStringA("--------------------------\n\n");
	}
#endif


	// Quit if the compilation failed.
	HRESULT hrStatus;
	pResults->GetStatus(&hrStatus);
	if (FAILED(hrStatus))
	{
#if defined(DEBUG) || defined(_DEBUG) 
		SError::showErrorMessageBox(L"SMiscHelpers::compileShader::GetStatus()", L"There was a shader compilation error (see output).");
#else
		SError::showErrorMessageBox(L"SMiscHelpers::compileShader::GetStatus()", L"There was a shader compilation error.");
#endif
		return nullptr;
	}


	// Save shader binary.
	CComPtr<IDxcBlob> pShader = nullptr;
	CComPtr<IDxcBlobUtf16> pShaderName = nullptr;
	pResults->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pShader), &pShaderName);
	if (pShader == nullptr)
	{
		SError::showErrorMessageBox(L"SMiscHelpers::compileShader::GetStatus()", L"no shader binary was generated.");
		return nullptr;
	}


	// Write bytecode to file.

	/*FILE* fp = NULL;

	_wfopen_s(&fp, pShaderName->GetStringPointer(), L"wb");
	fwrite(pShader->GetBufferPointer(), pShader->GetBufferSize(), 1, fp);
	fclose(fp);*/


#if defined(DEBUG) || defined(_DEBUG) 
	if (bCompileShadersInRelease == false)
	{
		// Save pdb.
		CComPtr<IDxcBlob> pPDB = nullptr;
		CComPtr<IDxcBlobUtf16> pPDBName = nullptr;
		pResults->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(&pPDB), &pPDBName);
		{
			std::wstring sTempPDBPath = L"_temp_shaders_pdb/";
			if (!std::filesystem::exists(sTempPDBPath))
			{
				std::filesystem::create_directory(sTempPDBPath);
			}

			std::wstring sPDBPath = (sTempPDBPath + std::wstring(pPDBName->GetStringPointer())).c_str();

			FILE* fp = NULL;
			// Note that if you don't specify -Fd, a pdb name will be automatically generated.
			_wfopen_s(&fp, sPDBPath.c_str(), L"wb");
			fwrite(pPDB->GetBufferPointer(), pPDB->GetBufferSize(), 1, fp);
			fclose(fp);
		}
	}
#endif

	return pShader;
}