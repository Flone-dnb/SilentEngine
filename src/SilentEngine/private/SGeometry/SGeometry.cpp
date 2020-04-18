// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************


#include "SGeometry.h"

// STL
#include <fstream>

// DirectX
#include <D3Dcompiler.h>

// Custom
#include "SilentEngine/private/SError/SError.h"

#pragma comment(lib,"d3dcompiler.lib")


Microsoft::WRL::ComPtr<ID3D12Resource> SGeometry::createDefaultBuffer(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, const void* pInitBufferData, UINT64 iDataSizeInBytes, Microsoft::WRL::ComPtr<ID3D12Resource>& pUploadBuffer)
{
	Microsoft::WRL::ComPtr<ID3D12Resource> pDefaultBuffer;

	// Create the actual default buffer resource.

	HRESULT hresult = pDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(iDataSizeInBytes),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(pDefaultBuffer.GetAddressOf()));
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SGeometry::createDefaultBuffer::ID3D12Device::CreateCommittedResource() (default buffer)");
		return nullptr;
	}


	// In order to copy CPU memory data into our default buffer, we need to create an intermediate upload heap.

	hresult = pDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(iDataSizeInBytes),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(pUploadBuffer.GetAddressOf()));
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SGeometry::createDefaultBuffer::ID3D12Device::CreateCommittedResource() (upload heap)");
		return nullptr;
	}


	// Describe the data we want to copy into the default buffer.

	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = pInitBufferData;
	subResourceData.RowPitch = iDataSizeInBytes;
	subResourceData.SlicePitch = subResourceData.RowPitch;



	// Copy the data to the default buffer resource.

	pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pDefaultBuffer.Get(), 
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));



	// Copy CPU memory into the intermediate upload heap. Using ID3D12CommandList::CopySubresourceRegion.

	UpdateSubresources<1>(pCommandList, pDefaultBuffer.Get(), pUploadBuffer.Get(), 0, 0, 1, &subResourceData);



	pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pDefaultBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

	// pUploadBuffer has to be kept alive because the command list has not been executed yet that performs the actual copy.
	// The caller can Release the pUploadBuffer after it knows the copy has been executed.


	return pDefaultBuffer;
}

Microsoft::WRL::ComPtr<ID3DBlob> SGeometry::compileShader(const std::wstring& sPathToShader, 
	const D3D_SHADER_MACRO* defines, 
	const std::string& sShaderEntryPoint, 
	const std::string& sShaderModel)
{
	// Check if the file exist.

	std::ifstream sShaderFile(sPathToShader.c_str());

	if (sShaderFile.is_open())
	{
		sShaderFile.close();
	}
	else
	{
		SError::showErrorMessageBox(L"SGeometry::compileShader::D3DCompileFromFile()", L"File at " + sPathToShader + L" does not exist.");
		return nullptr;
	}


	UINT iCompileFlags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
#if defined(DEBUG) || defined(_DEBUG)  
	iCompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	HRESULT hresult = S_OK;

	Microsoft::WRL::ComPtr<ID3DBlob> pByteCode = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pErrors = nullptr;

	hresult = D3DCompileFromFile(sPathToShader.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		sShaderEntryPoint.c_str(), sShaderModel.c_str(), iCompileFlags, 0, &pByteCode, &pErrors);
	if (pErrors != nullptr)
	{
		OutputDebugStringA((char*)pErrors->GetBufferPointer());
	}

	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SGeometry::compileShader::D3DCompileFromFile()");
		return nullptr;
	}

	return pByteCode;
}

D3D12_VERTEX_BUFFER_VIEW SMeshGeometry::getVertexBufferView() const
{
	D3D12_VERTEX_BUFFER_VIEW vbv;
	vbv.BufferLocation = pVertexBufferGPU->GetGPUVirtualAddress();
	vbv.StrideInBytes = iVertexByteStride;
	vbv.SizeInBytes = iVertexBufferSizeInBytes;

	return vbv;
}

D3D12_INDEX_BUFFER_VIEW SMeshGeometry::getIndexBufferView() const
{
	D3D12_INDEX_BUFFER_VIEW ibv;
	ibv.BufferLocation = pIndexBufferGPU->GetGPUVirtualAddress();
	ibv.Format = indexFormat;
	ibv.SizeInBytes = iIndexBufferSizeInBytes;

	return ibv;
}

void SMeshGeometry::freeUploaders()
{
	pVertexBufferUploader = nullptr;
	pIndexBufferUploader = nullptr;
}
