// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "SComponent.h"

#include "SilentEngine/Public/EntityComponentSystem/SContainer/SContainer.h"
#include "SilentEngine/Public/EntityComponentSystem/SMeshComponent/SMeshComponent.h"
#include "SilentEngine/Public/EntityComponentSystem/SRuntimeMeshComponent/SRuntimeMeshComponent.h"
#include "SilentEngine/Private/SError/SError.h"
#include "SilentEngine/Private/SShader/SShader.h"
#include "SilentEngine/Public/SApplication/SApplication.h"
#include "SilentEngine/Public/SPrimitiveShapeGenerator/SPrimitiveShapeGenerator.h"
#include "SilentEngine/Public/EntityComponentSystem/SAudioComponent/SAudioComponent.h"

SComponent::SComponent()
{
	componentType = SComponentType::SCT_NONE;

	sComponentName = "";

	bSpawnedInLevel = false;
	bVisible = true;
	bEnableTransparency = false;

	pParentComponent = nullptr;
	pContainer       = nullptr;
	pCustomShader    = nullptr;

	vObjectCenter = SVector(0.0f, 0.0f, 0.0f);

	iMeshComponentsCount  = 0;
	fCullDistance         = -1.0f;

	vLocation             = SVector(0.0f, 0.0f, 0.0f);
	vRotation             = SVector(0.0f, 0.0f, 0.0f);
	vScale                = SVector(1.0f, 1.0f, 1.0f);

	vLocalXAxisVector     = SVector(1.0f, 0.0f, 0.0f);
	vLocalYAxisVector     = SVector(0.0f, 1.0f, 0.0f);
	vLocalZAxisVector     = SVector(0.0f, 0.0f, 1.0f);

	collisionPreset = SCollisionPreset::SCP_BOX;
}

SComponent::~SComponent()
{
	if (bSpawnedInLevel)
	{
		SError::showErrorMessageBoxAndLog("component destructor is called while the component is spawned. Despawn the component first.");
	}

	for (int i = 0; i < vResourceUsed.size(); i++)
	{
		if (SApplication::getApp()->doesComputeShaderExists(vResourceUsed[i].pShader) == false)
		{
			// Don't notify about setMeshData anymore.
			vResourceUsed.erase(vResourceUsed.begin() + i);

			i--;
		}
	}
	

	for (size_t i = 0; i < vChildComponents.size(); i++)
	{
		delete vChildComponents[i];
	}
}

bool SComponent::addChildComponent(SComponent* pComponent)
{
	if (this == pComponent)
	{
		return true;
	}

	if (bSpawnedInLevel)
	{
		SError::showErrorMessageBoxAndLog("cannot add a component when the parent component is already spawned.");
		return true;
	}
	else
	{
		if (pContainer)
		{
			if (pComponent->pContainer)
			{
				SError::showErrorMessageBoxAndLog("component already added to other container/component.");
				return true;
			}

			SComponent* pComponentWithSameName = nullptr;
			pComponentWithSameName = pContainer->getComponentByName(pComponent->getComponentName());

			if (pComponentWithSameName == nullptr)
			{
				if (pComponent->componentType == SComponentType::SCT_MESH || pComponent->componentType == SComponentType::SCT_RUNTIME_MESH)
				{
					iMeshComponentsCount++;
				}

				vChildComponents.push_back(pComponent);
				pComponent->setParentComponent(this);
				pComponent->setContainer(pContainer);

				return false;
			}
			else
			{
				return true;
			}
		}
		else
		{
			SError::showErrorMessageBoxAndLog("The component has no parent and it can't have child "
				"components yet. Add the component to the container or other component and then use this function.");
			return true;
		}
	}
}

bool SComponent::removeChildComponent(SComponent* pComponent)
{
	if (bSpawnedInLevel)
	{
		return true;
	}
	else
	{
		for (size_t i = 0; i < vChildComponents.size(); i++)
		{
			if (vChildComponents[i] == pComponent)
			{
				if (pComponent->componentType == SComponentType::SCT_MESH || pComponent->componentType == SComponentType::SCT_RUNTIME_MESH)
				{
					iMeshComponentsCount--;
				}

				pComponent->setParentComponent(nullptr);
				pComponent->setContainer(nullptr);

				vChildComponents.erase( vChildComponents.begin() + i );

				break;
			}
		}

		return false;
	}
}

void SComponent::setLocalLocation(const SVector& location)
{
	if (!pContainer)
	{
		SError::showErrorMessageBoxAndLog("add the component to a container or other component first.");
		return;
	}

	SVector vParentOriginLocation;
	SVector vParentXAxisVector, vParentYAxisVector, vParentZAxisVector;

	if (pParentComponent)
	{
		vParentOriginLocation = pParentComponent->getLocalLocation();
		pParentComponent->getComponentLocalAxis(&vParentXAxisVector, &vParentYAxisVector, &vParentZAxisVector);
	}
	else
	{
		vParentOriginLocation = pContainer->getLocation();
		pContainer->getLocalAxis(&vParentXAxisVector, &vParentYAxisVector, &vParentZAxisVector);
	}


	vLocation = vParentXAxisVector * location + vParentYAxisVector * location + vParentZAxisVector * location;

	updateMyAndChildsLocationRotationScale(true);
}

void SComponent::setLocalRotation(const SVector& rotation)
{
	if (!pContainer)
	{
		SError::showErrorMessageBoxAndLog("Add the component to a container or other component first.");
		return;
	}

	vRotation = rotation;

	// Rotate Axis.

	DirectX::XMVECTOR rot = DirectX::XMVectorSet(DirectX::XMConvertToRadians(vRotation.getX()), DirectX::XMConvertToRadians(vRotation.getY()),
		DirectX::XMConvertToRadians(vRotation.getZ()), 0.0f);

	DirectX::XMFLOAT3 rotationFloat;
	DirectX::XMStoreFloat3(&rotationFloat, rot);

	DirectX::XMMATRIX rotationMat = DirectX::XMMatrixIdentity() *  DirectX::XMMatrixRotationX(rotationFloat.x)
		* DirectX::XMMatrixRotationY(rotationFloat.y) * DirectX::XMMatrixRotationZ(rotationFloat.z);

	DirectX::XMFLOAT4X4 rotMat;
	DirectX::XMStoreFloat4x4(&rotMat, rotationMat);


	vLocalXAxisVector = SVector(rotMat._11, rotMat._12, rotMat._13);
	vLocalYAxisVector = SVector(rotMat._21, rotMat._22, rotMat._23);
	vLocalZAxisVector = SVector(rotMat._31, rotMat._32, rotMat._33);

	updateMyAndChildsLocationRotationScale(true);
}

void SComponent::setLocalScale(const SVector& scale)
{
	if (!pContainer)
	{
		SError::showErrorMessageBoxAndLog("Add the component to a container or other component first.");
		return;
	}

	SVector vScaleBefore = vScale;
	vScale = scale;

	updateMyAndChildsLocationRotationScale(true);
}

bool SComponent::setComponentName(const std::string& sComponentName)
{
	if (bSpawnedInLevel == false)
	{
		this->sComponentName = sComponentName;
		return false;
	}
	else
	{
		return true;
	}
}

void SComponent::setParentComponent(SComponent* pComponent)
{
	pParentComponent = pComponent;
}

void SComponent::createVertexBufferForRuntimeMeshComponents(SFrameResource* pFrameResource)
{
	if (componentType == SComponentType::SCT_RUNTIME_MESH)
	{
		SRuntimeMeshComponent* pRuntimeMesh = dynamic_cast<SRuntimeMeshComponent*>(this);
		pRuntimeMesh->addVertexBuffer(pFrameResource);
	}

	for (size_t i = 0; i < vChildComponents.size(); i++)
	{
		vChildComponents[i]->createVertexBufferForRuntimeMeshComponents(pFrameResource);
	}
}

void SComponent::createInstancingDataForFrameResource(std::vector<std::unique_ptr<SFrameResource>>* vFrameResources)
{
	if (componentType == SComponentType::SCT_MESH)
	{
		SMeshComponent* pMesh = dynamic_cast<SMeshComponent*>(this);
		
		if (pMesh->bUseInstancing)
		{
			pMesh->mtxInstancing.lock();

			for (size_t i = 0; i < vFrameResources->size(); i++)
			{
				pMesh->vFrameResourcesInstancedData.push_back(vFrameResources->operator[](i)->addNewInstancedMesh(&pMesh->vInstanceData));
			}
			
			pMesh->mtxInstancing.unlock();
		}
	}

	for (size_t i = 0; i < vChildComponents.size(); i++)
	{
		vChildComponents[i]->createInstancingDataForFrameResource(vFrameResources);
	}
}

void SComponent::getRequiredShadowMapCount(size_t& iDSVCount)
{
	if (componentType == SComponentType::SCT_LIGHT)
	{
		SLightComponent* pLight = dynamic_cast<SLightComponent*>(this);
		pLight->getRequiredShadowMapCount(iDSVCount);
	}

	for (size_t i = 0; i < vChildComponents.size(); i++)
	{
		vChildComponents[i]->getRequiredShadowMapCount(iDSVCount);
	}
}

void SComponent::allocateShadowMapCBsForLightComponents(std::vector<std::unique_ptr<SFrameResource>>* vFrameResources, ID3D12Device* pDevice,
	CD3DX12_CPU_DESCRIPTOR_HANDLE& dsvHeapHandle, UINT iDSVDescriptorSize,
	CD3DX12_CPU_DESCRIPTOR_HANDLE& srvCpuHeapHandle, CD3DX12_GPU_DESCRIPTOR_HANDLE& srvGpuHeapHandle,
	UINT iSRVDescriptorSize)
{
	if (componentType == SComponentType::SCT_LIGHT)
	{
		SLightComponent* pLight = dynamic_cast<SLightComponent*>(this);
		pLight->allocateShadowMaps(vFrameResources, pDevice, dsvHeapHandle, iDSVDescriptorSize, srvCpuHeapHandle, srvGpuHeapHandle, iSRVDescriptorSize);
	}

	for (size_t i = 0; i < vChildComponents.size(); i++)
	{
		vChildComponents[i]->allocateShadowMapCBsForLightComponents(
			vFrameResources, pDevice, dsvHeapHandle, iDSVDescriptorSize, srvCpuHeapHandle, srvGpuHeapHandle, iSRVDescriptorSize);
	}
}

void SComponent::deallocateShadowMapCBsForLightComponents(std::vector<std::unique_ptr<SFrameResource>>* vFrameResources)
{
	if (componentType == SComponentType::SCT_LIGHT)
	{
		SLightComponent* pLight = dynamic_cast<SLightComponent*>(this);
		pLight->deallocateShadowMaps(vFrameResources);
	}

	for (size_t i = 0; i < vChildComponents.size(); i++)
	{
		vChildComponents[i]->deallocateShadowMapCBsForLightComponents(vFrameResources);
	}
}

void SComponent::removeVertexBufferForRuntimeMeshComponents(std::vector<std::unique_ptr<SFrameResource>>* vFrameResources, size_t& iRemovedCount)
{
	if (componentType == SComponentType::SCT_RUNTIME_MESH)
	{
		SRuntimeMeshComponent* pRuntimeMesh = dynamic_cast<SRuntimeMeshComponent*>(this);
		pRuntimeMesh->removeVertexBuffer(vFrameResources);

		iRemovedCount++;
	}

	for (size_t i = 0; i < vChildComponents.size(); i++)
	{
		vChildComponents[i]->removeVertexBufferForRuntimeMeshComponents(vFrameResources, iRemovedCount);
	}
}

void SComponent::removeInstancingDataForFrameResources(std::vector<std::unique_ptr<SFrameResource>>* vFrameResources)
{
	if (componentType == SComponentType::SCT_MESH)
	{
		SMeshComponent* pMesh = dynamic_cast<SMeshComponent*>(this);

		if (pMesh->bUseInstancing && pMesh->vFrameResourcesInstancedData.size() > 0)
		{
			pMesh->mtxInstancing.lock();

			for (size_t i = 0; i < vFrameResources->size(); i++)
			{
				vFrameResources->operator[](i)->removeInstancedMesh(pMesh->vFrameResourcesInstancedData[i]);
			}

			pMesh->mtxInstancing.unlock();
		}
	}

	for (size_t i = 0; i < vChildComponents.size(); i++)
	{
		vChildComponents[i]->removeInstancingDataForFrameResources(vFrameResources);
	}
}

void SComponent::getMaxVertexBufferIndexForRuntimeMeshComponents(size_t& iMaxIndex)
{
	if (componentType == SComponentType::SCT_RUNTIME_MESH)
	{
		SRuntimeMeshComponent* pRuntimeMesh = dynamic_cast<SRuntimeMeshComponent*>(this);
		pRuntimeMesh->updateVertexBufferMaxIndex(iMaxIndex);
	}

	for (size_t i = 0; i < vChildComponents.size(); i++)
	{
		vChildComponents[i]->getMaxVertexBufferIndexForRuntimeMeshComponents(iMaxIndex);
	}
}

void SComponent::updateVertexBufferIndexForRuntimeMeshComponents(size_t iIfIndexMoreThatThisValue, size_t iMinusValue)
{
	if (componentType == SComponentType::SCT_RUNTIME_MESH)
	{
		SRuntimeMeshComponent* pRuntimeMesh = dynamic_cast<SRuntimeMeshComponent*>(this);
		pRuntimeMesh->updateVertexBufferIndex(iIfIndexMoreThatThisValue, iMinusValue);
	}

	for (size_t i = 0; i < vChildComponents.size(); i++)
	{
		vChildComponents[i]->updateVertexBufferIndexForRuntimeMeshComponents(iIfIndexMoreThatThisValue, iMinusValue);
	}
}

DirectX::XMMATRIX XM_CALLCONV SComponent::getWorldMatrix()
{
	DirectX::XMMATRIX parentWorld = DirectX::XMMatrixIdentity();

	if (pParentComponent)
	{
		parentWorld = pParentComponent->getWorldMatrix();
	}
	else
	{
		SVector vParentLocationInWorld = pContainer->getLocation();
		SVector vParentScale           = pContainer->getScale();
		SVector vParentRotation        = pContainer->getRotation();

		DirectX::XMVECTOR rot = DirectX::XMVectorSet(DirectX::XMConvertToRadians(vParentRotation.getX()), DirectX::XMConvertToRadians(vParentRotation.getY()),
			DirectX::XMConvertToRadians(vParentRotation.getZ()), 0.0f);
		DirectX::XMFLOAT3 rotationFloat;
		DirectX::XMStoreFloat3(&rotationFloat, rot);
		
		parentWorld *= DirectX::XMMatrixScaling(vParentScale.getX(), vParentScale.getY(), vParentScale.getZ());
		parentWorld *=  DirectX::XMMatrixRotationX(rotationFloat.x) * DirectX::XMMatrixRotationY(rotationFloat.y) *
			DirectX::XMMatrixRotationZ(rotationFloat.z);
		parentWorld *= DirectX::XMMatrixTranslation(vParentLocationInWorld.getX(), vParentLocationInWorld.getY(), vParentLocationInWorld.getZ());
	}

	DirectX::XMVECTOR rot = DirectX::XMVectorSet(DirectX::XMConvertToRadians(vRotation.getX()), DirectX::XMConvertToRadians(vRotation.getY()),
		DirectX::XMConvertToRadians(vRotation.getZ()), 0.0f);
	DirectX::XMFLOAT3 rotationFloat;
	DirectX::XMStoreFloat3(&rotationFloat, rot);

	DirectX::XMMATRIX myWorld = DirectX::XMMatrixIdentity() * 
		DirectX::XMMatrixScaling(vScale.getX(), vScale.getY(), vScale.getZ()) * 
		DirectX::XMMatrixRotationX(rotationFloat.x) * DirectX::XMMatrixRotationY(rotationFloat.y) * DirectX::XMMatrixRotationZ(rotationFloat.z) *
		DirectX::XMMatrixTranslation(vLocation.getX(), vLocation.getY(), vLocation.getZ());

	return myWorld * parentWorld;
}

void SComponent::addMeshesByShader(std::vector<SShaderObjects>* pOpaqueMeshesByShader, std::vector<SShaderObjects>* pTransparentMeshesByShader) const
{
	if (componentType == SComponentType::SCT_MESH || componentType == SComponentType::SCT_RUNTIME_MESH)
	{
		std::vector<SShaderObjects>* pVectorToLook;

		if (bEnableTransparency)
		{
			pVectorToLook = pTransparentMeshesByShader;
		}
		else
		{
			pVectorToLook = pOpaqueMeshesByShader;
		}


		bool bFound = false;

		for (size_t i = 0; i < pVectorToLook->size(); i++)
		{
			if (pVectorToLook->operator[](i).pShader == pCustomShader)
			{
				pVectorToLook->operator[](i).vMeshComponentsWithThisShader.push_back(const_cast<SComponent*>(this));

				bFound = true;

				break;
			}
		}

		if (bFound == false)
		{
			SShaderObjects so;
			so.pShader = pCustomShader;
			so.vMeshComponentsWithThisShader.push_back(const_cast<SComponent*>(this));

			pVectorToLook->push_back(so);
		}
	}

	for (size_t i = 0; i < vChildComponents.size(); i++)
	{
		vChildComponents[i]->addMeshesByShader(pOpaqueMeshesByShader, pTransparentMeshesByShader);
	}
}

void SComponent::removeMeshesByShader(std::vector<SShaderObjects>* pOpaqueMeshesByShader, std::vector<SShaderObjects>* pTransparentMeshesByShader) const
{
	if (componentType == SComponentType::SCT_MESH || componentType == SComponentType::SCT_RUNTIME_MESH)
	{
		std::vector<SShaderObjects>* pVectorToLook;

		if (bEnableTransparency)
		{
			pVectorToLook = pTransparentMeshesByShader;
		}
		else
		{
			pVectorToLook = pOpaqueMeshesByShader;
		}


		bool bFound = false;

		for (size_t i = 0; i < pVectorToLook->size(); i++)
		{
			if (pVectorToLook->operator[](i).pShader == pCustomShader)
			{
				bool bFoundObject = false;

				for (size_t k = 0; k < pVectorToLook->operator[](i).vMeshComponentsWithThisShader.size(); k++)
				{
					if (pVectorToLook->operator[](i).vMeshComponentsWithThisShader[k] == this)
					{
						bFoundObject = true;

						pVectorToLook->operator[](i).vMeshComponentsWithThisShader.erase(
							pVectorToLook->operator[](i).vMeshComponentsWithThisShader.begin() + k);

						break;
					}
				}

				if (bFoundObject == false)
				{
					SError::showErrorMessageBoxAndLog("could not find the object in the shader array.");
				}

				if ((pVectorToLook->operator[](i).vMeshComponentsWithThisShader.size() == 0) && (pVectorToLook->operator[](i).pShader != nullptr))
				{
					pVectorToLook->erase(pVectorToLook->begin() + i);
				}

				bFound = true;

				break;
			}
		}

		if (bFound == false)
		{
			SError::showErrorMessageBoxAndLog("could not find the object by the given shader in the array.");
		}
	}

	for (size_t i = 0; i < vChildComponents.size(); i++)
	{
		vChildComponents[i]->removeMeshesByShader(pOpaqueMeshesByShader, pTransparentMeshesByShader);
	}
}

void SComponent::registerAll3DSoundComponents()
{
	if (componentType == SComponentType::SCT_AUDIO)
	{
		SApplication::getApp()->getAudioEngine()->registerNew3DAudioComponent(dynamic_cast<SAudioComponent*>(this));
	}

	for (size_t i = 0; i < vChildComponents.size(); i++)
	{
		vChildComponents[i]->registerAll3DSoundComponents();
	}
}

void SComponent::unregisterAll3DSoundComponents()
{
	if (componentType == SComponentType::SCT_AUDIO)
	{
		SApplication::getApp()->getAudioEngine()->unregister3DAudioComponent(dynamic_cast<SAudioComponent*>(this));
	}

	for (size_t i = 0; i < vChildComponents.size(); i++)
	{
		vChildComponents[i]->unregisterAll3DSoundComponents();
	}
}

void SComponent::bindResourceUpdates(SComputeShader* pShader, const std::string& sResourceName)
{
	SComputeResourceBind bind;
	bind.pShader = pShader;
	bind.sResource = sResourceName;

	mtxResourceUsed.lock();
	vResourceUsed.push_back(bind);
	mtxResourceUsed.unlock();
}

void SComponent::unbindResourceUpdates(SComputeShader* pShader)
{
	mtxResourceUsed.lock();

	for (size_t i = 0; i < vResourceUsed.size(); i++)
	{
		if (vResourceUsed[i].pShader == pShader)
		{
			vResourceUsed.erase(vResourceUsed.begin() + i);

			break;
		}
	}

	mtxResourceUsed.unlock();
}

void SComponent::updateObjectBounds()
{
	DirectX::XMFLOAT3 vMinf3(FLT_MAX, FLT_MAX, FLT_MAX);
	DirectX::XMFLOAT3 vMaxf3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	DirectX::XMVECTOR vMin = XMLoadFloat3(&vMinf3);
	DirectX::XMVECTOR vMax = XMLoadFloat3(&vMaxf3);

	std::vector<SMeshVertex>* pvVerts = meshData.getVertices();

	for (size_t i = 0; i < pvVerts->size(); i++)
	{
		DirectX::XMVECTOR P = DirectX::XMLoadFloat3(&pvVerts->operator[](i).vPosition);

		vMin = DirectX::XMVectorMin(vMin, P);
		vMax = DirectX::XMVectorMax(vMax, P);
	}

	using namespace DirectX; // for easy + and - operators
	DirectX::XMStoreFloat3(&boxCollision.Center, 0.5f * (vMin + vMax));
	DirectX::XMStoreFloat3(&boxCollision.Extents, 0.5f * (vMax - vMin));

	vObjectCenter = SVector(boxCollision.Center.x, boxCollision.Center.y, boxCollision.Center.z);

	if (collisionPreset == SCollisionPreset::SCP_SPHERE)
	{
		updateSphereBounds();
	}
}

void SComponent::updateSphereBounds()
{
	// CreateFromPoints() version for SMeshVertex

	// Find the points with minimum and maximum x, y, and z
	DirectX::XMFLOAT3 Min3X(FLT_MAX, FLT_MAX, FLT_MAX), Max3X(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	DirectX::XMFLOAT3 Min3Y(FLT_MAX, FLT_MAX, FLT_MAX), Max3Y(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	DirectX::XMFLOAT3 Min3Z(FLT_MAX, FLT_MAX, FLT_MAX), Max3Z(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	DirectX::XMVECTOR MinX = DirectX::XMLoadFloat3(&Min3X);
	DirectX::XMVECTOR MaxX = DirectX::XMLoadFloat3(&Max3X);

	DirectX::XMVECTOR MinY = DirectX::XMLoadFloat3(&Min3Y);
	DirectX::XMVECTOR MaxY = DirectX::XMLoadFloat3(&Max3Y);

	DirectX::XMVECTOR MinZ = DirectX::XMLoadFloat3(&Min3Z);
	DirectX::XMVECTOR MaxZ = DirectX::XMLoadFloat3(&Max3Z);

	std::vector<SMeshVertex>* pvVerts = meshData.getVertices();

	for (size_t i = 0; i < pvVerts->size(); i++)
	{
		DirectX::XMVECTOR Point = DirectX::XMLoadFloat3(&pvVerts->operator[](i).vPosition);

		float px = DirectX::XMVectorGetX(Point);
		float py = DirectX::XMVectorGetY(Point);
		float pz = DirectX::XMVectorGetZ(Point);

		if (px < DirectX::XMVectorGetX(MinX))
			MinX = Point;

		if (px > DirectX::XMVectorGetX(MaxX))
			MaxX = Point;

		if (py < DirectX::XMVectorGetY(MinY))
			MinY = Point;

		if (py > DirectX::XMVectorGetY(MaxY))
			MaxY = Point;

		if (pz < DirectX::XMVectorGetZ(MinZ))
			MinZ = Point;

		if (pz > DirectX::XMVectorGetZ(MaxZ))
			MaxZ = Point;
	}

	// Use the min/max pair that are farthest apart to form the initial sphere.
	DirectX::XMVECTOR DeltaX = DirectX::XMVectorSubtract(MaxX, MinX);
	DirectX::XMVECTOR DistX = DirectX::XMVector3Length(DeltaX);

	DirectX::XMVECTOR DeltaY = DirectX::XMVectorSubtract(MaxY, MinY);
	DirectX::XMVECTOR DistY = DirectX::XMVector3Length(DeltaY);

	DirectX::XMVECTOR DeltaZ = DirectX::XMVectorSubtract(MaxZ, MinZ);
	DirectX::XMVECTOR DistZ = DirectX::XMVector3Length(DeltaZ);

	DirectX::XMVECTOR vCenter;
	DirectX::XMVECTOR vRadius;

	if (DirectX::XMVector3Greater(DistX, DistY))
	{
		if (DirectX::XMVector3Greater(DistX, DistZ))
		{
			// Use min/max x.
			vCenter = DirectX::XMVectorLerp(MaxX, MinX, 0.5f);
			vRadius = DirectX::XMVectorScale(DistX, 0.5f);
		}
		else
		{
			// Use min/max z.
			vCenter = DirectX::XMVectorLerp(MaxZ, MinZ, 0.5f);
			vRadius = DirectX::XMVectorScale(DistZ, 0.5f);
		}
	}
	else // Y >= X
	{
		if (DirectX::XMVector3Greater(DistY, DistZ))
		{
			// Use min/max y.
			vCenter = DirectX::XMVectorLerp(MaxY, MinY, 0.5f);
			vRadius = DirectX::XMVectorScale(DistY, 0.5f);
		}
		else
		{
			// Use min/max z.
			vCenter = DirectX::XMVectorLerp(MaxZ, MinZ, 0.5f);
			vRadius = DirectX::XMVectorScale(DistZ, 0.5f);
		}
	}

	// Add any points not inside the sphere.
	for (size_t i = 0; i < pvVerts->size(); i++)
	{
		DirectX::XMVECTOR Point = DirectX::XMLoadFloat3(&pvVerts->operator[](i).vPosition);

		DirectX::XMVECTOR Delta = DirectX::XMVectorSubtract(Point, vCenter);

		DirectX::XMVECTOR Dist = DirectX::XMVector3Length(Delta);

		if (DirectX::XMVector3Greater(Dist, vRadius))
		{
			// Adjust sphere to include the new point.
			vRadius = DirectX::XMVectorScale(DirectX::XMVectorAdd(vRadius, Dist), 0.5f);
			vCenter = DirectX::XMVectorAdd(vCenter, DirectX::XMVectorMultiply(DirectX::XMVectorSubtract(DirectX::XMVectorReplicate(1.0f), DirectX::XMVectorDivide(vRadius, Dist)), Delta));
		}
	}

	DirectX::XMStoreFloat3(&sphereCollision.Center, vCenter);
	DirectX::XMStoreFloat(&sphereCollision.Radius, vRadius);
}

void SComponent::getAllMeshComponents(std::vector<SComponent*>* pvOpaqueComponents, std::vector<SComponent*>* pvTransparentComponents)
{
	if (componentType == SComponentType::SCT_MESH || componentType == SComponentType::SCT_RUNTIME_MESH)
	{
		if (bEnableTransparency)
		{
			pvTransparentComponents->push_back(this);
		}
		else
		{
			pvOpaqueComponents->push_back(this);
		}
	}

	for (size_t i = 0; i < vChildComponents.size(); i++)
	{
		vChildComponents[i]->getAllMeshComponents(pvOpaqueComponents, pvTransparentComponents);
	}
}

size_t SComponent::getMeshComponentsCount() const
{
	size_t iCount = iMeshComponentsCount;

	for (size_t i = 0; i < vChildComponents.size(); i++)
	{
		iCount += vChildComponents[i]->getMeshComponentsCount();
	}

	return iCount;
}

size_t SComponent::getLightComponentsCount() const
{
	size_t iCount = 0;

	if (componentType == SComponentType::SCT_LIGHT)
	{
		iCount++;
	}

	for (size_t i = 0; i < vChildComponents.size(); i++)
	{
		iCount += vChildComponents[i]->getLightComponentsCount();
	}

	return iCount;
}

void SComponent::addLightComponentsToVector(std::vector<SLightComponent*>& vLights)
{
	if (componentType == SComponentType::SCT_LIGHT)
	{
		SLightComponent* pThisComponent = dynamic_cast<SLightComponent*>(this);
		SVector vWorldPos = getLocationInWorld();

		pThisComponent->lightProps.vPosition = { vWorldPos.getX(), vWorldPos.getY(), vWorldPos.getZ() };

		vLights.push_back(pThisComponent);
	}

	for (size_t i = 0; i < vChildComponents.size(); i++)
	{
		vChildComponents[i]->addLightComponentsToVector(vLights);
	}
}

void SComponent::removeLightComponentsFromVector(std::vector<class SLightComponent*>& vLights)
{
	if (componentType == SComponentType::SCT_LIGHT)
	{
		for (size_t i = 0; i < vLights.size(); i++)
		{
			if (vLights[i] == this)
			{
				vLights.erase(vLights.begin() + i);
				break;
			}
		}
	}

	for (size_t i = 0; i < vChildComponents.size(); i++)
	{
		vChildComponents[i]->removeLightComponentsFromVector(vLights);
	}
}

void SComponent::setBindOnParentLocationRotationScaleChangedCallback(std::function<void(SComponent* pComponent)> function)
{
	onParentLocationRotationScaleChangedCallback = function;
}

SComponentType SComponent::getComponentType() const
{
	return componentType;
}

std::string SComponent::getComponentName() const
{
    return sComponentName;
}

SComponent* SComponent::getParentComponent() const
{
	return pParentComponent;
}

SComponent* SComponent::getChildComponentByName(const std::string& sComponentName) const
{
	SComponent* pComp = nullptr;

	for (size_t i = 0; i < vChildComponents.size(); i++)
	{
		pComp = vChildComponents[i]->getChildComponentByName(sComponentName);

		if (pComp != nullptr)
		{
			break;
		}
	}

	return pComp;
}

SContainer* SComponent::getContainer() const
{
	return pContainer;
}

std::vector<SComponent*> SComponent::getChildComponents() const
{
	return vChildComponents;
}

SVector SComponent::getLocationInWorld()
{
	if (!pContainer)
	{
		SError::showErrorMessageBoxAndLog("add the component to a container or other component first.");
		return SVector(0.0f, 0.0f, 0.0f);
	}

	DirectX::XMVECTOR scale, rotation, location;

	DirectX::XMMatrixDecompose(&scale, &rotation, &location, getWorldMatrix());

	DirectX::XMFLOAT3 vLocation;
	DirectX::XMStoreFloat3(&vLocation, location);

	SVector vLocationInWorld(vLocation.x, vLocation.y, vLocation.z);

	return vLocationInWorld;
}

SVector SComponent::getLocalLocation() const
{
	return vLocation;
}

SVector SComponent::getLocalScale() const
{
    return vScale;
}

SVector SComponent::getLocalRotation() const
{
	return vRotation;
}

void SComponent::getComponentLocalAxis(SVector* pvXAxisVector, SVector* pvYAxisVector, SVector* pvZAxisVector) const
{
	if (pvXAxisVector)
	{
		*pvXAxisVector = vLocalXAxisVector;
	}

	if (pvYAxisVector)
	{
		*pvYAxisVector = vLocalYAxisVector;
	}

	if (pvZAxisVector)
	{
		*pvZAxisVector = vLocalZAxisVector;
	}
}


void SComponent::setSpawnedInLevel(bool bSpawned)
{
	bSpawnedInLevel = bSpawned;

	for (size_t i = 0; i < vChildComponents.size(); i++)
	{
		vChildComponents[i]->setSpawnedInLevel(bSpawned);
	}
}

void SComponent::setUpdateCBForEveryMeshComponent()
{
	if (componentType == SComponentType::SCT_MESH)
	{
		SMeshComponent* pMeshComponent = dynamic_cast<SMeshComponent*>(this);
		pMeshComponent->renderData.iUpdateCBInFrameResourceCount = SFRAME_RES_COUNT;
	}
	else if (componentType == SComponentType::SCT_RUNTIME_MESH)
	{
		SRuntimeMeshComponent* pRuntimeMeshComponent = dynamic_cast<SRuntimeMeshComponent*>(this);
		pRuntimeMeshComponent->renderData.iUpdateCBInFrameResourceCount = SFRAME_RES_COUNT;
	}

	for (size_t i = 0; i < vChildComponents.size(); i++)
	{
		vChildComponents[i]->setUpdateCBForEveryMeshComponent();
	}
}

void SComponent::setCBIndexForMeshComponents(size_t* iIndex, bool bCreateBuffers)
{
	if (componentType == SComponentType::SCT_MESH || componentType == SComponentType::SCT_RUNTIME_MESH)
	{
		if (componentType == SComponentType::SCT_MESH)
		{
			SMeshComponent* pMeshComponent = dynamic_cast<SMeshComponent*>(this);

			pMeshComponent->mtxComponentProps.lock();

			if (bCreateBuffers && pMeshComponent->getMeshData()->getIndicesCount() > 0)
			{
				pMeshComponent->mtxComponentProps.unlock();

				pMeshComponent->createGeometryBuffers(true);
			}
			else
			{
				pMeshComponent->mtxComponentProps.unlock();
			}

			pMeshComponent->renderData.iObjCBIndex = *iIndex;
			(*iIndex)++;
		}
		else
		{
			SRuntimeMeshComponent* pRuntimeMeshComponent = dynamic_cast<SRuntimeMeshComponent*>(this);

			if (bCreateBuffers)
			{
				pRuntimeMeshComponent->mtxComponentProps.lock();

				if (pRuntimeMeshComponent->getMeshData()->getVerticesCount() > 0)
				{
					pRuntimeMeshComponent->mtxComponentProps.unlock();
					pRuntimeMeshComponent->createIndexBuffer();
				}
				else
				{
					pRuntimeMeshComponent->mtxComponentProps.unlock();
				}
			}
			pRuntimeMeshComponent->renderData.iObjCBIndex = *iIndex;
			(*iIndex)++;
		}
	}

	for (size_t i = 0; i < vChildComponents.size(); i++)
	{
		vChildComponents[i]->setCBIndexForMeshComponents(iIndex, bCreateBuffers);
	}
}

void SComponent::setContainer(SContainer* pContainer)
{
	this->pContainer = pContainer;

	for (size_t i = 0; i < vChildComponents.size(); i++)
	{
		vChildComponents[i]->setContainer(pContainer);
	}
}
