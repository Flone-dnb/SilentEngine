// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "SFrameResource.h"

// Custom
#include "SilentEngine/Private/SError/SError.h"

SFrameResource::SFrameResource(ID3D12Device* pDevice, UINT iObjectCBCount)
{
	this->pDevice = pDevice;

	HRESULT hresult = pDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(pCommandListAllocator.GetAddressOf()));

	if (FAILED(hresult))
	{
		SError::showErrorMessageBoxAndLog(hresult);
	}
	else
	{
		createRenderObjectBuffers(iObjectCBCount);
		createMaterialBuffer(iCBResizeMultiple);
	}
}

void SFrameResource::createRenderObjectBuffers(UINT64 iObjectCBCount)
{
	iObjectCBCount = roundUp(iObjectCBCount, iCBResizeMultiple);

	pRenderPassCB = std::make_unique<SUploadBuffer<SRenderPassConstants>>  (pDevice, iRenderPassCBCount, true);
	pObjectsCB    = std::make_unique<SUploadBuffer<SObjectConstants>>      (pDevice, iObjectCBCount, true);
}

void SFrameResource::createMaterialBuffer(UINT64 iMaterialCBCount)
{
	iMaterialCBCount = roundUp(iMaterialCBCount, iCBResizeMultiple);

	pMaterialCB = std::make_unique<SUploadBuffer<SMaterialConstants>> (pDevice, iMaterialCBCount, true);
}

UINT64 SFrameResource::addNewObjectCB(UINT64 iNewCBCount, bool* pbCBWasExpanded)
{
	UINT64 iCeiling = roundUp(iObjectsCBActualElementCount, iCBResizeMultiple);

	if (iObjectsCBActualElementCount + iNewCBCount > iCeiling)
	{
		// Need to expand.

		createRenderObjectBuffers(iObjectsCBActualElementCount + iNewCBCount); // recreating new buffers.

		*pbCBWasExpanded = true; // all objects will again copy their data to frame resources.
	}
	else
	{
		*pbCBWasExpanded = false;
	}

	iObjectsCBActualElementCount += iNewCBCount;

	return iObjectsCBActualElementCount - iNewCBCount;
}

void SFrameResource::removeObjectCB(UINT64 iCBStartIndex, UINT64 iCBCount, bool* pbCBWasResized)
{
	UINT64 iCeiling = roundUp(iObjectsCBActualElementCount, iCBResizeMultiple);

	iCeiling -= iCBResizeMultiple;

	if (iObjectsCBActualElementCount > iCBResizeMultiple)
	{
		if (iObjectsCBActualElementCount - iCBCount <= iCeiling)
		{
			// Resize.
			*pbCBWasResized = true; // all objects will again copy their data to frame resources.
			
			createRenderObjectBuffers(iObjectsCBActualElementCount - iCBCount); // recreating new buffers.
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

size_t SFrameResource::addNewMaterialCB(bool* pbCBWasExpanded)
{
	UINT64 iCeiling = roundUp(iMaterialCBActualElementCount, iCBResizeMultiple);

	if (iMaterialCBActualElementCount + 1 > iCeiling)
	{
		// Need to expand.

		createMaterialBuffer(iMaterialCBActualElementCount + 1); // recreating new buffers.

		*pbCBWasExpanded = true; // all materials will again copy their data to frame resources.
	}
	else
	{
		*pbCBWasExpanded = false;
	}

	iMaterialCBActualElementCount++;

	return iMaterialCBActualElementCount - 1;
}

void SFrameResource::removeMaterialCB(UINT64 iCBIndex, bool * pbCBWasResized)
{
	size_t iCeiling = roundUp(iMaterialCBActualElementCount, iCBResizeMultiple);

	iCeiling -= iCBResizeMultiple;

	if (iMaterialCBActualElementCount > iCBResizeMultiple)
	{
		if (iMaterialCBActualElementCount - 1 <= iCeiling)
		{
			// Resize.
			*pbCBWasResized = true; // all objects will again copy their data to frame resources.

			createMaterialBuffer(iMaterialCBActualElementCount - 1); // recreating new buffers.
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

	iMaterialCBActualElementCount--;
}

SUploadBuffer<SMaterialConstants>* SFrameResource::addNewMaterialBundleResource(SShader* pShader, size_t iResourceCount)
{
	vMaterialBundles.push_back(std::make_unique<SMaterialBundle>(pShader, pDevice, iResourceCount, false));

	return vMaterialBundles.back().get()->pResource.get();
}

void SFrameResource::removeMaterialBundle(SShader* pShader)
{
	for (size_t i = 0; i < vMaterialBundles.size(); i++)
	{
		if (vMaterialBundles[i]->pShaderUsingThisResource == pShader)
		{
			vMaterialBundles.erase(vMaterialBundles.begin() + i);

			break;
		}
	}
}

SUploadBuffer<SObjectConstants>* SFrameResource::addNewInstancedMesh(std::vector<SObjectConstants>* pInitData)
{
	vInstancedMeshes.push_back(std::make_unique<SUploadBuffer<SObjectConstants>>(pDevice, pInitData->size(), false));

	// don't:
	/*for (size_t i = 0; i < pInitData->size(); i++)
	{
		vInstancedMeshes.back()->copyDataToElement(i, pInitData->operator[](i));
	}*/
	// because we fill data on every frame during frustum culling

	return vInstancedMeshes.back().get();
}

SUploadBuffer<SObjectConstants>* SFrameResource::addNewInstanceToMesh(SUploadBuffer<SObjectConstants>* pInstancedData, const SObjectConstants& newInstanceData)
{
	// Copy data to new temp buffer.

	size_t iOldSize = pInstancedData->getElementCount();

	size_t iMappedDataSizeInBytes = 0;
	unsigned char* pMappedData = pInstancedData->getMappedData(iMappedDataSizeInBytes);

	unsigned char* pTempData = new unsigned char[iMappedDataSizeInBytes];
	std::memcpy(pTempData, pMappedData, iMappedDataSizeInBytes);


	removeInstancedMesh(pInstancedData);


	// Create new data.

	vInstancedMeshes.push_back(std::make_unique<SUploadBuffer<SObjectConstants>>(pDevice, iOldSize + 1, false));

	// don't: (see below)
	//vInstancedMeshes.back()->copyData(pTempData, iMappedDataSizeInBytes);


	delete[] pTempData;


	// don't:
	// Copy new instance data.
	//vInstancedMeshes.back()->copyDataToElement(iOldSize, newInstanceData);
	// because we fill data on every frame during frustum culling


	return vInstancedMeshes.back().get();
}

void SFrameResource::removeInstancedMesh(SUploadBuffer<SObjectConstants>* pInstancedDataToDelete)
{
	for (size_t i = 0; i < vInstancedMeshes.size(); i++)
	{
		if (vInstancedMeshes[i].get() == pInstancedDataToDelete)
		{
			vInstancedMeshes.erase(vInstancedMeshes.begin() + i);

			break;
		}
	}
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

size_t SFrameResource::roundUp(size_t iNum, size_t iMultiple)
{
	if (iMultiple == 0)
	{
		return iNum;
	}

	if (iNum == 0)
	{
		return iMultiple;
	}

	size_t iRemainder = iNum % iMultiple;
	if (iRemainder == 0)
	{
		return iNum;
	}

	return iNum + iMultiple - iRemainder;
}

SFrameResource::~SFrameResource()
{
}