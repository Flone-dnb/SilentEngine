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

SMeshComponent::SMeshComponent(std::string sComponentName, bool bUseInstancing) : SComponent()
{
	componentType = SComponentType::SCT_MESH;

	this->sComponentName = sComponentName;

	renderData.pGeometry = new SMeshGeometry();

	bVisible = true;
	bVertexBufferUsedInComputeShader = false;

	this->bUseInstancing = bUseInstancing;
}

SMeshComponent::~SMeshComponent()
{
	delete renderData.pGeometry;
}

void SMeshComponent::setCollisionPreset(SCollisionPreset preset)
{
	if (bVertexBufferUsedInComputeShader)
	{
		SError::showErrorMessageBoxAndLog("can't enable collision for this mesh because its vertex buffer is used in the compute shader.");
	}

	collisionPreset = preset;

	if (meshData.getVerticesCount() > 0)
	{
		std::lock_guard<std::mutex> lock(mtxComponentProps);

		updateObjectBounds();
	}
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

void SMeshComponent::addInstance(const SInstanceProps& instanceData)
{
	if (bUseInstancing)
	{
		std::lock_guard<std::mutex> lock(mtxInstancing);

		vInstanceData.push_back(convertInstancePropsToConstants(instanceData));

		if (bSpawnedInLevel)
		{
			SApplication* pApp = SApplication::getApp();

			pApp->mtxDraw.lock();

			if (vFrameResourcesInstancedData.size() == 0)
			{
				// was empty

				for (size_t i = 0; i < pApp->vFrameResources.size(); i++)
				{
					vFrameResourcesInstancedData.push_back(pApp->vFrameResources[i]->addNewInstancedMesh(&vInstanceData));
				}
			}
			else
			{
				// because we recreate the resource with SObjectConstants (instance data)
				pApp->flushCommandQueue(); // and it may be still used by GPU, wait for the GPU to finish all work

				for (size_t i = 0; i < pApp->vFrameResources.size(); i++)
				{
					vFrameResourcesInstancedData[i] = pApp->vFrameResources[i]->addNewInstanceToMesh(vFrameResourcesInstancedData[i], vInstanceData.back());
				}
			}

			pApp->mtxDraw.unlock();
		}
	}
}

void SMeshComponent::updateInstanceData(unsigned int iInstanceIndex, const SInstanceProps& instanceData)
{
	if (bUseInstancing)
	{
		std::lock_guard<std::mutex> lock(mtxInstancing);

		vInstanceData[iInstanceIndex] = convertInstancePropsToConstants(instanceData);
	}
}

void SMeshComponent::clearAllInstances()
{
	if (bUseInstancing)
	{
		std::lock_guard<std::mutex> lock(mtxInstancing);

		vInstanceData.clear();

		if (bSpawnedInLevel)
		{
			SApplication* pApp = SApplication::getApp();

			pApp->mtxDraw.lock();

			// because we delete the resource with SObjectConstants (instance data)
			pApp->flushCommandQueue(); // and it may be still used by GPU, wait for the GPU to finish all work

			for (size_t i = 0; i < pApp->vFrameResources.size(); i++)
			{
				pApp->vFrameResources[i]->removeInstancedMesh(vFrameResourcesInstancedData[i]);
			}

			vFrameResourcesInstancedData.clear();

			pApp->mtxDraw.unlock();
		}
	}
}

void SMeshComponent::setMeshData(const SMeshData& meshData, bool bAddedRemovedIndices)
{
	std::lock_guard<std::mutex> lock(mtxComponentProps);

	SMaterial* pOldMat = this->meshData.pMeshMaterial;
	this->meshData = meshData;
	this->meshData.pMeshMaterial = pOldMat;

	updateObjectBounds();

	if (bAddedRemovedIndices)
	{
		if (meshData.getVerticesCount() > UINT_MAX)
		{
			SError::showErrorMessageBoxAndLog("the number of vertices in the specified mesh data has exceeded the maximum amount of vertices (the maximum is "
				+ std::to_string(UINT_MAX) + ").");
		}
		else if (meshData.getVerticesCount() * sizeof(SVertex) > UINT_MAX)
		{
			SError::showErrorMessageBoxAndLog("the number of vertices in the specified mesh data is too big, can't continue because an overflow will occur.");
		}

		// to UINT because views require UINT
		renderData.pGeometry->iVertexBufferSizeInBytes = static_cast<UINT>(meshData.getVerticesCount() * sizeof(SVertex));
		renderData.pGeometry->iVertexGraphicsObjectSizeInBytes = static_cast<UINT>(sizeof(SVertex));

		if (meshData.getIndicesCount() > UINT_MAX)
		{
			SError::showErrorMessageBoxAndLog("the number of indices in the specified mesh data has exceeded the maximum amount of indices (the maximum is "
				+ std::to_string(UINT_MAX) + ").");
		}

		if (meshData.hasIndicesMoreThan16Bits())
		{
			if (meshData.getIndicesCount() * sizeof(std::uint32_t) > UINT_MAX)
			{
				SError::showErrorMessageBoxAndLog("the number of indices in the specified mesh data is too big, can't continue because an overflow will occur.");
			}

			renderData.pGeometry->indexFormat = DXGI_FORMAT_R32_UINT;
			renderData.pGeometry->iIndexBufferSizeInBytes = static_cast<UINT>(meshData.getIndicesCount() * sizeof(std::uint32_t));
		}
		else
		{
			if (meshData.getIndicesCount() * sizeof(std::uint16_t) > UINT_MAX)
			{
				SError::showErrorMessageBoxAndLog("the number of indices in the specified mesh data is too big, can't continue because an overflow will occur.");
			}

			renderData.pGeometry->indexFormat = DXGI_FORMAT_R16_UINT;
			renderData.pGeometry->iIndexBufferSizeInBytes = static_cast<UINT>(meshData.getIndicesCount() * sizeof(std::uint16_t));
		}

		renderData.iIndexCount = static_cast<UINT>(meshData.getIndicesCount());
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
			SError::showErrorMessageBoxAndLog("The given material is used in a bundle. It cannot be used here.");

			return true;
		}
		else
		{
			mtxComponentProps.lock();

			meshData.setMeshMaterial(pMaterial);

			mtxComponentProps.unlock();

			return false;
		}
	}
	else
	{
		SError::showErrorMessageBoxAndLog("the given material is not registered. Register the material "
			"using the SApplication::registerMaterial() function before using it.");

		return true;
	}
}

void SMeshComponent::setCullDistance(float fCullDistance)
{
	this->fCullDistance = fCullDistance;
}

SMaterial* SMeshComponent::getMeshMaterial()
{
	return meshData.getMeshMaterial();
}

float SMeshComponent::getCullDistance() const
{
	return fCullDistance;
}

unsigned int SMeshComponent::getInstanceCount()
{
	std::lock_guard<std::mutex> lock(mtxInstancing);

	if (bUseInstancing)
	{
		return static_cast<unsigned int>(vInstanceData.size());
	}
	else
	{
		return 0;
	}
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
			SError::showErrorMessageBoxAndLog("cannot draw as lines because the mesh is using transparency.");
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

SCollisionPreset SMeshComponent::getCollisionPreset() const
{
	return collisionPreset;
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
	if (bUseInstancing)
	{
		SError::showErrorMessageBoxAndLog("cannot use this mesh in a compute shader because this mesh is using instancing.");
		// because if we use instancing we should always do frustum culling
		// as we fill vInstancedMeshes every frame
		return nullptr;
	}

	if (bSpawnedInLevel == false)
	{
		SError::showErrorMessageBoxAndLog("cannot use this mesh in a compute shader because this mesh is not spawned (no buffer was created yet).");
		return nullptr;
	}

	SMeshDataComputeResource* pMeshDataResource = new SMeshDataComputeResource();
	pMeshDataResource->pResourceOwner = this;
	pMeshDataResource->bVertexBuffer = bGetVertexBuffer;

	if (bGetVertexBuffer)
	{
		// compute shader in destructor will set this to 'false'
		bVertexBufferUsedInComputeShader = true;
		collisionPreset = SCollisionPreset::SCP_NO_COLLISION;
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

	HRESULT hresult = S_OK;

	if (bSpawnedInLevel)
	{
		// Do not lock because this func will be
		// called in spawnContainerInLevel() (when bSpawnedInLevel == false) and it will have lock already.

		pApp->mtxDraw.lock();
		pApp->flushCommandQueue(); // vertex/index buffer may be in use currently
		pApp->resetCommandList();
	}

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

			renderData.pGeometry->pIndexBufferGPU = SMiscHelpers::createBufferWithData(pApp->pDevice.Get(), pApp->pCommandList.Get(), meshData.getIndices32()->data(),
				renderData.pGeometry->iIndexBufferSizeInBytes, renderData.pGeometry->pIndexBufferUploader, true);
		}
		else
		{
			//std::memcpy(renderData.pGeometry->pIndexBufferCPU->GetBufferPointer(), meshData.getIndices16().data(),
			//	renderData.pGeometry->iIndexBufferSizeInBytes);

			renderData.pGeometry->pIndexBufferGPU = SMiscHelpers::createBufferWithData(pApp->pDevice.Get(), pApp->pCommandList.Get(), meshData.getIndices16()->data(),
				renderData.pGeometry->iIndexBufferSizeInBytes, renderData.pGeometry->pIndexBufferUploader, true);
		}
	}


	if (bSpawnedInLevel)
	{
		pApp->executeCommandList();
		pApp->flushCommandQueue();
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

SObjectConstants SMeshComponent::convertInstancePropsToConstants(const SInstanceProps& instanceData)
{
	SObjectConstants constants;


	DirectX::XMVECTOR rot = DirectX::XMVectorSet(
		DirectX::XMConvertToRadians(instanceData.vLocalRotation.getX()),
		DirectX::XMConvertToRadians(instanceData.vLocalRotation.getY()),
		DirectX::XMConvertToRadians(instanceData.vLocalRotation.getZ()), 0.0f);
	DirectX::XMFLOAT3 rotationFloat;
	DirectX::XMStoreFloat3(&rotationFloat, rot);

	DirectX::XMMATRIX world = DirectX::XMMatrixIdentity() *
		DirectX::XMMatrixScaling(instanceData.vLocalScale.getX(), instanceData.vLocalScale.getY(), instanceData.vLocalScale.getZ()) *
		DirectX::XMMatrixRotationX(rotationFloat.x) *
		DirectX::XMMatrixRotationY(rotationFloat.y) *
		DirectX::XMMatrixRotationZ(rotationFloat.z) *
		DirectX::XMMatrixTranslation(instanceData.vLocalLocation.getX(), instanceData.vLocalLocation.getY(), instanceData.vLocalLocation.getZ());

	DirectX::XMStoreFloat4x4(&constants.vWorld, world);


	DirectX::XMMATRIX texTransform = DirectX::XMMatrixIdentity() *
		DirectX::XMMatrixTranslation(-0.5f, -0.5f, 0.0f) * // move center to the origin point
		DirectX::XMMatrixScaling(instanceData.vLocalScale.getX(), instanceData.vLocalScale.getY(), instanceData.vLocalScale.getZ()) *
		DirectX::XMMatrixRotationZ(DirectX::XMConvertToRadians(instanceData.fTextureRotation)) *
		DirectX::XMMatrixTranslation(instanceData.vTextureUVOffset.getX(), instanceData.vTextureUVOffset.getY(), instanceData.vTextureUVOffset.getZ()) *
		DirectX::XMMatrixTranslation(0.5f, 0.5f, 0.0f); // move center back.

	DirectX::XMStoreFloat4x4(&constants.vTexTransform, texTransform);


	constants.iCustomProperty = instanceData.iCustomProperty;


	return constants;
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

