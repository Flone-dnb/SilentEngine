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
#include "SilentEngine/Private/SMiscHelpers/SMiscHelpers.h"


SRuntimeMeshComponent::SRuntimeMeshComponent(std::string sComponentName, bool bDisableFrustumCulling) : SComponent()
{
	componentType = SCT_RUNTIME_MESH;

	this->sComponentName = sComponentName;

	renderData.pGeometry = new SMeshGeometry();

	iIndexInFrameResourceVertexBuffer = 0;

	bVisible = true;
	bIndexOfVertexBufferValid = false;
	bNoMeshDataOnSpawn = false;
	bNewMeshData = false;

	this->bDisableFrustumCulling = bDisableFrustumCulling;
}

SRuntimeMeshComponent::~SRuntimeMeshComponent()
{
	delete renderData.pGeometry;
}

void SRuntimeMeshComponent::setVisibility(bool bVisible)
{
	this->bVisible = bVisible;
}

void SRuntimeMeshComponent::setDisableFrustumCulling(bool bDisable)
{
	this->bDisableFrustumCulling = bDisable;

	if (bDisableFrustumCulling == false)
	{
		mtxDrawComponent.lock();

		if (meshData.getVerticesCount() > 0)
		{
			updateBoundsForFrustumCulling();
		}

		mtxDrawComponent.unlock();
	}
}

bool SRuntimeMeshComponent::setEnableTransparency(bool bEnable)
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

void SRuntimeMeshComponent::setUseCustomShader(SShader* pCustomShader, bool bForceChangeEvenIfSpawned)
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

bool SRuntimeMeshComponent::setUseDefaultShader(bool bForceUseDefaultEvenIfSpawned)
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

void SRuntimeMeshComponent::setMeshData(const SMeshData& meshData, bool bAddedRemovedVerticesOrAddedRemovedIndices)
{
	std::lock_guard<std::mutex> lock(mtxComponentProps);

	mtxDrawComponent.lock();

	this->meshData = meshData;
	bNewMeshData = true;

	if (bDisableFrustumCulling == false)
	{
		updateBoundsForFrustumCulling();

		updateObjectCenter();
	}

	mtxDrawComponent.unlock();

	if (bAddedRemovedVerticesOrAddedRemovedIndices)
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

			pApp->mtxDraw.lock();

			pApp->flushCommandQueue(); // because we recreate the buffer that may be still used by GPU

			for (size_t i = 0; i < pApp->vFrameResources.size(); i++)
			{
				pApp->vFrameResources[i]->recreateRuntimeMeshVertexBuffer(iIndexInFrameResourceVertexBuffer, meshData.getVerticesCount());
			}

			pApp->mtxDraw.unlock();

			mtxDrawComponent.unlock();
			pApp->mtxSpawnDespawn.unlock();
		}
	}
}

void SRuntimeMeshComponent::unbindMaterial()
{
	mtxComponentProps.lock();

	meshData.setMeshMaterial(nullptr);

	mtxComponentProps.unlock();
}

bool SRuntimeMeshComponent::setMeshMaterial(SMaterial* pMaterial)
{
	if (pMaterial->bRegistered)
	{
		if (pMaterial->bUsedInBundle)
		{
			SError::showErrorMessageBox(L"SRuntimeMeshComponent::setMeshMaterial", L"The given material is used in a bundle. It cannot be used here.");

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
		SError::showErrorMessageBox(L"SRuntimeMeshComponent::setMeshMaterial", L"The given material is not registered. Register the material "
			"using the SApplication::registerMaterial() function before using it.");

		return true;
	}
}

void SRuntimeMeshComponent::setCullDistance(float fCullDistance)
{
	this->fCullDistance = fCullDistance;
}

SMaterial* SRuntimeMeshComponent::getMeshMaterial()
{
	return meshData.getMeshMaterial();
}

float SRuntimeMeshComponent::getCullDistance() const
{
	return fCullDistance;
}

bool SRuntimeMeshComponent::getEnableTransparency() const
{
	return bEnableTransparency;
}

bool SRuntimeMeshComponent::setMeshTextureUVOffset(const SVector& vMeshTexUVOffset)
{
	mtxComponentProps.lock();

	bool bResult = renderData.setTextureUVOffset(vMeshTexUVOffset);

	mtxComponentProps.unlock();

	return bResult;
}

void SRuntimeMeshComponent::setTextureUVScale(const SVector& vTextureUVScale)
{
	mtxComponentProps.lock();

	renderData.setTextureUVScale(vTextureUVScale);

	mtxComponentProps.unlock();
}

void SRuntimeMeshComponent::setTextureUVRotation(float fRotation)
{
	mtxComponentProps.lock();

	renderData.setTextureUVRotation(fRotation);

	mtxComponentProps.unlock();
}

void SRuntimeMeshComponent::setCustomShaderProperty(unsigned int iCustomProperty)
{
	mtxComponentProps.lock();

	renderData.iCustomShaderProperty = iCustomProperty;

	renderData.iUpdateCBInFrameResourceCount = SFRAME_RES_COUNT;

	mtxComponentProps.unlock();
}

SVector SRuntimeMeshComponent::getTextureUVOffset() const
{
	return renderData.getTextureUVOffset();
}

SVector SRuntimeMeshComponent::getTextureUVScale() const
{
	return renderData.getTextureUVScale();
}

float SRuntimeMeshComponent::getTextureUVRotation() const
{
	return renderData.getTextureUVRotation();
}

SMeshData* SRuntimeMeshComponent::getMeshData()
{
	return &meshData;
}

SShader* SRuntimeMeshComponent::getCustomShader() const
{
	return pCustomShader;
}

bool SRuntimeMeshComponent::isVisible() const
{
	return bVisible;
}

SRenderItem* SRuntimeMeshComponent::getRenderData()
{
    return &renderData;
}

void SRuntimeMeshComponent::unbindMaterialsIncludingChilds()
{
	unbindMaterial();

	for (size_t i = 0; i < vChildComponents.size(); i++)
	{
		vChildComponents[i]->unbindMaterialsIncludingChilds();
	}
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


	if (renderData.pGeometry->indexFormat == DXGI_FORMAT_R32_UINT)
	{
		//std::memcpy(renderData.pGeometry->pIndexBufferCPU->GetBufferPointer(), meshData.getIndices32().data(),
		//	renderData.pGeometry->iIndexBufferSizeInBytes);

		renderData.pGeometry->pIndexBufferGPU = SMiscHelpers::createBufferWithData(pApp->pDevice.Get(), pApp->pCommandList.Get(), meshData.getIndices32().data(),
			renderData.pGeometry->iIndexBufferSizeInBytes, renderData.pGeometry->pIndexBufferUploader);
	}
	else
	{
		//std::memcpy(renderData.pGeometry->pIndexBufferCPU->GetBufferPointer(), meshData.getIndices16().data(),
		//	renderData.pGeometry->iIndexBufferSizeInBytes);

		renderData.pGeometry->pIndexBufferGPU = SMiscHelpers::createBufferWithData(pApp->pDevice.Get(), pApp->pCommandList.Get(), meshData.getIndices16().data(),
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
	mtxComponentProps.lock();

	DirectX::XMMATRIX world = getWorldMatrix();

	mtxWorldMatrixUpdate.lock();
	XMStoreFloat4x4(&renderData.vWorld, world);
	mtxWorldMatrixUpdate.unlock();

	mtxComponentProps.unlock();
}

void SRuntimeMeshComponent::updateMyAndChildsLocationRotationScale(bool bCalledOnSelf)
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

void SRuntimeMeshComponent::updateObjectCenter()
{
	if (bDisableFrustumCulling == false)
	{
		vObjectCenter = SVector(bounds.Center.x, bounds.Center.y, bounds.Center.z);
	}
}
