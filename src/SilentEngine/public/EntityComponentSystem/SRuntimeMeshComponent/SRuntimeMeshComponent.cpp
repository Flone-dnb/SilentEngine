// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "SRuntimeMeshComponent.h"

// DirectX
#include <D3Dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

// Custom
#include "SilentEngine/Private/SError/SError.h"
#include "SilentEngine/Public/SApplication/SApplication.h"
#include "SilentEngine/Public/EntityComponentSystem/SContainer/SContainer.h"


SRuntimeMeshComponent::SRuntimeMeshComponent(std::string sComponentName)
{
	componentType = SCT_RUNTIME_MESH;

	this->sComponentName = sComponentName;

	renderData.pGeometry = new SMeshGeometry();

	iIndexInFrameResourceVertexBuffer = 0;

	bVisible = true;
	bIndexOfVertexBufferValid = false;
	bNoMeshDataOnSpawn = false;
}

SRuntimeMeshComponent::~SRuntimeMeshComponent()
{
	delete renderData.pGeometry;
}

void SRuntimeMeshComponent::setVisibility(bool bVisible)
{
	this->bVisible = bVisible;
}

void SRuntimeMeshComponent::setMeshData(const SMeshData& meshData, bool bAddedVerticesOrUpdatedIndicesCount, bool bUpdateBoundingBox)
{
	mtxDrawComponent.lock();
	this->meshData = meshData;
	mtxDrawComponent.unlock();

	if (bAddedVerticesOrUpdatedIndicesCount)
	{
		renderData.pGeometry->iVertexBufferSizeInBytes = meshData.getVerticesCount() * sizeof(SVertex);
		renderData.pGeometry->iVertexGraphicsObjectSizeInBytes = sizeof(SVertex);

		if (meshData.hasIndicesMoreThan16Bits())
		{
			renderData.pGeometry->indexFormat = DXGI_FORMAT_R32_UINT;
			renderData.pGeometry->iIndexBufferSizeInBytes = meshData.getIndicesCount() * sizeof(std::uint32_t);
		}
		else
		{
			renderData.pGeometry->indexFormat = DXGI_FORMAT_R16_UINT;
			renderData.pGeometry->iIndexBufferSizeInBytes = meshData.getIndicesCount() * sizeof(std::uint16_t);
		}

		renderData.iIndexCount = meshData.getIndicesCount();

		if (bSpawnedInLevel)
		{
			createIndexBuffer();

			SApplication* pApp = SApplication::getApp();

			pApp->mtxSpawnDespawn.lock();
			mtxDrawComponent.lock();

			for (size_t i = 0; i < pApp->vFrameResources.size(); i++)
			{
				pApp->vFrameResources[i]->recreateRuntimeMeshVertexBuffer(iIndexInFrameResourceVertexBuffer, meshData.getVerticesCount());
			}

			mtxDrawComponent.unlock();
			pApp->mtxSpawnDespawn.unlock();
		}
	}
}

SMeshData SRuntimeMeshComponent::getMeshData() const
{
	return meshData;
}

bool SRuntimeMeshComponent::isVisible() const
{
	return bVisible;
}

SRenderItem* SRuntimeMeshComponent::getRenderData()
{
    return &renderData;
}

void SRuntimeMeshComponent::createIndexBuffer()
{
	SApplication* pApp = SApplication::getApp();

	/*HRESULT hresult = D3DCreateBlob(renderData.pGeometry->iIndexBufferSizeInBytes, &renderData.pGeometry->pIndexBufferCPU);
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SMeshComponent::createGeometryBuffers::D3DCreateBlob()");
		return;
	}*/

	if (bSpawnedInLevel)
	{
		pApp->mtxDraw.lock();
		pApp->mtxSpawnDespawn.lock();
		pApp->flushCommandQueue();
		pApp->resetCommandList();

		renderData.iUpdateCBInFrameResourceCount = SFRAME_RES_COUNT;

		renderData.pGeometry->freeUploaders();
	}


	if (renderData.pGeometry->indexFormat == DXGI_FORMAT_R32_UINT)
	{
		//std::memcpy(renderData.pGeometry->pIndexBufferCPU->GetBufferPointer(), meshData.getIndices32().data(),
		//	renderData.pGeometry->iIndexBufferSizeInBytes);

		renderData.pGeometry->pIndexBufferGPU = SGeometry::createDefaultBuffer(pApp->pDevice.Get(), pApp->pCommandList.Get(), meshData.getIndices32().data(),
			renderData.pGeometry->iIndexBufferSizeInBytes, renderData.pGeometry->pIndexBufferUploader);
	}
	else
	{
		//std::memcpy(renderData.pGeometry->pIndexBufferCPU->GetBufferPointer(), meshData.getIndices16().data(),
		//	renderData.pGeometry->iIndexBufferSizeInBytes);

		renderData.pGeometry->pIndexBufferGPU = SGeometry::createDefaultBuffer(pApp->pDevice.Get(), pApp->pCommandList.Get(), meshData.getIndices16().data(),
			renderData.pGeometry->iIndexBufferSizeInBytes, renderData.pGeometry->pIndexBufferUploader);
	}

	if (bSpawnedInLevel)
	{
		pApp->executeCommandList();
		pApp->flushCommandQueue();
		pApp->mtxSpawnDespawn.unlock();
		pApp->mtxDraw.unlock();
	}
}

void SRuntimeMeshComponent::updateWorldMatrix()
{
	XMStoreFloat4x4(&renderData.vWorld, getWorldMatrix());
}

void SRuntimeMeshComponent::updateMyAndChildsLocationRotationScale()
{
	updateWorldMatrix();

	renderData.iUpdateCBInFrameResourceCount = SFRAME_RES_COUNT;

	for (size_t i = 0; i < vChildComponents.size(); i++)
	{
		vChildComponents[i]->updateMyAndChildsLocationRotationScale();
	}
}

void SRuntimeMeshComponent::addVertexBuffer(SFrameResource* pFrameResource)
{
	if (meshData.getVerticesCount() > 0)
	{
		iIndexInFrameResourceVertexBuffer = pFrameResource->addRuntimeMeshVertexBuffer(meshData.getVerticesCount());
	}
	else
	{
		bNoMeshDataOnSpawn = true;
		iIndexInFrameResourceVertexBuffer = pFrameResource->addRuntimeMeshVertexBuffer(1);
	}

	bIndexOfVertexBufferValid = true;
}

void SRuntimeMeshComponent::removeVertexBuffer(std::vector<std::unique_ptr<SFrameResource>>* vFrameResources)
{
	for (size_t i = 0; i < vFrameResources->size(); i++)
	{
		vFrameResources->operator[](i)->removeRuntimeMeshVertexBuffer(iIndexInFrameResourceVertexBuffer);
	}

	bIndexOfVertexBufferValid = false;
	bNoMeshDataOnSpawn = false;

	pContainer->updateVertexBufferIndexForRuntimeMeshComponents(iIndexInFrameResourceVertexBuffer, 1);

	iIndexInFrameResourceVertexBuffer = 0;
}

void SRuntimeMeshComponent::updateVertexBufferIndex(size_t iIfIndexMoreThatThisValue, size_t iMinusValue)
{
	if (bIndexOfVertexBufferValid)
	{
		if (iIndexInFrameResourceVertexBuffer > iIfIndexMoreThatThisValue)
		{
			iIndexInFrameResourceVertexBuffer -= iMinusValue;
		}
	}
}

void SRuntimeMeshComponent::updateVertexBufferMaxIndex(size_t& iCurrentIndex)
{
	if (iIndexInFrameResourceVertexBuffer > iCurrentIndex)
	{
		iCurrentIndex = iIndexInFrameResourceVertexBuffer;
	}
}