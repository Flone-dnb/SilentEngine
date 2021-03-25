// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#pragma once

// DirectX
#include <wrl.h> // smart pointers
#include "d3d12.h"
#pragma warning(push, 0) // disable warnings from this header
#include "SilentEngine/Private/d3dx12.h"
#pragma warning(pop)

// Custom
#include "SilentEngine/Private/SMath/SMath.h"
#include "SilentEngine/Private/SError/SError.h"


template<typename T>
class SUploadBuffer
{
public:

	SUploadBuffer(ID3D12Device* pDevice, UINT64 iElementCount, bool bIsCBUFFER)
	{
		this->bIsConstantBuffer = bIsCBUFFER;

		iElementSizeInBytes = sizeof(T);

		if (bIsConstantBuffer)
		{
			// Constant buffer elements need to be multiples of 256 bytes.
			// This is because the hardware can only view constant data 
			// at m*256 byte offsets and of n*256 byte lengths. 

			iElementSizeInBytes = SMath::makeMultipleOf256(sizeof(T));
		}

		this->iElementCount = iElementCount;

		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(iElementSizeInBytes * iElementCount);

		HRESULT hresult = pDevice->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&pUploadBuffer));

		if (FAILED(hresult))
		{
			SError::showErrorMessageBox(hresult, L"SUploadBuffer::SUploadBuffer::ID3D12Device::CreateCommittedResource()");
		}

		hresult = pUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pMappedData));

		// We do not need to unmap until we are done with the resource. However, we must not write to
		// the resource while it is in use by the GPU (so we must use synchronization techniques).
	}

	SUploadBuffer(const SUploadBuffer&) = delete;
	SUploadBuffer& operator=(const SUploadBuffer&) = delete;

	~SUploadBuffer()
	{
		if (pUploadBuffer != nullptr)
		{
			pUploadBuffer->Unmap(0, nullptr);
		}

		pMappedData = nullptr;
	}

	UINT64 getElementCount() const
	{
		return iElementCount;
	}

	UINT64 getElementSize() const
	{
		return iElementSizeInBytes;
	}

	ID3D12Resource* getResource() const
	{
		return pUploadBuffer.Get();
	}

	void copyDataToElement(UINT64 iElementIndex, const T& data)
	{
		std::memcpy(&pMappedData[iElementIndex * iElementSizeInBytes], &data, sizeof(T));
	}

	void copyData(void* pData, UINT64 iDataSizeInBytes)
	{
		std::memcpy(pMappedData, pData, iDataSizeInBytes);
	}

	unsigned char* getMappedData(size_t& iOutSizeInBytes) const
	{
		iOutSizeInBytes = iElementSizeInBytes * iElementCount;

		return pMappedData;
	}

private:

	Microsoft::WRL::ComPtr<ID3D12Resource> pUploadBuffer;


	unsigned char* pMappedData;


	UINT64 iElementSizeInBytes;
	UINT64 iElementCount;


	bool bIsConstantBuffer;
};
