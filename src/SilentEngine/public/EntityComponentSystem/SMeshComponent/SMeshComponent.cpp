// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "SMeshComponent.h"

// DirectX
#include <D3Dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

// Custom
#include "SilentEngine/Private/SError/SError.h"
#include "SilentEngine/Public/SApplication/SApplication.h"
#include "SilentEngine/Public/EntityComponentSystem/SContainer/SContainer.h"

SMeshComponent::SMeshComponent(std::string sComponentName)
{
	componentType = SCT_MESH;

	this->sComponentName = sComponentName;

	renderData.pGeometry = new SMeshGeometry();

	bVisible = true;
}

SMeshComponent::~SMeshComponent()
{
	delete renderData.pGeometry;
}

void SMeshComponent::setVisibility(bool bVisible)
{
	this->bVisible = bVisible;
}

void SMeshComponent::setMeshData(const SMeshData& meshData, bool bAddedVerticesOrUpdatedIndicesCount, bool bUpdateBoundingBox)
{
	this->meshData = meshData;

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
	}

	if (bSpawnedInLevel)
	{
		createGeometryBuffers(bAddedVerticesOrUpdatedIndicesCount);
	}
}

SMeshData SMeshComponent::getMeshData() const
{
    return meshData;
}

bool SMeshComponent::isVisible() const
{
	return bVisible;
}

SRenderItem* SMeshComponent::getRenderData()
{
    return &renderData;
}

void SMeshComponent::createGeometryBuffers(bool bAddedVerticesOrUpdatedIndicesCount)
{
	SApplication* pApp = SApplication::getApp();

	std::vector<SVertex> vShaderVertices = meshData.toShaderVertex();

	HRESULT hresult;

	// do we need to create them?
	/*if (bAddedVerticesOrUpdatedIndicesCount)
	{
		hresult = D3DCreateBlob(renderData.pGeometry->iVertexBufferSizeInBytes, &renderData.pGeometry->pVertexBufferCPU);
		if (FAILED(hresult))
		{
			SError::showErrorMessageBox(hresult, L"SMeshComponent::createGeometryBuffers::D3DCreateBlob()");
			return;
		}
	}

	std::memcpy(renderData.pGeometry->pVertexBufferCPU->GetBufferPointer(), vShaderVertices.data(), renderData.pGeometry->iVertexBufferSizeInBytes);

	if (bAddedVerticesOrUpdatedIndicesCount)
	{
		hresult = D3DCreateBlob(renderData.pGeometry->iIndexBufferSizeInBytes, &renderData.pGeometry->pIndexBufferCPU);
		if (FAILED(hresult))
		{
			SError::showErrorMessageBox(hresult, L"SMeshComponent::createGeometryBuffers::D3DCreateBlob()");
			return;
		}
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

	renderData.pGeometry->pVertexBufferGPU = SGeometry::createDefaultBuffer(pApp->pDevice.Get(), pApp->pCommandList.Get(), vShaderVertices.data(),
		renderData.pGeometry->iVertexBufferSizeInBytes, renderData.pGeometry->pVertexBufferUploader);

	if (bAddedVerticesOrUpdatedIndicesCount)
	{
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
	}

	if (bSpawnedInLevel)
	{
		pApp->executeCommandList();
		pApp->flushCommandQueue();
		pApp->mtxSpawnDespawn.unlock();
		pApp->mtxDraw.unlock();
	}
}

void SMeshComponent::updateMyAndChildsLocationRotationScale()
{
	updateWorldMatrix();

	renderData.iUpdateCBInFrameResourceCount = SFRAME_RES_COUNT;

	for (size_t i = 0; i < vChildComponents.size(); i++)
	{
		vChildComponents[i]->updateMyAndChildsLocationRotationScale();
	}
}

void SMeshComponent::updateWorldMatrix()
{
	XMStoreFloat4x4(&renderData.vWorld, getWorldMatrix());
}
