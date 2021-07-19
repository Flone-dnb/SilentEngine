// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "SContainer.h"

// Custom
#include "SilentEngine/Public/SApplication/SApplication.h"
#include "SilentEngine/Private/EntityComponentSystem/SComponent/SComponent.h"
#include "SilentEngine/Public/EntityComponentSystem/SMeshComponent/SMeshComponent.h"

SContainer::SContainer(const std::string& sContainerName)
{
	bVisible              = true;
	bSpawnedInLevel       = false;
	bIsDynamicObjectUsedInIntersectionTests = false;

	this->sContainerName  = sContainerName;

	iMeshComponentsCount  = 0;
	iStartIndexCB         = 0;

	vLocation             = SVector(0.0f, 0.0f, 0.0f);
	vRotation             = SVector(0.0f, 0.0f, 0.0f);
	vScale                = SVector(1.0f, 1.0f, 1.0f);

	vLocalXAxisVector     = SVector(1.0f, 0.0f, 0.0f);
	vLocalYAxisVector     = SVector(0.0f, 1.0f, 0.0f);
	vLocalZAxisVector     = SVector(0.0f, 0.0f, 1.0f);
}

SContainer::~SContainer()
{
	SApplication* pApp = SApplication::getApp();

	if (pApp && bSpawnedInLevel)
	{
		pApp->getCurrentLevel()->despawnContainerFromLevel(this);
	}

	for (size_t i = 0; i < vComponents.size(); i++)
	{
		// Here we delete SComponent*:
		// First will be called child's destructor and then SComponent's destructor thanks to virtual destructors.

		// If you got exception here then you have called delete pComponent in your Container's destructor.
		delete vComponents[i];
	}
}

void SContainer::setLocation(const SVector& vNewLocation, bool bLocationInWorldCoordinateSystem)
{
	if (bIsDynamicObjectUsedInIntersectionTests)
	{
		SError::showErrorMessageBoxAndLog("containers of the dynamic objects should not be moved/rotated/scaled.");
	}

	if (bLocationInWorldCoordinateSystem)
	{
		vLocation = vNewLocation;
	}
	else
	{
		vLocation = vLocalXAxisVector * vNewLocation + vLocalYAxisVector * vNewLocation + vLocalZAxisVector  * vNewLocation;
	}

	for (size_t i = 0; i < vComponents.size(); i++)
	{
		vComponents[i]->updateMyAndChildsLocationRotationScale(false);
	}
}

void SContainer::setRotation(const SVector& vNewRotation)
{
	if (bIsDynamicObjectUsedInIntersectionTests)
	{
		SError::showErrorMessageBoxAndLog("containers of the dynamic objects should not be moved/rotated/scaled.");
	}

	if (vNewRotation.getX() >= 360.0f)
	{
		vRotation.setX( 360.0f - vNewRotation.getX() );
	}
	else
	{
		vRotation.setX( vNewRotation.getX() );
	}

	if (vNewRotation.getY() >= 360.0f)
	{
		vRotation.setY( 360.0f - vNewRotation.getY() );
	}
	else
	{
		vRotation.setY( vNewRotation.getY() );
	}

	if (vNewRotation.getZ() >= 360.0f)
	{
		vRotation.setZ( 360.0f - vNewRotation.getZ() );
	}
	else
	{
		vRotation.setZ( vNewRotation.getZ() );
	}

	DirectX::XMVECTOR rot = DirectX::XMVectorSet(DirectX::XMConvertToRadians(vRotation.getX()), DirectX::XMConvertToRadians(vRotation.getY()),
		DirectX::XMConvertToRadians(vRotation.getZ()), 0.0f);

	DirectX::XMFLOAT3 rotationFloat;
	DirectX::XMStoreFloat3(&rotationFloat, rot);

	DirectX::XMMATRIX rotation = DirectX::XMMatrixIdentity() *  DirectX::XMMatrixRotationX(rotationFloat.x)
		* DirectX::XMMatrixRotationY(rotationFloat.y) * DirectX::XMMatrixRotationZ(rotationFloat.z);

	DirectX::XMFLOAT4X4 rotMat;
	DirectX::XMStoreFloat4x4(&rotMat, rotation);

	vLocalXAxisVector = SVector(rotMat._11, rotMat._12, rotMat._13);
	vLocalYAxisVector = SVector(rotMat._21, rotMat._22, rotMat._23);
	vLocalZAxisVector = SVector(rotMat._31, rotMat._32, rotMat._33);

	for (size_t i = 0; i < vComponents.size(); i++)
	{
		vComponents[i]->updateMyAndChildsLocationRotationScale(false);
	}
}

void SContainer::setScale(const SVector& vNewScale)
{
	if (bIsDynamicObjectUsedInIntersectionTests)
	{
		SError::showErrorMessageBoxAndLog("containers of the dynamic objects should not be moved/rotated/scaled.");
	}

	vScale = vNewScale;

	for (size_t i = 0; i < vComponents.size(); i++)
	{
		vComponents[i]->updateMyAndChildsLocationRotationScale(false);
	}
}

bool SContainer::addComponentToContainer(SComponent* pComponent)
{
	if (bSpawnedInLevel)
	{
		SError::showErrorMessageBoxAndLog("cannot add a component when the container is already spawned.");
		return true;
	}

	for (size_t i = 0; i < vComponents.size(); i++)
	{
		if (vComponents[i]->getComponentName() == pComponent->getComponentName())
		{
			SError::showErrorMessageBoxAndLog("the component's name is not unique within this container.");
			return true;
		}
	}

	if (pComponent->componentType == SComponentType::SCT_MESH || pComponent->componentType == SComponentType::SCT_RUNTIME_MESH)
	{
		iMeshComponentsCount++;
	}

	vComponents.push_back(pComponent);
	pComponent->setContainer(this);

	pComponent->setLocalLocation(SVector(0.0f, 0.0f, 0.0f));

	return false;
}

bool SContainer::removeComponentFromContainer(SComponent* pComponent)
{
	if (bSpawnedInLevel)
	{
		SError::showErrorMessageBoxAndLog("cannot remove a component when the container is already spawned.");
		return true;
	}

	for (size_t i = 0; i < vComponents.size(); i++)
	{
		if (vComponents[i] == pComponent)
		{
			if (pComponent->componentType == SComponentType::SCT_MESH || pComponent->componentType == SComponentType::SCT_RUNTIME_MESH)
			{
				iMeshComponentsCount--;
			}

			vComponents[i]->setContainer(nullptr);
			vComponents.erase( vComponents.begin() + i );

			break;
		}
	}

	return false;
}

void SContainer::setVisibility(bool bVisible)
{
	this->bVisible = bVisible;
}

bool SContainer::setContainerName(const std::string& sContainerName)
{
	if (bSpawnedInLevel == false)
	{
		this->sContainerName = sContainerName;
		return false;
	}
	else
	{
		return true;
	}
}

void SContainer::unbindMaterialsFromAllComponents()
{
	for (size_t i = 0; i < vComponents.size(); i++)
	{
		vComponents[i]->unbindMaterialsIncludingChilds();
	}
}

bool SContainer::isVisible() const
{
	return bVisible;
}

std::string SContainer::getContainerName() const
{
	return sContainerName;
}

SVector SContainer::getLocation() const
{
	return vLocation;
}

SVector SContainer::getRotation() const
{
	return vRotation;
}

SVector SContainer::getScale() const
{
	return vScale;
}

void SContainer::getLocalAxis(SVector* pvXAxisVector, SVector* pvYAxisVector, SVector* pvZAxisVector) const
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

SComponent* SContainer::getComponentByName(const std::string& sComponentName) const
{
	SComponent* pComp = nullptr;
	
	for (size_t i = 0; i < vComponents.size(); i++)
	{
		if (vComponents[i]->getComponentName() == sComponentName)
		{
			pComp = vComponents[i];
			break;
		}

		pComp = vComponents[i]->getChildComponentByName(sComponentName);
		if (pComp != nullptr)
		{
			break;
		}
	}

	return pComp;
}

std::vector<SComponent*> SContainer::getComponents() const
{
    return vComponents;
}

void SContainer::setSpawnedInLevel(bool bSpawned)
{
	bSpawnedInLevel = bSpawned;
	
	for (size_t i = 0; i < vComponents.size(); i++)
	{
		vComponents[i]->setSpawnedInLevel(bSpawned);
	}
}

void SContainer::setStartIndexInCB(size_t iStartIndex)
{
	iStartIndexCB = iStartIndex;
}

void SContainer::getAllMeshComponents(std::vector<SComponent*>* pvOpaqueComponents, std::vector<SComponent*>* pvTransparentComponents)
{
	for (size_t i = 0; i < vComponents.size(); i++)
	{
		vComponents[i]->getAllMeshComponents(pvOpaqueComponents, pvTransparentComponents);
	}
}

void SContainer::createVertexBufferForRuntimeMeshComponents(SFrameResource* pFrameResource)
{
	for (size_t i = 0; i < vComponents.size(); i++)
	{
		vComponents[i]->createVertexBufferForRuntimeMeshComponents(pFrameResource);
	}
}

size_t SContainer::getLightComponentsCount()
{
	size_t iCount = 0;

	for (size_t i = 0; i < vComponents.size(); i++)
	{
		iCount += vComponents[i]->getLightComponentsCount();
	}

	return iCount;
}

void SContainer::getRequiredDSVCountForShadowMaps(size_t& iDSVCount)
{
	for (size_t i = 0; i < vComponents.size(); i++)
	{
		vComponents[i]->getRequiredDSVCountForShadowMaps(iDSVCount);
	}
}

void SContainer::createInstancingDataForFrameResource(std::vector<std::unique_ptr<SFrameResource>>* vFrameResources)
{
	for (size_t i = 0; i < vComponents.size(); i++)
	{
		vComponents[i]->createInstancingDataForFrameResource(vFrameResources);
	}
}

void SContainer::removeVertexBufferForRuntimeMeshComponents(std::vector<std::unique_ptr<SFrameResource>>* vFrameResources, size_t& iRemovedCount)
{
	for (size_t i = 0; i < vComponents.size(); i++)
	{
		vComponents[i]->removeVertexBufferForRuntimeMeshComponents(vFrameResources, iRemovedCount);
	}
}

void SContainer::removeInstancingDataForFrameResources(std::vector<std::unique_ptr<SFrameResource>>* vFrameResources)
{
	for (size_t i = 0; i < vComponents.size(); i++)
	{
		vComponents[i]->removeInstancingDataForFrameResources(vFrameResources);
	}
}

void SContainer::deallocateShadowMapCBsForLightComponents(std::vector<std::unique_ptr<SFrameResource>>* vFrameResources)
{
	for (size_t i = 0; i < vComponents.size(); i++)
	{
		vComponents[i]->deallocateShadowMapCBsForLightComponents(vFrameResources);
	}
}

void SContainer::getMaxVertexBufferIndexForRuntimeMeshComponents(size_t& iMaxIndex)
{
	for (size_t i = 0; i < vComponents.size(); i++)
	{
		vComponents[i]->getMaxVertexBufferIndexForRuntimeMeshComponents(iMaxIndex);
	}
}

void SContainer::updateVertexBufferIndexForRuntimeMeshComponents(size_t iIfIndexMoreThatThisValue, size_t iMinusValue)
{
	for (size_t i = 0; i < vComponents.size(); i++)
	{
		vComponents[i]->updateVertexBufferIndexForRuntimeMeshComponents(iIfIndexMoreThatThisValue, iMinusValue);
	}
}

size_t SContainer::getMeshComponentsCount() const
{
	size_t iMeshCompsCount = iMeshComponentsCount;

	for (size_t i = 0; i < vComponents.size(); i++)
	{
		iMeshCompsCount += vComponents[i]->getMeshComponentsCount();
	}

	return iMeshCompsCount;
}

size_t SContainer::getStartIndexInCB() const
{
	return iStartIndexCB;
}

void SContainer::addMeshesByShader(std::vector<SShaderObjects>* pOpaqueMeshesByShader, std::vector<SShaderObjects>* pTransparentMeshesByShader) const
{
	for (size_t i = 0; i < vComponents.size(); i++)
	{
		vComponents[i]->addMeshesByShader(pOpaqueMeshesByShader, pTransparentMeshesByShader);
	}
}

void SContainer::removeMeshesByShader(std::vector<SShaderObjects>* pOpaqueMeshesByShader, std::vector<SShaderObjects>* pTransparentMeshesByShader) const
{
	for (size_t i = 0; i < vComponents.size(); i++)
	{
		vComponents[i]->removeMeshesByShader(pOpaqueMeshesByShader, pTransparentMeshesByShader);
	}
}

void SContainer::registerAll3DSoundComponents()
{
	for (size_t i = 0; i < vComponents.size(); i++)
	{
		vComponents[i]->registerAll3DSoundComponents();
	}
}

void SContainer::unregisterAll3DSoundComponents()
{
	for (size_t i = 0; i < vComponents.size(); i++)
	{
		vComponents[i]->unregisterAll3DSoundComponents();
	}
}
