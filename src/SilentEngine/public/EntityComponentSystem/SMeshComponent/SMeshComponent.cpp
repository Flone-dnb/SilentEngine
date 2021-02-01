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
#include "SilentEngine/Private/SMiscHelpers/SMiscHelpers.h"

SMeshComponent::SMeshComponent(std::string sComponentName) : SComponent()
{
	componentType = SCT_MESH;

	this->sComponentName = sComponentName;

	renderData.pGeometry = new SMeshGeometry();

	bVisible = true;
	bVertexBufferUsedInComputeShader = false;
}

SMeshComponent::~SMeshComponent()
{
	delete renderData.pGeometry;
}

void SMeshComponent::setVisibility(bool bVisible)
{
	this->bVisible = bVisible;
}

bool SMeshComponent::setEnableTransparency(bool bEnable)
{
	if (bSpawnedInLevel)
	{
		return true;
	}
	else
	{
		bEnableTransparency = bEnable;

		return false;
	}
}

void SMeshComponent::setUseCustomShader(SShader* pCustomShader, bool bForceChangeEvenIfSpawned)
{
	if (pCustomShader == this->pCustomShader) return;

	if (bSpawnedInLevel == false)
	{
		this->pCustomShader = pCustomShader;
	}
	else if (bForceChangeEvenIfSpawned)
	{
		SApplication::getApp()->forceChangeMeshShader(this->pCustomShader, pCustomShader, this, bEnableTransparency);

		this->pCustomShader = pCustomShader;
	}
}

void SMeshComponent::setMeshData(const SMeshData& meshData, bool bAddedRemovedIndices)
{
	std::lock_guard<std::mutex> lock(mtxComponentProps);

	SMaterial* pOldMat = this->meshData.pMeshMaterial;
	this->meshData = meshData;
	this->meshData.pMeshMaterial = pOldMat;


	updateBoundsForFrustumCulling();



	if (bAddedRemovedIndices)
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
		createGeometryBuffers(bAddedRemovedIndices);
	}

	mtxResourceUsed.lock();
	// if used in compute shader
	for (size_t i = 0; i < vResourceUsed.size(); i++)
	{
		vResourceUsed[i].pShader->updateMeshResource(vResourceUsed[i].sResource);
	}
	mtxResourceUsed.unlock();
}

void SMeshComponent::unbindMaterial()
{
	std::lock_guard<std::mutex> lock(mtxComponentProps);

	meshData.setMeshMaterial(nullptr);
}

bool SMeshComponent::setUseDefaultShader(bool bForceUseDefaultEvenIfSpawned)
{
	if (pCustomShader == nullptr) return false;

	if (bSpawnedInLevel == false)
	{
		pCustomShader = nullptr;

		return false;
	}
	else if (bForceUseDefaultEvenIfSpawned)
	{
		SApplication::getApp()->forceChangeMeshShader(this->pCustomShader, nullptr, this, bEnableTransparency);

		this->pCustomShader = nullptr;

		return false;
	}

	return true;
}

bool SMeshComponent::setMeshMaterial(SMaterial* pMaterial)
{
	if (pMaterial->bRegistered)
	{
		if (pMaterial->bUsedInBundle)
		{
			SError::showErrorMessageBox(L"SMeshComponent::setMeshMaterial", L"The given material is used in a bundle. It cannot be used here.");

			return true;
		}
		else
		{
			meshData.setMeshMaterial(pMaterial);

			return false;
		}
	}
	else
	{
		SError::showErrorMessageBox(L"SMeshComponent::setMeshMaterial", L"The given material is not registered. Register the material "
			"using the SApplication::registerMaterial() function before using it.");

		return true;
	}
}

SMaterial* SMeshComponent::getMeshMaterial()
{
	return meshData.getMeshMaterial();
}

bool SMeshComponent::getEnableTransparency() const
{
	return bEnableTransparency;
}

bool SMeshComponent::setMeshTextureUVOffset(const SVector& vMeshTexUVOffset)
{
	mtxComponentProps.lock();

	bool bResult = renderData.setTextureUVOffset(vMeshTexUVOffset);

	mtxComponentProps.unlock();

	return bResult;
}

void SMeshComponent::setTextureUVScale(const SVector& vTextureUVScale)
{
	mtxComponentProps.lock();

	renderData.setTextureUVScale(vTextureUVScale);

	mtxComponentProps.unlock();
}

void SMeshComponent::setTextureUVRotation(float fRotation)
{
	mtxComponentProps.lock();

	renderData.setTextureUVRotation(fRotation);

	mtxComponentProps.unlock();
}

void SMeshComponent::setCustomShaderProperty(unsigned int iCustomProperty)
{
	mtxComponentProps.lock();

	renderData.iCustomShaderProperty = iCustomProperty;

	renderData.iUpdateCBInFrameResourceCount = SFRAME_RES_COUNT;

	mtxComponentProps.unlock();
}

void SMeshComponent::setDrawAsLines(bool bDrawAsLines)
{
	if (bDrawAsLines)
	{
		if (bEnableTransparency)
		{
			SError::showErrorMessageBox(L"SMeshComponent::setDrawAsLines()", L"cannot draw as lines because the mesh is using transparency.");
		}
		else
		{
			renderData.primitiveTopologyType = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
		}
	}
	else
	{
		renderData.primitiveTopologyType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	}
}

SVector SMeshComponent::getTextureUVOffset() const
{
	return renderData.getTextureUVOffset();
}

SVector SMeshComponent::getTextureUVScale() const
{
	return renderData.getTextureUVScale();
}

float SMeshComponent::getTextureUVRotation() const
{
	return renderData.getTextureUVRotation();
}

SMeshData* SMeshComponent::getMeshData()
{
    return &meshData;
}

SShader* SMeshComponent::getCustomShader() const
{
	return pCustomShader;
}

SMeshDataComputeResource* SMeshComponent::getMeshDataAsComputeResource(bool bGetVertexBuffer)
{
	SMeshDataComputeResource* pMeshDataResource = new SMeshDataComputeResource();
	pMeshDataResource->pResourceOwner = this;
	pMeshDataResource->bVertexBuffer = bGetVertexBuffer;

	if (bGetVertexBuffer)
	{
		// compute shader in destructor will set this to 'false'
		bVertexBufferUsedInComputeShader = true;
	}

	return pMeshDataResource;
}

bool SMeshComponent::isVisible() const
{
	return bVisible;
}

void SMeshComponent::unbindMaterialsIncludingChilds()
{
	unbindMaterial();

	for (size_t i = 0; i < vChildComponents.size(); i++)
	{
		vChildComponents[i]->unbindMaterialsIncludingChilds();
	}
}

Microsoft::WRL::ComPtr<ID3D12Resource> SMeshComponent::getResource(bool bVertexBuffer)
{
	if (bVertexBuffer)
	{
		return renderData.pGeometry->pVertexBufferGPU;
	}
	else
	{
		return renderData.pGeometry->pIndexBufferGPU;
	}
}

SRenderItem* SMeshComponent::getRenderData()
{
    return &renderData;
}



void SMeshComponent::createGeometryBuffers(bool bAddedRemovedIndices)
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
		// Do not lock because this func will be
		// called in spawnContainerInLevel() (when bSpawnedInLevel == false) and it will have lock already.

		pApp->mtxDraw.lock();
		pApp->mtxSpawnDespawn.lock();
		pApp->flushCommandQueue(); // vertex/index buffer may be in use currently
		pApp->resetCommandList();
	}

	// ?
	//renderData.iUpdateCBInFrameResourceCount = SFRAME_RES_COUNT;

	renderData.pGeometry->freeUploaders();


	// Create all with UAV flag/state so it can be easily used in compute shader as RW buffer.
	renderData.pGeometry->pVertexBufferGPU = SMiscHelpers::createBufferWithData(pApp->pDevice.Get(), pApp->pCommandList.Get(), vShaderVertices.data(),
		renderData.pGeometry->iVertexBufferSizeInBytes, renderData.pGeometry->pVertexBufferUploader, true);

	if (bAddedRemovedIndices)
	{
		if (renderData.pGeometry->indexFormat == DXGI_FORMAT_R32_UINT)
		{
			//std::memcpy(renderData.pGeometry->pIndexBufferCPU->GetBufferPointer(), meshData.getIndices32().data(),
			//	renderData.pGeometry->iIndexBufferSizeInBytes);

			renderData.pGeometry->pIndexBufferGPU = SMiscHelpers::createBufferWithData(pApp->pDevice.Get(), pApp->pCommandList.Get(), meshData.getIndices32().data(),
				renderData.pGeometry->iIndexBufferSizeInBytes, renderData.pGeometry->pIndexBufferUploader, true);
		}
		else
		{
			//std::memcpy(renderData.pGeometry->pIndexBufferCPU->GetBufferPointer(), meshData.getIndices16().data(),
			//	renderData.pGeometry->iIndexBufferSizeInBytes);

			renderData.pGeometry->pIndexBufferGPU = SMiscHelpers::createBufferWithData(pApp->pDevice.Get(), pApp->pCommandList.Get(), meshData.getIndices16().data(),
				renderData.pGeometry->iIndexBufferSizeInBytes, renderData.pGeometry->pIndexBufferUploader, true);
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

void SMeshComponent::updateMyAndChildsLocationRotationScale(bool bCalledOnSelf)
{
	updateWorldMatrix();

	renderData.iUpdateCBInFrameResourceCount = SFRAME_RES_COUNT;


	if ((bCalledOnSelf == false) && onParentLocationRotationScaleChangedCallback)
	{
		onParentLocationRotationScaleChangedCallback(this);
	}


	for (size_t i = 0; i < vChildComponents.size(); i++)
	{
		vChildComponents[i]->updateMyAndChildsLocationRotationScale(false);
	}
}

void SMeshComponent::updateWorldMatrix()
{
	mtxComponentProps.lock();

	DirectX::XMMATRIX world = getWorldMatrix();

	mtxWorldMatrixUpdate.lock();
	XMStoreFloat4x4(&renderData.vWorld, world);
	mtxWorldMatrixUpdate.unlock();

	mtxComponentProps.unlock();
}

