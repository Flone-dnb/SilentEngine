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

SComponent::SComponent()
{
	componentType = SCT_NONE;

	sComponentName = "";

	bSpawnedInLevel = false;

	pParentComponent = nullptr;
	pContainer       = nullptr;

	iMeshComponentsCount = 0;

	vLocation             = SVector(0.0f, 0.0f, 0.0f);
	vRotation             = SVector(0.0f, 0.0f, 0.0f);
	vScale                = SVector(1.0f, 1.0f, 1.0f);

	vLocalXAxisVector     = SVector(1.0f, 0.0f, 0.0f);
	vLocalYAxisVector     = SVector(0.0f, 1.0f, 0.0f);
	vLocalZAxisVector     = SVector(0.0f, 0.0f, 1.0f);
}

SComponent::~SComponent()
{
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
		return true;
	}
	else
	{
		if (pContainer)
		{
			SComponent* pComponentWithSameName = nullptr;
			pComponentWithSameName = pContainer->getComponentByName(pComponent->getComponentName());

			if (pComponentWithSameName == nullptr)
			{
				if (pComponent->componentType == SCT_MESH || pComponent->componentType == SCT_RUNTIME_MESH)
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
				if (pComponent->componentType == SCT_MESH || pComponent->componentType == SCT_RUNTIME_MESH)
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

void SComponent::setLocation(const SVector& location)
{
	if (!pContainer)
	{
		SError::showErrorMessageBox(L"SComponent::setLocation()", L"pContainer was nullptr - add the component to a container.");
		return;
	}

	SVector vParentOriginLocation;
	SVector vParentXAxisVector, vParentYAxisVector, vParentZAxisVector;

	if (pParentComponent)
	{
		vParentOriginLocation = pParentComponent->getLocation();
		pParentComponent->getComponentLocalAxis(&vParentXAxisVector, &vParentYAxisVector, &vParentZAxisVector);
	}
	else
	{
		vParentOriginLocation = pContainer->getLocation();
		pContainer->getLocalAxis(&vParentXAxisVector, &vParentYAxisVector, &vParentZAxisVector);
	}


	SVector vLocationBefore = vLocation;

	vLocation = vParentXAxisVector * location + vParentYAxisVector * location + vParentZAxisVector * location;

	updateMyAndChildsLocationRotationScale();
}

void SComponent::setRotation(const SVector& rotation)
{
	if (!pContainer)
	{
		SError::showErrorMessageBox(L"SComponent::setRotation()", L"pContainer was nullptr - add the component to a container.");
		return;
	}

	if (rotation.getX() >= 360.0f)
	{
		vRotation.setX( 360.0f - rotation.getX() );
	}
	else
	{
		vRotation.setX( rotation.getX() );
	}

	if (rotation.getY() >= 360.0f)
	{
		vRotation.setY( 360.0f - rotation.getY() );
	}
	else
	{
		vRotation.setY( rotation.getY() );
	}

	if (rotation.getZ() >= 360.0f)
	{
		vRotation.setZ( 360.0f - rotation.getZ() );
	}
	else
	{
		vRotation.setZ( rotation.getZ() );
	}


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

	updateMyAndChildsLocationRotationScale();
}

void SComponent::setScale(const SVector& scale)
{
	if (!pContainer)
	{
		SError::showErrorMessageBox(L"SComponent::setScale()", L"pContainer was nullptr - add the component to a container.");
		return;
	}

	SVector vScaleBefore = vScale;
	vScale = scale;

	updateMyAndChildsLocationRotationScale();
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
	if (componentType == SCT_RUNTIME_MESH)
	{
		SRuntimeMeshComponent* pRuntimeMesh = dynamic_cast<SRuntimeMeshComponent*>(this);
		pRuntimeMesh->addVertexBuffer(pFrameResource);
	}

	for (size_t i = 0; i < vChildComponents.size(); i++)
	{
		vChildComponents[i]->createVertexBufferForRuntimeMeshComponents(pFrameResource);
	}
}

void SComponent::removeVertexBufferForRuntimeMeshComponents(std::vector<std::unique_ptr<SFrameResource>>* vFrameResources, size_t& iRemovedCount)
{
	if (componentType == SCT_RUNTIME_MESH)
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

void SComponent::getMaxVertexBufferIndexForRuntimeMeshComponents(size_t& iMaxIndex)
{
	if (componentType == SCT_RUNTIME_MESH)
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
	if (componentType == SCT_RUNTIME_MESH)
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

size_t SComponent::getMeshComponentsCount() const
{
	size_t iCount = iMeshComponentsCount;

	for (size_t i = 0; i < vChildComponents.size(); i++)
	{
		iCount += vChildComponents[i]->getMeshComponentsCount();
	}

	return iCount;
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
		SError::showErrorMessageBox(L"SComponent::getLocationInWorld()", L"pContainer was nullptr. First, add the component to a container.");
		return SVector(0.0f, 0.0f, 0.0f);
	}

	DirectX::XMVECTOR scale, rotation, location;

	DirectX::XMMatrixDecompose(&scale, &rotation, &location, getWorldMatrix());

	DirectX::XMFLOAT3 vLocation;
	DirectX::XMStoreFloat3(&vLocation, location);

	SVector vLocationInWorld(vLocation.x, vLocation.y, vLocation.z);

	return vLocationInWorld;
}

SVector SComponent::getLocation() const
{
	return vLocation;
}

SVector SComponent::getScale() const
{
    return vScale;
}

SVector SComponent::getRotation() const
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
	if (componentType == SCT_MESH)
	{
		SMeshComponent* pMeshComponent = dynamic_cast<SMeshComponent*>(this);
		pMeshComponent->renderData.iUpdateCBInFrameResourceCount = SFRAME_RES_COUNT;
	}
	else if (componentType == SCT_RUNTIME_MESH)
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
	if (componentType == SCT_MESH || componentType == SCT_RUNTIME_MESH)
	{
		if (componentType == SCT_MESH)
		{
			SMeshComponent* pMeshComponent = dynamic_cast<SMeshComponent*>(this);
			if (bCreateBuffers && pMeshComponent->getMeshData().getIndicesCount() > 0) pMeshComponent->createGeometryBuffers(true);
			pMeshComponent->renderData.iObjCBIndex = *iIndex;
			(*iIndex)++;
		}
		else if (componentType == SCT_RUNTIME_MESH)
		{
			SRuntimeMeshComponent* pRuntimeMeshComponent = dynamic_cast<SRuntimeMeshComponent*>(this);
			if (bCreateBuffers)
			{
				if (pRuntimeMeshComponent->meshData.getVerticesCount() > 0)
				{
					pRuntimeMeshComponent->createIndexBuffer();
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
