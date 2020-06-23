// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "SFrameResource.h"

// Custom
#include "SilentEngine/Private/SError/SError.h"

SFrameResource::SFrameResource(ID3D12Device* pDevice, UINT iRenderPassCBCount, UINT iObjectCBCount)
{
	this->pDevice = pDevice;

	HRESULT hresult = pDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(pCommandListAllocator.GetAddressOf()));

	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SFrameResource::SFrameResource::ID3D12Device::CreateCommandAllocator()");
	}
	else
	{
		createBuffers(iRenderPassCBCount, iObjectCBCount);
		this->iRenderPassCBCount = iRenderPassCBCount;
	}
}

void SFrameResource::createBuffers(UINT iRenderPassCBCount, UINT iObjectCBCount)
{
	iObjectCBCount = roundUp(iObjectCBCount, iObjectCBResizeMultiple);

	pRenderPassCB = std::make_unique<SUploadBuffer<SRenderPassConstants>>  (pDevice, iRenderPassCBCount, true);
	pObjectsCB    = std::make_unique<SUploadBuffer<SObjectConstants>>      (pDevice, iObjectCBCount, true);
}

size_t SFrameResource::addNewObjectCB(size_t iNewCBCount, bool* pbCBWasExpanded)
{
	size_t iCeiling = roundUp(iObjectsCBActualElementCount, iObjectCBResizeMultiple);

	if (iObjectsCBActualElementCount + iNewCBCount > iCeiling)
	{
		// Need to expand.

		createBuffers(iRenderPassCBCount, iObjectsCBActualElementCount + iNewCBCount);

		*pbCBWasExpanded = true;
	}
	else
	{
		*pbCBWasExpanded = false;
	}

	iObjectsCBActualElementCount += iNewCBCount;

	return iObjectsCBActualElementCount - iNewCBCount;
}

void SFrameResource::removeObjectCB(size_t iCBStartIndex, size_t iCBCount, bool* pbCBWasResized)
{
	size_t iCeiling = roundUp(iObjectsCBActualElementCount, iObjectCBResizeMultiple);

	iCeiling -= iObjectCBResizeMultiple;

	if (iObjectsCBActualElementCount > iObjectCBResizeMultiple)
	{
		if (iObjectsCBActualElementCount - iCBCount <= iCeiling)
		{
			// Resize.
			*pbCBWasResized = true;

			createBuffers(iRenderPassCBCount, iObjectsCBActualElementCount - iCBCount);
		}
		else
		{
			*pbCBWasResized = false;
		}
	}
	else
	{
		*pbCBWasResized = false;
	}

	iObjectsCBActualElementCount -= iCBCount;
}

size_t SFrameResource::addRuntimeMeshVertexBuffer(size_t iVertexCount)
{
	vRuntimeMeshVertexBuffers.push_back(std::make_unique<SUploadBuffer<SVertex>> (pDevice, iVertexCount, false));

	return vRuntimeMeshVertexBuffers.size() - 1;
}

void SFrameResource::removeRuntimeMeshVertexBuffer(size_t iVertexBufferIndex)
{
	vRuntimeMeshVertexBuffers.erase( vRuntimeMeshVertexBuffers.begin() + iVertexBufferIndex );
}

void SFrameResource::recreateRuntimeMeshVertexBuffer(size_t iVertexBufferIndex, size_t iNewVertexCount)
{
	vRuntimeMeshVertexBuffers[iVertexBufferIndex] = std::make_unique<SUploadBuffer<SVertex>> (pDevice, iNewVertexCount, false);
}

UINT SFrameResource::roundUp(UINT iNum, UINT iMultiple)
{
	if (iMultiple == 0)
	{
		return iNum;
	}

	if (iNum == 0)
	{
		return iMultiple;
	}

	int iRemainder = iNum % iMultiple;
	if (iRemainder == 0)
	{
		return iNum;
	}

	return iNum + iMultiple - iRemainder;
}

SFrameResource::~SFrameResource()
{
}