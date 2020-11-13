// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************


#include "SApplication.h"

// STL
#include <array>
#include <thread>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

// DirectX
#if defined(DEBUG) || defined(_DEBUG) 
#include <dxgidebug.h>
#include <InitGuid.h>
#pragma comment(lib, "dxguid.lib")
#endif

#include "SilentEngine/Private/DDSTextureLoader.h"

// Custom
#include "SilentEngine/Private/SError/SError.h"
#include "SilentEngine/Public/STimer/STimer.h"
#include "SilentEngine/Public/SPrimitiveShapeGenerator/SPrimitiveShapeGenerator.h"
#include "SilentEngine/Public/EntityComponentSystem/SContainer/SContainer.h"
#include "SilentEngine/Private/EntityComponentSystem/SComponent/SComponent.h"
#include "SilentEngine/Public/EntityComponentSystem/SMeshComponent/SMeshComponent.h"
#include "SilentEngine/Public/EntityComponentSystem/SRuntimeMeshComponent/SRuntimeMeshComponent.h"


SApplication* SApplication::pApp = nullptr;


// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------

bool SApplication::close()
{
	if (pApp)
	{
		if (pApp->bRunCalled)
		{
			DestroyWindow(pApp->getMainWindowHandle());
			return false;
		}
		else
		{
			MessageBox(0, L"An error occurred at SApplication::close(). Error: run() should be called first.", L"Error", 0);
			return true;
		}
	}
	else
	{
		MessageBox(0, L"An error occurred at SApplication::close(). Error: an application instance is not created (pApp was nullptr).", L"Error", 0);
		return true;
	}
}

void SApplication::setGlobalVisualSettings(SGlobalVisualSettings settings)
{
	renderPassVisualSettings = settings;
}

SGlobalVisualSettings SApplication::getGlobalVisualSettings()
{
	return renderPassVisualSettings;
}

SMaterial* SApplication::registerMaterial(const std::string& sMaterialName, bool& bErrorOccurred)
{
	bErrorOccurred = false;

	if (sMaterialName == "")
	{
		bErrorOccurred = true;
		return nullptr;
	}

	bool bHasUniqueName = true;

	mtxMaterial.lock();
	mtxSpawnDespawn.lock();

	for (size_t i = 0; i < vRegisteredMaterials.size(); i++)
	{
		if (vRegisteredMaterials[i]->getMaterialName() == sMaterialName)
		{
			bHasUniqueName = false;
			break;
		}
	}


	if (bHasUniqueName)
	{
		bool bExpanded = false;
		int iNewMaterialCBIndex = -1;

		for (size_t i = 0; i < vFrameResources.size(); i++)
		{
			iNewMaterialCBIndex = vFrameResources[i]->addNewMaterialCB(&bExpanded);
		}

		SMaterial* pMat = new SMaterial();
		pMat->sMaterialName = sMaterialName;
		pMat->iMatCBIndex = iNewMaterialCBIndex;
		pMat->bRegistered = true;
		pMat->iUpdateCBInFrameResourceCount = SFRAME_RES_COUNT;

		vRegisteredMaterials.push_back(pMat);

		if (bExpanded)
		{
			mtxDraw.lock();

			flushCommandQueue();

			for (size_t i = 0; i < vRegisteredMaterials.size(); i++)
			{
				vRegisteredMaterials[i]->iUpdateCBInFrameResourceCount = SFRAME_RES_COUNT;
			}

			// Recreate cbv heap.
			createCBVSRVHeap();
			createViews();

			mtxDraw.unlock();
		}

		mtxSpawnDespawn.unlock();
		mtxMaterial.unlock();

		return pMat;
	}
	else
	{
		mtxSpawnDespawn.unlock();
		mtxMaterial.unlock();

		bErrorOccurred = true;
		return nullptr;
	}
}

SMaterial* SApplication::getRegisteredMaterial(const std::string& sMaterialName)
{
	SMaterial* pMaterial = nullptr;

	mtxMaterial.lock();

	for (size_t i = 0; i < vRegisteredMaterials.size(); i++)
	{
		if (vRegisteredMaterials[i]->getMaterialName() == sMaterialName)
		{
			pMaterial = vRegisteredMaterials[i];
			break;
		}
	}

	mtxMaterial.unlock();

	return pMaterial;
}

std::vector<SMaterial*>* SApplication::getRegisteredMaterials()
{
	return &vRegisteredMaterials;
}

bool SApplication::unregisterMaterial(const std::string& sMaterialName)
{
	if (sMaterialName == sDefaultEngineMaterialName)
	{
		return true;
	}



	// Is this material registered?

	bool bRegistered = false;

	mtxMaterial.lock();

	for (size_t i = 0; i < vRegisteredMaterials.size(); i++)
	{
		if (vRegisteredMaterials[i]->getMaterialName() == sMaterialName)
		{
			bRegistered = true;
			break;
		}
	}

	mtxMaterial.unlock();

	if (bRegistered == false)
	{
		return true;
	}



	// Remove material.

	mtxMaterial.lock();
	mtxSpawnDespawn.lock();
	mtxDraw.lock();


	// Find if any spawned object is using this material.

	std::vector<SComponent*> vAllSpawnedMeshComponents = vAllRenderableSpawnedOpaqueComponents;
	vAllSpawnedMeshComponents.insert(vAllSpawnedMeshComponents.end(), vAllRenderableSpawnedTransparentComponents.begin(),
		vAllRenderableSpawnedTransparentComponents.end());

	for (size_t i = 0; i < vAllSpawnedMeshComponents.size(); i++)
	{
		if (vAllSpawnedMeshComponents[i]->componentType == SCT_MESH)
		{
			if (dynamic_cast<SMeshComponent*>(vAllSpawnedMeshComponents[i])->getMeshMaterial())
			{
				// Not the default material.
				if (dynamic_cast<SMeshComponent*>(vAllSpawnedMeshComponents[i])->getMeshMaterial()->getMaterialName() == sMaterialName)
				{
					dynamic_cast<SMeshComponent*>(vAllSpawnedMeshComponents[i])->unbindMaterial();
				}
			}
		}
		else if (vAllSpawnedMeshComponents[i]->componentType == SCT_RUNTIME_MESH)
		{
			if (dynamic_cast<SRuntimeMeshComponent*>(vAllSpawnedMeshComponents[i])->getMeshMaterial())
			{
				// Not the default material.
				if (dynamic_cast<SRuntimeMeshComponent*>(vAllSpawnedMeshComponents[i])->getMeshMaterial()->getMaterialName() == sMaterialName)
				{
					dynamic_cast<SRuntimeMeshComponent*>(vAllSpawnedMeshComponents[i])->unbindMaterial();
				}
			}
		}
	}




	bool bResized = false;

	for (size_t i = 0; i < vRegisteredMaterials.size(); i++)
	{
		if (vRegisteredMaterials[i]->getMaterialName() == sMaterialName)
		{
			for (size_t k = 0; k < vFrameResources.size(); k++)
			{
				vFrameResources[k]->removeMaterialCB(vRegisteredMaterials[i]->iMatCBIndex, &bResized);
			}

			vRegisteredMaterials[i]->bRegistered = false;

			delete vRegisteredMaterials[i];

			vRegisteredMaterials.erase(vRegisteredMaterials.begin() + i);

			break;
		}
	}

	if (bResized)
	{
		flushCommandQueue();

		for (size_t i = 0; i < vRegisteredMaterials.size(); i++)
		{
			vRegisteredMaterials[i]->iUpdateCBInFrameResourceCount = SFRAME_RES_COUNT;
		}

		// Recreate cbv heap.
		createCBVSRVHeap();
		createViews();
	}

	mtxSpawnDespawn.unlock();
	mtxMaterial.unlock();
	mtxDraw.unlock();

	return false;
}

STextureHandle SApplication::loadTextureFromDiskToGPU(std::string sTextureName, std::wstring sPathToTexture, bool& bErrorOccurred)
{
	bErrorOccurred = false;


	// See if the texture name is empty.

	if (sTextureName == "")
	{
		bErrorOccurred = true;

		return STextureHandle();
	}



	// See if the texture name is not unique.

	mtxTexture.lock();

	for (size_t i = 0; i < vLoadedTextures.size(); i++)
	{
		if (vLoadedTextures[i]->sTextureName == sTextureName)
		{
			bErrorOccurred = true;

			mtxTexture.unlock();

			return STextureHandle();
		}
	}

	mtxTexture.unlock();




	// See if the file exists.

	std::ifstream textureFile(sPathToTexture);

	if (textureFile.is_open() == false)
	{
		bErrorOccurred = true;

		return STextureHandle();
	}
	else
	{
		textureFile.close();
	}



	// See if the file format is .dds.

	// (use ISO C++17 Standard for no errors on MSVC 2019 toolset)
	if (fs::path(sPathToTexture).extension().string() != ".dds")
	{
		bErrorOccurred = true;

		return STextureHandle();
	}



	// Load texture.

	STextureInternal* pTexture = new STextureInternal();
	pTexture->sTextureName = sTextureName;
	pTexture->sPathToTexture = sPathToTexture;

	mtxTexture.lock();
	mtxDraw.lock();

	flushCommandQueue();
	resetCommandList();
	
	HRESULT hresult = DirectX::CreateDDSTextureFromFile12(pDevice.Get(), pCommandList.Get(), sPathToTexture.c_str(),
		pTexture->pResource, pTexture->pUploadHeap);

	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::loadTextureFromDiskToGPU::DirectX::CreateDDSTextureFromFile12()");

		mtxDraw.unlock();
		mtxTexture.unlock();

		bErrorOccurred = true;
		
		delete pTexture;

		return STextureHandle();
	}

	if (executeCommandList())
	{
		mtxDraw.unlock();
		mtxTexture.unlock();

		bErrorOccurred = true;

		delete pTexture;

		return STextureHandle();
	}

	if (flushCommandQueue())
	{
		mtxDraw.unlock();
		mtxTexture.unlock();

		bErrorOccurred = true;

		delete pTexture;

		return STextureHandle();
	}

	mtxDraw.unlock();



	// Check if the texture size is x4.

	if (pTexture->pResource->GetDesc().Width % 4 != 0   || pTexture->pResource->GetDesc().Height % 4 != 0
		|| pTexture->pResource->GetDesc().Width != pTexture->pResource->GetDesc().Height)
	{
		delete pTexture;

		mtxTexture.unlock();

		bErrorOccurred = true;

		return STextureHandle();
	}



	// Get Resource size.

	D3D12_RESOURCE_DESC texDesc = pTexture->pResource->GetDesc();

	D3D12_RESOURCE_ALLOCATION_INFO info = pDevice->GetResourceAllocationInfo(0, 1, &texDesc);
	pTexture->iResourceSizeInBytesOnGPU = info.SizeInBytes + info.Alignment;


	
	// Add texture to loaded textures array.

	vLoadedTextures.push_back(pTexture);



	// Add the SRV to this texture.

	mtxSpawnDespawn.lock();
	mtxDraw.lock();

	flushCommandQueue();

	// Recreate cbv heap.
	createCBVSRVHeap();
	createViews();

	mtxDraw.unlock();
	mtxSpawnDespawn.unlock();
	mtxTexture.unlock();



	// Return texture handle.

	STextureHandle texHandle;
	texHandle.sTextureName = sTextureName;
	texHandle.sPathToTexture = sPathToTexture;
	texHandle.bRegistered = true;

	texHandle.pRefToTexture = vLoadedTextures.back();


	return texHandle;
}

STextureHandle SApplication::getLoadedTexture(const std::string& sTextureName, bool& bNotFound)
{
	bNotFound = true;

	STextureHandle tex;

	for (size_t i = 0; i < vLoadedTextures.size(); i++)
	{
		if (vLoadedTextures[i]->sTextureName == sTextureName)
		{
			bNotFound = false;

			tex.bRegistered = true;
			tex.pRefToTexture  = vLoadedTextures[i];
			tex.sTextureName   = vLoadedTextures[i]->sTextureName;
			tex.sPathToTexture = vLoadedTextures[i]->sPathToTexture;

			break;
		}
	}

	return tex;
}

std::vector<STextureHandle> SApplication::getLoadedTextures()
{
	std::vector<STextureHandle> vTextures;

	mtxTexture.lock();

	for (size_t i = 0; i < vLoadedTextures.size(); i++)
	{
		STextureHandle tex;
		tex.bRegistered = true;
		tex.sTextureName = vLoadedTextures[i]->sTextureName;
		tex.sPathToTexture = vLoadedTextures[i]->sPathToTexture;
		tex.pRefToTexture = vLoadedTextures[i];

		vTextures.push_back(tex);
	}

	mtxTexture.unlock();

	return vTextures;
}

bool SApplication::unloadTextureFromGPU(STextureHandle& textureHandle)
{
	if (textureHandle.bRegistered == false)
	{
		return true;
	}


	mtxTexture.lock();
	mtxSpawnDespawn.lock();
	mtxDraw.lock();

	// Find if any spawned object is using a material with this texture.

	std::vector<SComponent*> vAllSpawnedMeshComponents = vAllRenderableSpawnedOpaqueComponents;
	vAllSpawnedMeshComponents.insert(vAllSpawnedMeshComponents.end(), vAllRenderableSpawnedTransparentComponents.begin(),
		vAllRenderableSpawnedTransparentComponents.end());

	for (size_t i = 0; i < vAllSpawnedMeshComponents.size(); i++)
	{
		if (vAllSpawnedMeshComponents[i]->componentType == SCT_MESH)
		{
			if (dynamic_cast<SMeshComponent*>(vAllSpawnedMeshComponents[i])->getMeshMaterial())
			{
				// Not the default material.
				SMaterialProperties matProps = dynamic_cast<SMeshComponent*>(vAllSpawnedMeshComponents[i])->getMeshMaterial()->getMaterialProperties();
				STextureHandle texHandle;
				if (matProps.getDiffuseTexture(&texHandle) == false)
				{
					if (texHandle.getTextureName() == textureHandle.getTextureName())
					{
						dynamic_cast<SMeshComponent*>(vAllSpawnedMeshComponents[i])->unbindMaterial();
					}
				}

				// ADD OTHER TEXTURES HERE
			}
		}
		else if (vAllSpawnedMeshComponents[i]->componentType == SCT_RUNTIME_MESH)
		{
			if (dynamic_cast<SRuntimeMeshComponent*>(vAllSpawnedMeshComponents[i])->getMeshMaterial())
			{
				// Not the default material.
				SMaterialProperties matProps = dynamic_cast<SRuntimeMeshComponent*>(vAllSpawnedMeshComponents[i])->getMeshMaterial()->getMaterialProperties();
				STextureHandle texHandle;
				if (matProps.getDiffuseTexture(&texHandle) == false)
				{
					if (texHandle.getTextureName() == textureHandle.getTextureName())
					{
						dynamic_cast<SRuntimeMeshComponent*>(vAllSpawnedMeshComponents[i])->unbindMaterial();
					}
				}

				// ADD OTHER TEXTURES HERE
			}
		}
	}

	textureHandle.bRegistered = false;

	for (size_t i = 0; i < vLoadedTextures.size(); i++)
	{
		if (vLoadedTextures[i]->sTextureName == textureHandle.getTextureName())
		{
			unsigned long iLeftRef = vLoadedTextures[i]->pResource.Reset();

			if (iLeftRef != 0)
			{
				showMessageBox(L"Error", L"SApplication::unloadTextureFromGPU() error: texture ref count is not 0.");
			}

			delete vLoadedTextures[i];
			vLoadedTextures.erase(vLoadedTextures.begin() + i);

			break;
		}
	}



	// Remove the SRV to this texture.

	flushCommandQueue();

	// Recreate cbv heap.
	createCBVSRVHeap();
	createViews();

	mtxDraw.unlock();
	mtxSpawnDespawn.unlock();
	mtxTexture.unlock();
	

	return false;
}

SLevel* SApplication::getCurrentLevel() const
{
	return pCurrentLevel;
}

bool SApplication::spawnContainerInLevel(SContainer* pContainer)
{
	mtxSpawnDespawn.lock();

	bool bHasUniqueName = true;

	for (size_t i = 0; i < vAllRenderableSpawnedContainers.size(); i++)
	{
		if (vAllRenderableSpawnedContainers[i]->getContainerName() == pContainer->getContainerName())
		{
			bHasUniqueName = false;
			break;
		}
	}

	if (bHasUniqueName)
	{
		for (size_t i = 0; i < vAllNonrenderableSpawnedContainers.size(); i++)
		{
			if (vAllNonrenderableSpawnedContainers[i]->getContainerName() == pContainer->getContainerName())
			{
				bHasUniqueName = false;
				break;
			}
		}
	}
	

	if (bHasUniqueName == false)
	{
		mtxSpawnDespawn.unlock();
		return true;
	}



	// Check light count.
	size_t iLightComponents = 0;

	for (size_t i = 0; i < pContainer->vComponents.size(); i++)
	{
		iLightComponents += pContainer->vComponents[i]->getLightComponentsCount();
	}

	if (getCurrentLevel()->vSpawnedLightComponents.size() + iLightComponents > MAX_LIGHTS)
	{
		SError::showErrorMessageBox(L"SApplication::spawnContainerInLevel()", L"exceeded MAX_LIGHTS (this container was not spawned)");
		mtxSpawnDespawn.unlock();
		return true;
	}

	// Add lights.
	for (size_t i = 0; i < pContainer->vComponents.size(); i++)
	{
		pContainer->vComponents[i]->addLightComponentsToVector(getCurrentLevel()->vSpawnedLightComponents);
	}



	// We need 1 CB for each SCT_MESH, SCT_RUNTIME_MESH component.
	size_t iCBCount = pContainer->getMeshComponentsCount();

	mtxDraw.lock();

	flushCommandQueue();

	if (iCBCount == 0)
	{
		// No renderable components inside.

		std::vector<SContainer*>* pvNotRenderableContainers = nullptr;
		pCurrentLevel->getNotRenderableContainers(pvNotRenderableContainers);

		pvNotRenderableContainers->push_back(pContainer);

		vAllNonrenderableSpawnedContainers.push_back(pContainer);
	}
	else
	{
		iActualObjectCBCount += iCBCount;

		bool bExpanded = false;
		size_t iNewObjectsCBIndex = 0;

		for (size_t i = 0; i < vFrameResources.size(); i++)
		{
			iNewObjectsCBIndex = vFrameResources[i]->addNewObjectCB(iCBCount, &bExpanded);

			pContainer->createVertexBufferForRuntimeMeshComponents(vFrameResources[i].get());
		}


		pContainer->setStartIndexInCB(iNewObjectsCBIndex);



		resetCommandList();

		for (size_t i = 0; i < pContainer->vComponents.size(); i++)
		{
			pContainer->vComponents[i]->setCBIndexForMeshComponents(&iNewObjectsCBIndex);
		}

		if (executeCommandList())
		{
			mtxSpawnDespawn.unlock();
			mtxDraw.unlock();
			return true;
		}

		if (flushCommandQueue())
		{
			mtxSpawnDespawn.unlock();
			mtxDraw.unlock();
			return true;
		}


		std::vector<SContainer*>* pvRenderableContainers = nullptr;
		pCurrentLevel->getRenderableContainers(pvRenderableContainers);

		pvRenderableContainers->push_back(pContainer);

		vAllRenderableSpawnedContainers.push_back(pContainer);


		pContainer->getAllMeshComponents(&vAllRenderableSpawnedOpaqueComponents,
			&vAllRenderableSpawnedTransparentComponents);


		if (bExpanded)
		{
			// All CBs are cleared (they are new), need to refill them.

			for (size_t i = 0; i < pvRenderableContainers->size(); i++)
			{
				for (size_t j = 0; j < pvRenderableContainers->operator[](i)->vComponents.size(); j++)
				{
					pvRenderableContainers->operator[](i)->vComponents[j]->setUpdateCBForEveryMeshComponent();
				}
			}


			// Recreate cbv heap.
			createCBVSRVHeap();
			createViews();
		}
	}

	pContainer->setSpawnedInLevel(true);

	mtxSpawnDespawn.unlock();
	mtxDraw.unlock();

	return false;
}

void SApplication::despawnContainerFromLevel(SContainer* pContainer)
{
	mtxSpawnDespawn.lock();

	// Remove lights.
	for (size_t i = 0; i < pContainer->vComponents.size(); i++)
	{
		pContainer->vComponents[i]->removeLightComponentsFromVector(getCurrentLevel()->vSpawnedLightComponents);
	}

	// We need 1 for each SCT_MESH, SCT_RUNTIME_MESH component.
	size_t iCBCount = pContainer->getMeshComponentsCount();

	mtxDraw.lock();

	flushCommandQueue();

	if (iCBCount == 0)
	{
		// No renderable components inside.
		// Just remove from vector.

		std::vector<SContainer*>* pvNotRenderableContainers = nullptr;
		pCurrentLevel->getNotRenderableContainers(pvNotRenderableContainers);
		
		for (size_t i = 0; i < pvNotRenderableContainers->size(); i++)
		{
			if (pvNotRenderableContainers->operator[](i) == pContainer)
			{
				pvNotRenderableContainers->erase( pvNotRenderableContainers->begin() + i );
				break;
			}
		}

		for (size_t i = 0; i < vAllNonrenderableSpawnedContainers.size(); i++)
		{
			if (vAllNonrenderableSpawnedContainers[i] == pContainer)
			{
				vAllNonrenderableSpawnedContainers.erase(vAllNonrenderableSpawnedContainers.begin() + i);
				break;
			}
		}
	}
	else
	{
		iActualObjectCBCount -= iCBCount;

		bool bResized = false;

		for (size_t i = 0; i < vFrameResources.size(); i++)
		{
			vFrameResources[i]->removeObjectCB(pContainer->getStartIndexInCB(), iCBCount, &bResized);
		}

		size_t iMaxVertexBufferIndex = 0;
		pContainer->getMaxVertexBufferIndexForRuntimeMeshComponents(iMaxVertexBufferIndex);

		size_t iRemovedCount = 0;
		pContainer->removeVertexBufferForRuntimeMeshComponents(&vFrameResources, iRemovedCount);

		
		std::vector<SContainer*>* pvRenderableContainers = nullptr;
		pCurrentLevel->getRenderableContainers(pvRenderableContainers);

		if (iRemovedCount != 0)
		{
			for (size_t i = 0; i < pvRenderableContainers->size(); i++)
			{
				pvRenderableContainers->operator[](i)->updateVertexBufferIndexForRuntimeMeshComponents(iMaxVertexBufferIndex, iRemovedCount);
			}
		}

		for (size_t i = 0; i < pvRenderableContainers->size(); i++)
		{
			if (pvRenderableContainers->operator[](i) == pContainer)
			{
				pvRenderableContainers->erase(pvRenderableContainers->begin() + i);
				break;
			}
		}

		for (size_t i = 0; i < vAllRenderableSpawnedContainers.size(); i++)
		{
			if (vAllRenderableSpawnedContainers[i] == pContainer)
			{
				vAllRenderableSpawnedContainers.erase(vAllRenderableSpawnedContainers.begin() + i);
				break;
			}
		}



		size_t iStartIndex = pContainer->getStartIndexInCB();

		for (size_t i = 0; i < pvRenderableContainers->size(); i++)
		{
			if (pvRenderableContainers->operator[](i)->getStartIndexInCB() >= iStartIndex)
			{
				pvRenderableContainers->operator[](i)->setStartIndexInCB(iStartIndex);

				for (size_t j = 0; j < pvRenderableContainers->operator[](i)->vComponents.size(); j++)
				{
					pvRenderableContainers->operator[](i)->vComponents[j]->setCBIndexForMeshComponents(&iStartIndex, false);
				}

				iStartIndex = pvRenderableContainers->operator[](i)->getStartIndexInCB() + pvRenderableContainers->operator[](i)->getMeshComponentsCount();
			}
		}

		pContainer->setStartIndexInCB(0);


		removeComponentsFromGlobalVectors(pContainer);



		if (bResized)
		{
			// All CBs are cleared (they are new), need to refill them.

			for (size_t i = 0; i < pvRenderableContainers->size(); i++)
			{
				for (size_t j = 0; j < pvRenderableContainers->operator[](i)->vComponents.size(); j++)
				{
					pvRenderableContainers->operator[](i)->vComponents[j]->setUpdateCBForEveryMeshComponent();
				}
			}

			// Recreate cbv heap.
			createCBVSRVHeap();
			createViews();
		}
	}

	pContainer->setSpawnedInLevel(false);

	if (bExitCalled)
	{
		delete pContainer;
	}

	mtxSpawnDespawn.unlock();
	mtxDraw.unlock();
}

bool SApplication::setInitPreferredDisplayAdapter(std::wstring sPreferredDisplayAdapter)
{
	if (bInitCalled == false)
	{
		this->sPreferredDisplayAdapter = sPreferredDisplayAdapter;
		return false;
	}
	else
	{
		MessageBox(0, L"An error occurred at SApplication::setInitPreferredDisplayAdapter(). Error: this function should be called before init() call.", L"Error", 0);
		return true;
	}
}

bool SApplication::setInitPreferredOutputAdapter(std::wstring sPreferredOutputAdapter)
{
	if (bInitCalled == false)
	{
		this->sPreferredOutputAdapter = sPreferredOutputAdapter;
		return false;
	}
	else
	{
		MessageBox(0, L"An error occurred at SApplication::setInitPreferredOutputAdapter(). Error: this function should be called before init() call.", L"Error", 0);
		return true;
	}
}

bool SApplication::setInitFullscreen(bool bFullscreen)
{
	if (bInitCalled == false)
	{
		this->bFullscreen = bFullscreen;
		return false;
	}
	else
	{
		MessageBox(0, L"An error occurred at SApplication::setInitFullscreen(). Error: this function should be called before init() call.", L"Error", 0);
		return true;
	}
}

bool SApplication::setInitEnableVSync(bool bEnable)
{
	if (bInitCalled == false)
	{
		bVSyncEnabled = bEnable;
		return false;
	}
	else
	{
		MessageBox(0, L"An error occurred at SApplication::setInitEnableVSync(). Error: this function should be called before init() call.", L"Error", 0);
		return true;
	}
}

void SApplication::setBackBufferFillColor(SVector vColor)
{
	backBufferFillColor[0] = vColor.getX();
	backBufferFillColor[1] = vColor.getY();
	backBufferFillColor[2] = vColor.getZ();
}

void SApplication::setEnableWireframeMode(bool bEnable)
{
	mtxDraw.lock();

	// Don't change this value is draw() in progress.
	bUseFillModeWireframe = bEnable;

	mtxDraw.unlock();
}

void SApplication::setMSAAEnabled(bool bEnable)
{
	if (MSAA_Enabled != bEnable)
	{
		MSAA_Enabled = bEnable;

		if (bInitCalled)
		{
			mtxDraw.lock();

			createPSO();
			onResize();

			mtxDraw.unlock();
		}
	}
}

bool SApplication::setMSAASampleCount(MSAASampleCount eSampleCount)
{
	if (pDevice)
	{
		if (MSAA_SampleCount != eSampleCount)
		{
			MSAA_SampleCount = eSampleCount;

			if (checkMSAASupport())
			{
				return true;
			}

			if (MSAA_Enabled && bInitCalled)
			{
				mtxDraw.lock();

				createPSO();

				onResize();

				mtxDraw.unlock();
			}
		}

		return false;
	}
	else
	{
		return true;
	}
}

bool SApplication::isMSAAEnabled() const
{
	return MSAA_Enabled;
}

MSAASampleCount SApplication::getMSAASampleCount() const
{
	MSAASampleCount sampleCount;

	switch (MSAA_SampleCount)
	{
	case(2):
		sampleCount = MSAASampleCount::SC_2;
		break;
	case(4):
		sampleCount = MSAASampleCount::SC_4;
		break;
	}

	return sampleCount;
}

bool SApplication::setFullscreenWithCurrentResolution(bool bFullscreen)
{
	if (bInitCalled)
	{
		if (this->bFullscreen != bFullscreen)
		{
			mtxDraw.lock();

			this->bFullscreen = bFullscreen;

			HRESULT hresult;

			if (bFullscreen)
			{
				hresult = pSwapChain->SetFullscreenState(bFullscreen, pOutput.Get());
			}
			else
			{
				// From docs: "pTarget - If you pass FALSE to Fullscreen, you must set this parameter to NULL."
				hresult = pSwapChain->SetFullscreenState(bFullscreen, NULL);
			}

			if (FAILED(hresult))
			{
				SError::showErrorMessageBox(hresult, L"SApplication::setFullscreen::IDXGISwapChain::SetFullscreenState()");
				mtxDraw.unlock();
				return true;
			}
			else
			{
				// Resize the buffers.
				onResize();
			}

			mtxDraw.unlock();
		}

		return false;
	}
	else
	{
		MessageBox(0, L"An error occurred at SApplication::setFullscreen(). Error: init() should be called first.", L"Error", 0);
		return true;
	}
}

bool SApplication::setScreenResolution(SScreenResolution screenResolution)
{
	if (bInitCalled)
	{
		if (iMainWindowWidth != screenResolution.iWidth || iMainWindowHeight != screenResolution.iHeight)
		{
			if ((bWindowMaximized || bWindowMinimized) && bFullscreen == false)
			{
				restoreWindow();
			}

			iMainWindowWidth  = screenResolution.iWidth;
			iMainWindowHeight = screenResolution.iHeight;

			bCustomWindowSize = true;

			getScreenParams(true);

			DXGI_MODE_DESC desc;
			desc.Format = BackBufferFormat;
			desc.Width  = iMainWindowWidth;
			desc.Height = iMainWindowHeight;
			desc.RefreshRate.Numerator   = iRefreshRateNumerator;
			desc.RefreshRate.Denominator = iRefreshRateDenominator;
			desc.Scaling = iScaling;
			desc.ScanlineOrdering = iScanlineOrder;

			
			mtxDraw.lock();

			flushCommandQueue();

			pSwapChain->ResizeTarget(&desc);

			// Resize the buffers.
			onResize();

			mtxDraw.unlock();
		}
		
		return false;
	}
	else
	{
		MessageBox(0, L"An error occurred at SApplication::setScreenResolution(). Error: init() should be called first.", L"Error", 0);
		return true;
	}
}

bool SApplication::setCameraFOV(float fFOVInDeg)
{
	if (fFOVInDeg > 200 || fFOVInDeg < 1)
	{
		MessageBox(0, L"An error occurred at SApplication::setFOV(). Error: the FOV value should be in the range [1; 200].", L"Error", 0);
		return true;
	}
	else
	{
		this->fFOVInDeg = fFOVInDeg;

		if (bInitCalled)
		{
			mtxDraw.lock();

			flushCommandQueue();

			// Apply FOV.
			onResize();

			mtxDraw.unlock();
		}
		
		return false;
	}
}

bool SApplication::setNearClipPlane(float fNearClipPlaneValue)
{
	if ( (fNearClipPlaneValue < 0.0f) || (bInitCalled == false) )
	{
		MessageBox(0, L"An error occurred at SApplication::setNearClipPlane(). Error: the fNearClipPlaneValue value should be"
			" more than 0 and the init() function should be called first.", L"Error", 0);
		return true;
	}
	else
	{
		this->fNearClipPlaneValue = fNearClipPlaneValue;

		if (bInitCalled)
		{
			mtxDraw.lock();

			onResize();

			mtxDraw.unlock();
		}

		return false;
	}
}

bool SApplication::setFarClipPlane(float fFarClipPlaneValue)
{
	if ( (fFarClipPlaneValue < 0.0f) || (bInitCalled == false) )
	{
		MessageBox(0, L"An error occurred at SApplication::setFarClipPlane(). Error: the fFarClipPlaneValue value should be"
			" more than 0 and the init() function should be called first.", L"Error", 0);
		return true;
	}
	else
	{
		this->fFarClipPlaneValue = fFarClipPlaneValue;

		if (bInitCalled)
		{
			mtxDraw.lock();

			onResize();

			mtxDraw.unlock();
		}

		return false;
	}
}

void SApplication::setFixedCameraRotationShift(float fHorizontalShift, float fVerticalShift)
{
	// Make each pixel correspond to a quarter of a degree.
	float dx = DirectX::XMConvertToRadians(0.25f * fHorizontalShift);
	float dy = DirectX::XMConvertToRadians(0.25f * fVerticalShift);

	// Update angles based on input to orbit camera around box.
	fTheta += dx;
	fPhi += -dy;

	// Restrict the angle mPhi.
	fPhi = SMath::clamp(fPhi, 0.1f, SMath::fPi - 0.1f);
}

void SApplication::setFixedCameraZoom(float fZoom)
{
	if (fZoom > 0.0f)
	{
		fRadius = fZoom;
	}
}

void SApplication::setFixedCameraRotation(float fPhi, float fTheta)
{
	this->fPhi = fPhi;
	this->fTheta = fTheta;
}

void SApplication::setCallTick(bool bTickCanBeCalled)
{
	bCallTick = bTickCanBeCalled;
}

void SApplication::setShowMouseCursor(bool bShow)
{
	if (bShow)
	{
		if (bMouseCursorShown == false)
		{
			ShowCursor(true);
			bMouseCursorShown = true;
		}
	}
	else
	{
		if (bMouseCursorShown)
		{
			ShowCursor(false);
			bMouseCursorShown = false;
		}
	}
}

bool SApplication::setCursorPos(const SVector& vPos)
{
	if (bInitCalled)
	{
		if (bMouseCursorShown)
		{
			POINT pos;
			pos.x = vPos.getX();
			pos.y = vPos.getY();

			if (ClientToScreen(hMainWindow, &pos) == 0)
			{
				SError::showErrorMessageBox(L"SApplication::setCursorPos::ClientToScreen()", std::to_wstring(GetLastError()));
				return true;
			}

			if (SetCursorPos(pos.x, pos.y) == 0)
			{
				SError::showErrorMessageBox(L"SApplication::setCursorPos::SetCursorPos()", std::to_wstring(GetLastError()));
				return true;
			}

			return false;
		}
		else
		{
			SError::showErrorMessageBox(L"SApplication::setCursorPos()", L"the cursor is hidden.");
			return true;
		}
	}
	else
	{
		SError::showErrorMessageBox(L"SApplication::setCursorPos()", L"init() should be called first.");
		return true;
	}
}

void SApplication::setFPSLimit(float fFPSLimit)
{
	if (fFPSLimit <= 0.1f)
	{
		this->fFPSLimit = 0.0f;
		dDelayBetweenFramesInMS = 0.0;
	}
	else
	{
		this->fFPSLimit = fFPSLimit;
		dDelayBetweenFramesInMS = 1000.0 / fFPSLimit;
	}
}


void SApplication::setShowFrameStatsInWindowTitle(bool bShow)
{
	bShowFrameStatsInTitle = bShow;
}

void SApplication::setWindowTitleText(const std::wstring& sTitleText)
{
	sMainWindowTitle = sTitleText;

	if (bInitCalled)
	{
		if (bShowFrameStatsInTitle == false)
		{
			SetWindowText(hMainWindow, sTitleText.c_str());
		}
	}
}

SApplication* SApplication::getApp()
{
	return pApp;
}

bool SApplication::getCursorPos(SVector* vPos)
{
	if (bInitCalled)
	{
		if (bMouseCursorShown)
		{
			POINT pos;

			if (GetCursorPos(&pos) == 0)
			{
				SError::showErrorMessageBox(L"SApplication::getCursorPos::GetCursorPos()", std::to_wstring(GetLastError()));
				return true;
			}

			if (ScreenToClient(hMainWindow, &pos) == 0)
			{
				SError::showErrorMessageBox(L"SApplication::getCursorPos::ScreenToClient()", std::to_wstring(GetLastError()));
				return true;
			}

			vPos->setX(pos.x);
			vPos->setY(pos.y);

			return false;
		}
		else
		{
			SError::showErrorMessageBox(L"SApplication::getCursorPos()", L"the cursor is hidden.");
			return true;
		}
	}
	else
	{
		SError::showErrorMessageBox(L"SApplication::getCursorPos()", L"init() shound be called first.");
		return true;
	}
}

bool SApplication::getWindowSize(SVector* vSize)
{
	if (bInitCalled)
	{
		vSize->setX(iMainWindowWidth);
		vSize->setY(iMainWindowHeight);

		return false;
	}
	else
	{
		SError::showErrorMessageBox(L"SApplication::getWindowSize()", L"init() should be called first.");
		return true;
	}
}

SVideoSettings* SApplication::getVideoSettings() const
{
	return pVideoSettings;
}

SProfiler* SApplication::getProfiler() const
{
	return pProfiler;
}

void SApplication::showMessageBox(const std::wstring& sMessageBoxTitle, const std::wstring & sMessageText) const
{
	MessageBox(0, sMessageText.c_str(), sMessageBoxTitle.c_str(), 0);
}

std::vector<std::wstring> SApplication::getSupportedDisplayAdapters() const
{
	std::vector<std::wstring> vSupportedAdapters;

	if (pFactory)
	{
		for(UINT adapterIndex = 0; ; ++adapterIndex)
		{
			IDXGIAdapter3* pAdapter1 = nullptr;
			if (pFactory->EnumAdapters(adapterIndex, reinterpret_cast<IDXGIAdapter**>(&pAdapter1)) == DXGI_ERROR_NOT_FOUND)
			{
				// No more adapters to enumerate.
				break;
			} 



			// Check to see if the adapter supports Direct3D version, but don't create the actual device yet.

			if (SUCCEEDED(D3D12CreateDevice(pAdapter1, ENGINE_D3D_FEATURE_LEVEL, _uuidof(ID3D12Device), nullptr)))
			{
				DXGI_ADAPTER_DESC desc;
				pAdapter1->GetDesc(&desc);

				vSupportedAdapters.push_back(desc.Description);

				pAdapter1->Release();
				pAdapter1 = nullptr;
			}
		}
	}
	else
	{
		vSupportedAdapters.push_back(L"Error. DXGIFactory was not created. Call init() first.");
	}

	return vSupportedAdapters;
}

std::wstring SApplication::getCurrentDisplayAdapter() const
{
	if (bInitCalled)
	{
		if (bUsingWARPAdapter)
		{
			return L"WARP software adapter.";
		}
		else
		{
			DXGI_ADAPTER_DESC desc;
			pAdapter.Get()->GetDesc(&desc);

			std::wstring sCurrentAdapter = desc.Description;

			return sCurrentAdapter;
		}
	}
	else
	{
		return L"init() should be called first.";
	}
}

bool SApplication::getVideoMemorySizeInBytesOfCurrentDisplayAdapter(SIZE_T* pSizeInBytes) const
{
	if (bInitCalled)
	{
		DXGI_ADAPTER_DESC desc;
		pAdapter.Get()->GetDesc(&desc);

		*pSizeInBytes = desc.DedicatedVideoMemory;

		return false;
	}
	else
	{
		return true;
	}
}

bool SApplication::getVideoMemoryUsageInBytesOfCurrentDisplayAdapter(UINT64* pSizeInBytes) const
{
	if (bInitCalled)
	{
		DXGI_QUERY_VIDEO_MEMORY_INFO videoMemoryInfo;
		pAdapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &videoMemoryInfo);

		*pSizeInBytes = videoMemoryInfo.CurrentUsage;

		return false;
	}
	else
	{
		return true;
	}
}

std::vector<std::wstring> SApplication::getOutputDisplaysOfCurrentDisplayAdapter() const
{
	std::vector<std::wstring> vOutputAdapters;

	if (pFactory)
	{
		if (pAdapter)
		{
			for(UINT outputIndex = 0; ; ++outputIndex)
			{
				IDXGIOutput* pOutput1 = nullptr;
				if (pAdapter->EnumOutputs(outputIndex, &pOutput1) == DXGI_ERROR_NOT_FOUND)
				{
					// No more displays to enumerate.
					break;
				} 

				DXGI_OUTPUT_DESC desc;
				pOutput1->GetDesc(&desc);

				vOutputAdapters.push_back(desc.DeviceName);

				pOutput1->Release();
				pOutput1 = nullptr;
			}
		}
		else
		{
			vOutputAdapters.push_back(L"Error. DXGIAdapter was not created.");
		}
	}
	else
	{
		vOutputAdapters.push_back(L"Error. DXGIFactory was not created. Call init() first.");
	}

	return vOutputAdapters;
}

bool SApplication::getAvailableScreenResolutionsOfCurrentOutputDisplay(std::vector<SScreenResolution>* vResolutions) const
{
	if (bInitCalled)
	{
		UINT numModes = 0;

		HRESULT hresult = pOutput->GetDisplayModeList(BackBufferFormat, 0, &numModes, NULL);
		if (FAILED(hresult))
		{
			SError::showErrorMessageBox(hresult, L"SApplication::getSupportedScreenResolutionsOfCurrentOutputDisplay::IDXGIOutput::GetDisplayModeList() (count)");
			return true;
		}

		std::vector<DXGI_MODE_DESC> vDisplayModes(numModes);

		hresult = pOutput->GetDisplayModeList(BackBufferFormat, 0, &numModes, &vDisplayModes[0]);
		if (FAILED(hresult))
		{
			SError::showErrorMessageBox(hresult, L"SApplication::getSupportedScreenResolutionsOfCurrentOutputDisplay::IDXGIOutput::GetDisplayModeList() (list)");
			return true;
		}


		
		// Get result.

		for (size_t i = 0; i < vDisplayModes.size(); i++)
		{
			if ((vDisplayModes[i].ScanlineOrdering == iScanlineOrder)
				&&
				(vDisplayModes[i].Scaling == iScaling))
			{
				SScreenResolution sr;
				sr.iWidth  = vDisplayModes[i].Width;
				sr.iHeight = vDisplayModes[i].Height;

				vResolutions->push_back(sr);
			}
		}

		return false;
	}
	else
	{
		return true;
	}
}

std::wstring SApplication::getCurrentOutputDisplay() const
{
	if (bInitCalled)
	{
		DXGI_OUTPUT_DESC desc;
		pOutput.Get()->GetDesc(&desc);

		std::wstring sCurrentAdapter = desc.DeviceName;

		return sCurrentAdapter;
	}
	else
	{
		return L"init() should be called first.";
	}
}

float SApplication::getCurrentOutputDisplayRefreshRate() const
{
	if (bInitCalled)
	{
		DXGI_SWAP_CHAIN_FULLSCREEN_DESC desc;

		HRESULT hresult = pSwapChain->GetFullscreenDesc(&desc);
		if (FAILED(hresult))
		{
			SError::showErrorMessageBox(hresult, L"SApplication::getCurrentScreenResolution::IDXGISwapChain1::GetFullscreenDesc()");
			return 0.0f;
		}
		else
		{
			return desc.RefreshRate.Numerator / static_cast<float>(desc.RefreshRate.Denominator);
		}
	}
	else
	{
		SError::showErrorMessageBox(L"SApplication::getCurrentOutputDisplayRefreshRate()", L"init() should be called first.");
		return 0.0f;
	}
}

bool SApplication::getCurrentScreenResolution(SScreenResolution* pScreenResolution) const
{
	if (bInitCalled)
	{
		DXGI_SWAP_CHAIN_DESC1 desc;

		HRESULT hresult = pSwapChain->GetDesc1(&desc);
		if (FAILED(hresult))
		{
			SError::showErrorMessageBox(hresult, L"SApplication::getCurrentScreenResolution::IDXGISwapChain1::GetDesc1()");
			return true;
		}
		else
		{
			pScreenResolution->iWidth  = desc.Width;
			pScreenResolution->iHeight = desc.Height;

			return false;
		}
	}
	else
	{
		MessageBox(0, L"An error occurred at SApplication::getCurrentScreenResolution(). Error: init() should be called first.", L"Error", 0);
		return true;
	}
}

bool SApplication::isFullscreen() const
{
	return bFullscreen;
}

float SApplication::getNearClipPlaneValue() const
{
	return fNearClipPlaneValue;
}

float SApplication::getFarClipPlaneValue() const
{
	return fFarClipPlaneValue;
}

SVector SApplication::getCameraLocation() const
{
	return SVector(vCameraPos.x, vCameraPos.y, vCameraPos.z);
}

void SApplication::getFixedCameraLocalAxisVector(SVector* pvXAxis, SVector* pvYAxis, SVector* pvZAxis) const
{
	if (pvXAxis)
	{
		pvXAxis->setX(vView._11);
		pvXAxis->setY(vView._12);
		pvXAxis->setZ(vView._13);
	}

	if (pvYAxis)
	{
		pvYAxis->setX(vView._21);
		pvYAxis->setY(vView._22);
		pvYAxis->setZ(vView._23);
	}

	if (pvZAxis)
	{
		pvZAxis->setX(vView._31);
		pvZAxis->setY(vView._32);
		pvZAxis->setZ(vView._33);
	}
}

void SApplication::getFixedCameraRotation(float* fPhi, float* fTheta) const
{
	*fPhi = this->fPhi;
	*fTheta = this->fTheta;
}

float SApplication::getFixedCameraZoom() const
{
	return fRadius;
}

SVector SApplication::getBackBufferFillColor() const
{
	return SVector(backBufferFillColor[0], backBufferFillColor[1], backBufferFillColor[2]);
}

bool SApplication::isWireframeModeEnabled() const
{
	return bUseFillModeWireframe;
}

unsigned long long SApplication::getTriangleCountInWorld()
{
	if (pCurrentLevel)
	{
		unsigned long long iTrisCount = 0;

		mtxSpawnDespawn.lock();

		std::vector<SContainer*>* pvRenderableContainers = nullptr;
		pCurrentLevel->getRenderableContainers(pvRenderableContainers);

		for (size_t i = 0; i < pvRenderableContainers->size(); i++)
		{
			for (size_t j = 0; j < pvRenderableContainers->operator[](i)->vComponents.size(); j++)
			{
				if (pvRenderableContainers->operator[](i)->vComponents[j]->componentType == SCT_MESH)
				{
					SMeshComponent* pMesh = dynamic_cast<SMeshComponent*>(pvRenderableContainers->operator[](i)->vComponents[j]);
					iTrisCount += pMesh->getMeshData().getIndicesCount() / 3;
				}
				else if (pvRenderableContainers->operator[](i)->vComponents[j]->componentType == SCT_RUNTIME_MESH)
				{
					SRuntimeMeshComponent* pRuntimeMesh = dynamic_cast<SRuntimeMeshComponent*>(pvRenderableContainers->operator[](i)->vComponents[j]);
					iTrisCount += pRuntimeMesh->getMeshData().getIndicesCount() / 3;
				}
			}
		}

		mtxSpawnDespawn.unlock();

		return iTrisCount;
	}
	else
	{
		return 0;
	}
}

bool SApplication::getTimeElapsedFromStart(float* fTimeInSec) const
{
	if (bRunCalled)
	{
		*fTimeInSec = gameTimer.getTimeElapsedInSec();

		return false;
	}
	else
	{
		MessageBox(0, L"An error occurred at SApplication::getTimeElapsedNonPausedFromStart(). Error: run() should be called first.", L"Error", 0);
		return true;
	}
}

bool SApplication::getFPS(int* iFPS) const
{
	if (bRunCalled)
	{
		*iFPS = this->iFPS;
		return false;
	}
	else
	{
		MessageBox(0, L"An error occurred at SApplication::getFPS(). Error: run() should be called first.", L"Error", 0);
		return true;
	}
}

bool SApplication::getTimeToRenderFrame(float* fTimeInMS) const
{
	if (bRunCalled)
	{
		*fTimeInMS = fTimeToRenderFrame;
		return false;
	}
	else
	{
		MessageBox(0, L"An error occurred at SApplication::getFPS(). Error: run() should be called first.", L"Error", 0);
		return true;
	}
}

bool SApplication::getLastFrameDrawCallCount(unsigned long long* iDrawCallCount) const
{
	if (bRunCalled)
	{
		*iDrawCallCount = iLastFrameDrawCallCount;
		return false;
	}
	else
	{
		SError::showErrorMessageBox(L"SApplication::getLastFrameDrawCallCount()", L"run() should be called first.");
		return true;
	}
}

float SApplication::getScreenAspectRatio() const
{
	return static_cast<float>(iMainWindowWidth) / iMainWindowHeight;
}

HWND SApplication::getMainWindowHandle() const
{
	return hMainWindow;
}

bool SApplication::onResize()
{
	if (bInitCalled)
	{
		// Flush before changing any resources.
		if (flushCommandQueue())
		{
			return true;
		}

		HRESULT hresult = pCommandList->Reset(pCommandListAllocator.Get(), nullptr);
		if (FAILED(hresult))
		{
			SError::showErrorMessageBox(hresult, L"SApplication::onResize::ID3D12GraphicsCommandList::Reset()");
			return true;
		}




		// Release the previous resources we will be recreating.

		for (int i = 0; i < iSwapChainBufferCount; i++)
		{
			pSwapChainBuffer[i].Reset();
		}

		pMSAARenderTarget.Reset();
		pDepthStencilBuffer.Reset();




		// Resize the swap chain.

		if (bVSyncEnabled)
		{
			hresult = pSwapChain->ResizeBuffers(iSwapChainBufferCount, iMainWindowWidth, iMainWindowHeight, BackBufferFormat,
				DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
		}
		else
		{
			hresult = pSwapChain->ResizeBuffers(iSwapChainBufferCount, iMainWindowWidth, iMainWindowHeight, BackBufferFormat,
				DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);
		}

		if (FAILED(hresult))
		{
			SError::showErrorMessageBox(hresult, L"SApplication::onResize::IDXGISwapChain::ResizeBuffers()");
			return true;
		}


		iCurrentBackBuffer = 0;




		// Create RTV.

		CD3DX12_CPU_DESCRIPTOR_HANDLE RTVHeapHandle(pRTVHeap->GetCPUDescriptorHandleForHeapStart());

		for (UINT i = 0; i < iSwapChainBufferCount; i++)
		{
			hresult = pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pSwapChainBuffer[i]));
			if (FAILED(hresult))
			{
				SError::showErrorMessageBox(hresult, L"SApplication::onResize::IDXGISwapChain::GetBuffer() (i = " + std::to_wstring(i) + L")");
				return true;
			}

			pDevice->CreateRenderTargetView(pSwapChainBuffer[i].Get(), nullptr, RTVHeapHandle);

			RTVHeapHandle.Offset(1, iRTVDescriptorSize);
		}



		// Create the MSAA render target.

		D3D12_RESOURCE_DESC msaaRenderTargetDesc;
		msaaRenderTargetDesc.Dimension      = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		msaaRenderTargetDesc.Alignment      = 0;
		msaaRenderTargetDesc.Width          = iMainWindowWidth;
		msaaRenderTargetDesc.Height         = iMainWindowHeight;
		msaaRenderTargetDesc.DepthOrArraySize = 1;
		msaaRenderTargetDesc.MipLevels      = 1;
		msaaRenderTargetDesc.Format             = BackBufferFormat;
		msaaRenderTargetDesc.SampleDesc.Count   = MSAA_Enabled ? MSAA_SampleCount : 1;
		msaaRenderTargetDesc.SampleDesc.Quality = MSAA_Enabled ? (MSAA_Quality - 1) : 0;
		msaaRenderTargetDesc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		msaaRenderTargetDesc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		D3D12_CLEAR_VALUE msaaClear;
		msaaClear.Format               = BackBufferFormat;
		msaaClear.Color[0] = backBufferFillColor[0];
		msaaClear.Color[1] = backBufferFillColor[1];
		msaaClear.Color[2] = backBufferFillColor[2];
		msaaClear.Color[3] = backBufferFillColor[3];

		hresult = pDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&msaaRenderTargetDesc,
			D3D12_RESOURCE_STATE_COMMON,
			&msaaClear,
			IID_PPV_ARGS(pMSAARenderTarget.GetAddressOf()));
		if (FAILED(hresult))
		{
			SError::showErrorMessageBox(hresult, L"SApplication::onResize::ID3D12Device::CreateCommittedResource()");
			return true;
		}

		pDevice->CreateRenderTargetView(pMSAARenderTarget.Get(), nullptr, RTVHeapHandle);



		// Create the depth/stencil buffer and view.

		D3D12_RESOURCE_DESC depthStencilDesc;
		depthStencilDesc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthStencilDesc.Alignment          = 0;
		depthStencilDesc.Width              = iMainWindowWidth;
		depthStencilDesc.Height             = iMainWindowHeight;
		depthStencilDesc.DepthOrArraySize   = 1;
		depthStencilDesc.MipLevels          = 1;
		depthStencilDesc.Format             = DepthStencilFormat;
		depthStencilDesc.SampleDesc.Count   = MSAA_Enabled ? MSAA_SampleCount : 1;
		depthStencilDesc.SampleDesc.Quality = MSAA_Enabled ? (MSAA_Quality - 1) : 0;
		depthStencilDesc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		depthStencilDesc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;



		D3D12_CLEAR_VALUE optClear;
		optClear.Format               = DepthStencilFormat;
		optClear.DepthStencil.Depth   = 1.0f;
		optClear.DepthStencil.Stencil = 0;

		hresult = pDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&depthStencilDesc,
			D3D12_RESOURCE_STATE_COMMON,
			&optClear,
			IID_PPV_ARGS(pDepthStencilBuffer.GetAddressOf()));
		if (FAILED(hresult))
		{
			SError::showErrorMessageBox(hresult, L"SApplication::onResize::ID3D12Device::CreateCommittedResource()");
			return true;
		}



		// Create DSV.

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
		dsvDesc.Flags              = D3D12_DSV_FLAG_NONE;
		dsvDesc.ViewDimension      = MSAA_Enabled ? D3D12_DSV_DIMENSION_TEXTURE2DMS : D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Format             = DepthStencilFormat;
		dsvDesc.Texture2D.MipSlice = 0;

		pDevice->CreateDepthStencilView(pDepthStencilBuffer.Get(), &dsvDesc, getDepthStencilViewHandle());



		// Transition the resource from its initial state to be used as a depth buffer.

		pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pDepthStencilBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));



		// Execute the resize commands.

		hresult = pCommandList->Close();
		if (FAILED(hresult))
		{
			SError::showErrorMessageBox(hresult, L"SApplication::onResize::ID3D12GraphicsCommandList::Close()");
			return true;
		}

		ID3D12CommandList* commandLists[] = { pCommandList.Get() };

		pCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);



		// Wait until resize is complete.

		if (flushCommandQueue())
		{
			return true;
		}



		// Update the viewport transform to cover the new window size.

		ScreenViewport.TopLeftX = 0;
		ScreenViewport.TopLeftY = 0;
		ScreenViewport.Width    = static_cast<float>(iMainWindowWidth);
		ScreenViewport.Height   = static_cast<float>(iMainWindowHeight);
		ScreenViewport.MinDepth = 0.0f;
		ScreenViewport.MaxDepth = 1.0f;

		ScissorRect = { 0, 0, iMainWindowWidth, iMainWindowHeight };



		// Update aspect ratio and recompute the projection matrix.

		DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(fFOVInDeg), getScreenAspectRatio(),
			fNearClipPlaneValue, fFarClipPlaneValue);
		XMStoreFloat4x4(&vProj, P);


		return false;
	}
	else
	{
		MessageBox(0, L"An error occurred at SApplication::onResize(). Error: init() should be called first.", L"Error", 0);

		return true;
	}
}

void SApplication::update()
{
	updateCamera();

	if (iCurrentFrameResourceIndex + 1 == iFrameResourcesCount)
	{
		iCurrentFrameResourceIndex = 0;
	}
	else
	{
		iCurrentFrameResourceIndex++;
	}

	pCurrentFrameResource = vFrameResources[iCurrentFrameResourceIndex].get();

	// Has the GPU finished processing the commands of the current frame resource?
	// If not, wait until the GPU has completed commands up to this fence point.
	if (pCurrentFrameResource->iFence != 0 && pFence->GetCompletedValue() < pCurrentFrameResource->iFence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
		HRESULT hresult = pFence->SetEventOnCompletion(pCurrentFrameResource->iFence, eventHandle);
		if (FAILED(hresult))
		{
			SError::showErrorMessageBox(hresult, L"SApplication::update::SetEventOnCompletion()");
			return;
		}
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	updateObjectCBs();
	updateMainPassCB();
}

void SApplication::updateCamera()
{
	// Convert Spherical to Cartesian coordinates.
	vCameraPos.x = fRadius * sinf(fPhi) * cosf(fTheta);
	vCameraPos.y = fRadius * sinf(fPhi) * sinf(fTheta);
	vCameraPos.z = fRadius * cosf(fPhi);

	// Build the view matrix.
	DirectX::XMVECTOR pos    = DirectX::XMVectorSet(vCameraPos.x, vCameraPos.y, vCameraPos.z, 1.0f);
	DirectX::XMVECTOR target = DirectX::XMVectorSet(vCameraTargetPos.x, vCameraTargetPos.y, vCameraTargetPos.z, 1.0f);
	DirectX::XMVECTOR up     = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

	DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(pos, target, up);
	DirectX::XMStoreFloat4x4(&vView, view);
}

void SApplication::updateObjectCBs()
{
	SUploadBuffer<SObjectConstants>* pCurrentObjectCB  = pCurrentFrameResource->pObjectsCB.get();
	SUploadBuffer<SMaterialConstants>* pCurrentMaterialCB = pCurrentFrameResource->pMaterialCB.get();

	mtxSpawnDespawn.lock();

	std::vector<SContainer*>* pvRenderableContainers = nullptr;
	pCurrentLevel->getRenderableContainers(pvRenderableContainers);

	for (size_t i = 0; i < pvRenderableContainers->size(); i++)
	{
		for (size_t j = 0; j < pvRenderableContainers->operator[](i)->vComponents.size(); j++)
		{
			updateComponentAndChilds(pvRenderableContainers->operator[](i)->vComponents[j], pCurrentObjectCB, pCurrentMaterialCB);
		}
	}

	mtxSpawnDespawn.unlock();
}

void SApplication::updateComponentAndChilds(SComponent* pComponent, SUploadBuffer<SObjectConstants>* pCurrentObjectCB,
	SUploadBuffer<SMaterialConstants>* pCurrentMaterialCB)
{
	if (pComponent->componentType == SCT_MESH)
	{
		SMeshComponent* pMeshComponent = dynamic_cast<SMeshComponent*>(pComponent);

		if (pMeshComponent->renderData.iUpdateCBInFrameResourceCount > 0)
		{
			pMeshComponent->mtxComponentProps.lock();

			DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&pMeshComponent->renderData.vWorld);
			DirectX::XMMATRIX texTransform = DirectX::XMLoadFloat4x4(&pMeshComponent->renderData.vTexTransform);

			SObjectConstants objConstants;
			DirectX::XMStoreFloat4x4(&objConstants.vWorld, DirectX::XMMatrixTranspose(world));
			DirectX::XMStoreFloat4x4(&objConstants.vTexTransform, DirectX::XMMatrixTranspose(texTransform));

			pCurrentObjectCB->copyDataToElement(pMeshComponent->renderData.iObjCBIndex, objConstants);

			// Next FrameResource need to be updated too.
			pMeshComponent->renderData.iUpdateCBInFrameResourceCount--;

			pMeshComponent->mtxComponentProps.unlock();
		}
	}
	else if (pComponent->componentType == SCT_RUNTIME_MESH)
	{
		SRuntimeMeshComponent* pRuntimeMeshComponent = dynamic_cast<SRuntimeMeshComponent*>(pComponent);


		if (pRuntimeMeshComponent->bNoMeshDataOnSpawn == false)
		{
			pRuntimeMeshComponent->mtxDrawComponent.lock();

			auto pVertexBuffer = pCurrentFrameResource->vRuntimeMeshVertexBuffers[pRuntimeMeshComponent->iIndexInFrameResourceVertexBuffer].get();

			std::vector<SVertex> vMeshShaderData = pRuntimeMeshComponent->meshData.toShaderVertex();

			for (UINT64 i = 0; i < vMeshShaderData.size(); i++)
			{
				pVertexBuffer->copyDataToElement(i, vMeshShaderData[i]);
			}

			pRuntimeMeshComponent->renderData.pGeometry->pVertexBufferGPU = pVertexBuffer->getResource();

			pRuntimeMeshComponent->mtxDrawComponent.unlock();
		}



		if (pRuntimeMeshComponent->renderData.iUpdateCBInFrameResourceCount > 0)
		{
			pRuntimeMeshComponent->mtxComponentProps.lock();

			DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&pRuntimeMeshComponent->renderData.vWorld);
			DirectX::XMMATRIX texTransform = DirectX::XMLoadFloat4x4(&pRuntimeMeshComponent->renderData.vTexTransform);

			SObjectConstants objConstants;
			DirectX::XMStoreFloat4x4(&objConstants.vWorld, DirectX::XMMatrixTranspose(world));
			DirectX::XMStoreFloat4x4(&objConstants.vTexTransform, DirectX::XMMatrixTranspose(texTransform));

			pCurrentObjectCB->copyDataToElement(pRuntimeMeshComponent->renderData.iObjCBIndex, objConstants);

			// Next FrameResource need to be updated too.
			pRuntimeMeshComponent->renderData.iUpdateCBInFrameResourceCount--;

			pRuntimeMeshComponent->mtxComponentProps.unlock();
		}
	}


	if (pComponent->componentType == SCT_MESH || pComponent->componentType == SCT_RUNTIME_MESH)
	{
		mtxMaterial.lock();

		pComponent->mtxComponentProps.lock();
		mtxUpdateMat.lock();

		SMaterial* pMaterial = pComponent->meshData.getMeshMaterial();

		if(pMaterial == nullptr)
		{
			pMaterial = vRegisteredMaterials[0];
		}

		if (pMaterial->iUpdateCBInFrameResourceCount > 0)
		{
			if (pMaterial->bLastFrameResourceIndexValid)
			{
				if (pMaterial->iFrameResourceIndexLastUpdated != iCurrentFrameResourceIndex)
				{
					updateMaterialInFrameResource(pMaterial, pCurrentMaterialCB);
				}
				// else: Already updated for this frame resource. Don't do that again.
			}
			else
			{
				updateMaterialInFrameResource(pMaterial, pCurrentMaterialCB);
			}
		}

		mtxUpdateMat.unlock();
		pComponent->mtxComponentProps.unlock();

		mtxMaterial.unlock();
	}


	std::vector<SComponent*> vChilds = pComponent->getChildComponents();

	for (size_t i = 0; i < vChilds.size(); i++)
	{
		updateComponentAndChilds(pComponent->getChildComponents()[i], pCurrentObjectCB, pCurrentMaterialCB);
	}
}

void SApplication::updateMainPassCB()
{
	DirectX::XMMATRIX view = DirectX::XMLoadFloat4x4(&vView);
	DirectX::XMMATRIX proj = DirectX::XMLoadFloat4x4(&vProj);

	DirectX::XMMATRIX viewProj    = DirectX::XMMatrixMultiply(view, proj);
	DirectX::XMMATRIX invView     = DirectX::XMMatrixInverse(&DirectX::XMMatrixDeterminant(view), view);
	DirectX::XMMATRIX invProj     = DirectX::XMMatrixInverse(&DirectX::XMMatrixDeterminant(proj), proj);
	DirectX::XMMATRIX invViewProj = DirectX::XMMatrixInverse(&DirectX::XMMatrixDeterminant(viewProj), viewProj);

	DirectX::XMStoreFloat4x4(&mainRenderPassCB.vView, XMMatrixTranspose(view));
	DirectX::XMStoreFloat4x4(&mainRenderPassCB.vInvView, XMMatrixTranspose(invView));
	DirectX::XMStoreFloat4x4(&mainRenderPassCB.vProj, XMMatrixTranspose(proj));
	DirectX::XMStoreFloat4x4(&mainRenderPassCB.vInvProj, XMMatrixTranspose(invProj));
	DirectX::XMStoreFloat4x4(&mainRenderPassCB.vViewProj, XMMatrixTranspose(viewProj));
	DirectX::XMStoreFloat4x4(&mainRenderPassCB.vInvViewProj, XMMatrixTranspose(invViewProj));
	mainRenderPassCB.vCameraPos = vCameraPos;
	mainRenderPassCB.vRenderTargetSize = DirectX::XMFLOAT2(static_cast<float>(iMainWindowWidth), static_cast<float>(iMainWindowHeight));
	mainRenderPassCB.vInvRenderTargetSize = DirectX::XMFLOAT2(1.0f / iMainWindowWidth, 1.0f / iMainWindowHeight);
	mainRenderPassCB.fNearZ = fNearClipPlaneValue;
	mainRenderPassCB.fFarZ = fFarClipPlaneValue;
	mainRenderPassCB.fTotalTime = gameTimer.getTimeElapsedInSec();
	mainRenderPassCB.fDeltaTime = gameTimer.getDeltaTimeBetweenFramesInSec();
	mainRenderPassCB.iDirectionalLightCount = 0;
	mainRenderPassCB.iPointLightCount = 0;
	mainRenderPassCB.iSpotLightCount = 0;
	mainRenderPassCB.iTextureFilterIndex = textureFilterIndex;

	mainRenderPassCB.vAmbientLightRGBA = { renderPassVisualSettings.vAmbientLightRGB.getX(),
		renderPassVisualSettings.vAmbientLightRGB.getY(),
		renderPassVisualSettings.vAmbientLightRGB.getZ(),
		1.0f};
	mainRenderPassCB.vFogColor = { renderPassVisualSettings.distantFog.vDistantFogColorRGBA.getX(),
		renderPassVisualSettings.distantFog.vDistantFogColorRGBA.getY(),
		renderPassVisualSettings.distantFog.vDistantFogColorRGBA.getZ(),
		renderPassVisualSettings.distantFog.vDistantFogColorRGBA.getW()};
	mainRenderPassCB.fFogStart = renderPassVisualSettings.distantFog.fDistantFogStart;
	mainRenderPassCB.fFogRange = renderPassVisualSettings.distantFog.fDistantFogRange;
	mainRenderPassCB.vCameraMultiplyColor = { renderPassVisualSettings.vCameraMultiplyColor.getX(),
		renderPassVisualSettings.vCameraMultiplyColor.getY(), renderPassVisualSettings.vCameraMultiplyColor.getZ() };
	mainRenderPassCB.fGamma = renderPassVisualSettings.fGamma;

	SLevel* pLevel = getCurrentLevel();

	if (pLevel)
	{
		mtxSpawnDespawn.lock();

		size_t iCurrentIndex = 0;
		size_t iTypeIndex = 0;
		std::vector<SLightComponentType> vTypes = { SLCT_DIRECTIONAL, SLCT_POINT, SLCT_SPOT };

		for (size_t k = 0; k < vTypes.size(); k++)
		{
			for (size_t i = 0; i < pLevel->vSpawnedLightComponents.size(); i++)
			{
				if (pLevel->vSpawnedLightComponents[i]->isVisible())
				{
					if (pLevel->vSpawnedLightComponents[i]->lightType == vTypes[iTypeIndex])
					{
						SVector vWorldPos = pLevel->vSpawnedLightComponents[i]->getLocationInWorld();
						pLevel->vSpawnedLightComponents[i]->lightProps.vPosition = { vWorldPos.getX(), vWorldPos.getY(), vWorldPos.getZ() };

						mainRenderPassCB.lights[iCurrentIndex] = pLevel->vSpawnedLightComponents[i]->lightProps;
						iCurrentIndex++;

						if (vTypes[iTypeIndex] == SLCT_DIRECTIONAL)
						{
							mainRenderPassCB.iDirectionalLightCount++;
						}
						else if (vTypes[iTypeIndex] == SLCT_POINT)
						{
							mainRenderPassCB.iPointLightCount++;
						}
						else
						{
							mainRenderPassCB.iSpotLightCount++;
						}
					}
				}
			}

			iTypeIndex++;
		}

		mtxSpawnDespawn.unlock();
	}


	SUploadBuffer<SRenderPassConstants>* pCurrentPassCB = pCurrentFrameResource->pRenderPassCB.get();
	pCurrentPassCB->copyDataToElement(0, mainRenderPassCB);
}

void SApplication::draw()
{
	mtxDraw.lock();

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> pCurrentCommandListAllocator = pCurrentFrameResource->pCommandListAllocator;

	// Should be only called if the GPU is not using it (i.e. command queue is empty).

	HRESULT hresult = pCurrentCommandListAllocator->Reset();
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::draw::ID3D12CommandAllocator::Reset()");
		mtxDraw.unlock();
		return;
	}



	// A command list can be reset after it has been added to the command queue via ExecuteCommandList (was added in init()).

	if (bUseFillModeWireframe)
	{
		hresult = pCommandList->Reset(pCurrentCommandListAllocator.Get(), pOpaqueWireframePSO.Get());
	}
	else
	{
		hresult = pCommandList->Reset(pCurrentCommandListAllocator.Get(), pOpaquePSO.Get());
	}

	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::draw::ID3D12GraphicsCommandList::Reset()");
		mtxDraw.unlock();
		return;
	}



	// Record new commands in the command list:

	// Set the viewport and scissor rect. This needs to be reset whenever the command list is reset.

	pCommandList->RSSetViewports(1, &ScreenViewport);
	pCommandList->RSSetScissorRects(1, &ScissorRect);



	// Translate back buffer state from present state to render target state.


	pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(getCurrentBackBufferResource(), 
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));



	// Clear buffers.

	// nullptr - "ClearRenderTargetView clears the entire resource view".
	pCommandList->ClearRenderTargetView(getCurrentBackBufferViewHandle(), backBufferFillColor, 0, nullptr);
	pCommandList->ClearDepthStencilView(getDepthStencilViewHandle(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);


	
	// Binds the RTV and DSV to the rendering pipeline.

	pCommandList->OMSetRenderTargets(1, &getCurrentBackBufferViewHandle(), true, &getDepthStencilViewHandle());



	// CBV/SRV heap.

	ID3D12DescriptorHeap* descriptorHeaps[] = { pCBVSRVHeap.Get() };
	pCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	pCommandList->SetGraphicsRootSignature(pRootSignature.Get());


	// Render pass cb.

	int iRenderPassCBVIndex = iRenderPassCBVOffset + iCurrentFrameResourceIndex;
	auto renderPassCBVHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(pCBVSRVHeap->GetGPUDescriptorHandleForHeapStart());
	renderPassCBVHandle.Offset(iRenderPassCBVIndex, iCBVSRVUAVDescriptorSize);

	pCommandList->SetGraphicsRootDescriptorTable(0, renderPassCBVHandle);



	// Draw.

	iLastFrameDrawCallCount = 0;

	mtxSpawnDespawn.lock();

	drawOpaqueComponents();

	if (bUseFillModeWireframe)
	{
		if (MSAA_Enabled)
		{
			pCommandList->SetPipelineState(pTransparentAlphaToCoverageWireframePSO.Get());
		}
		else
		{
			pCommandList->SetPipelineState(pTransparentWireframePSO.Get());
		}
	}
	else
	{
		if (MSAA_Enabled)
		{
			pCommandList->SetPipelineState(pTransparentAlphaToCoveragePSO.Get());
		}
		else
		{
			pCommandList->SetPipelineState(pTransparentPSO.Get());
		}
	}

	drawTransparentComponents();

	mtxSpawnDespawn.unlock();



	pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(getCurrentBackBufferResource(), 
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	if (MSAA_Enabled)
	{
		// Resolve MSAA render target to our swap chain buffer.

		CD3DX12_RESOURCE_BARRIER barriers1[] = {
			CD3DX12_RESOURCE_BARRIER::Transition(pMSAARenderTarget.Get(), 
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RESOLVE_SOURCE),
			CD3DX12_RESOURCE_BARRIER::Transition(getCurrentBackBufferResource(true), 
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RESOLVE_DEST)
		};

		CD3DX12_RESOURCE_BARRIER barriers2[] = {
			CD3DX12_RESOURCE_BARRIER::Transition(pMSAARenderTarget.Get(), 
			D3D12_RESOURCE_STATE_RESOLVE_SOURCE, D3D12_RESOURCE_STATE_PRESENT),
			CD3DX12_RESOURCE_BARRIER::Transition(getCurrentBackBufferResource(true), 
			D3D12_RESOURCE_STATE_RESOLVE_DEST, D3D12_RESOURCE_STATE_PRESENT)
		};

		pCommandList->ResourceBarrier(2, barriers1);

		pCommandList->ResolveSubresource(getCurrentBackBufferResource(true), 0, pMSAARenderTarget.Get(), 0, BackBufferFormat);

		pCommandList->ResourceBarrier(2, barriers2);
	}


	// Stop recording commands.

	hresult = pCommandList->Close();
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::draw::ID3D12GraphicsCommandList::Close()");
		mtxDraw.unlock();
		return;
	}



	// Add the command list to the command queue for execution.

	ID3D12CommandList* commandLists[] = { pCommandList.Get() };
	pCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);



	// Swap back & front buffers.

	UINT SyncInterval = 0;

	if (bVSyncEnabled)
	{
		// Max: 4.
		SyncInterval = 1;
	}

	if (bFullscreen)
	{
		// The DXGI_PRESENT_ALLOW_TEARING flag cannot be used in an application that is currently in full screen
		// exclusive mode (set by calling SetFullscreenState(TRUE)). It can only be used in windowed mode.
		hresult = pSwapChain->Present(SyncInterval, 0);
	}
	else
	{
		if (bVSyncEnabled)
		{
			hresult = pSwapChain->Present(SyncInterval, 0);
		}
		else
		{
			hresult = pSwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
		}
	}
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::draw::IDXGISwapChain1::Present()");
		mtxDraw.unlock();
		return;
	}
	
	if (iCurrentBackBuffer == (iSwapChainBufferCount - 1))
	{
		iCurrentBackBuffer = 0;
	}
	else
	{
		iCurrentBackBuffer++;
	}

	iCurrentFence++;
	pCurrentFrameResource->iFence = iCurrentFence;

	// Add an instruction to the command queue to set a new fence point.
	// This fence point won't be set until the GPU finishes processing all the commands prior to this Signal().
	pCommandQueue->Signal(pFence.Get(), iCurrentFence);

	mtxDraw.unlock();
}

void SApplication::drawOpaqueComponents()
{
	for (size_t i = 0; i < vAllRenderableSpawnedOpaqueComponents.size(); i++)
	{
		if (vAllRenderableSpawnedOpaqueComponents[i]->getContainer()->isVisible())
		{
			drawComponent(vAllRenderableSpawnedOpaqueComponents[i]);
		}
	}
}

void SApplication::drawTransparentComponents()
{
	for (size_t i = 0; i < vAllRenderableSpawnedTransparentComponents.size(); i++)
	{
		if (vAllRenderableSpawnedTransparentComponents[i]->getContainer()->isVisible())
		{
			drawComponent(vAllRenderableSpawnedTransparentComponents[i]);
		}
	}
}

void SApplication::drawComponent(SComponent* pComponent)
{
	bool bDrawThisComponent = false;

	if (pComponent->componentType == SCT_MESH)
	{
		SMeshComponent* pMeshComponent = dynamic_cast<SMeshComponent*>(pComponent);

		if (pMeshComponent->isVisible() && pMeshComponent->getMeshData().getVerticesCount() > 0)
		{
			bDrawThisComponent = true;
		}
	}
	else if (pComponent->componentType == SCT_RUNTIME_MESH)
	{
		SRuntimeMeshComponent* pRuntimeMeshComponent = dynamic_cast<SRuntimeMeshComponent*>(pComponent);

		if (pRuntimeMeshComponent->isVisible() && pRuntimeMeshComponent->getMeshData().getVerticesCount() > 0)
		{
			bDrawThisComponent = true;
		}
	}

	if (bDrawThisComponent)
	{
		pCommandList->IASetVertexBuffers(0, 1, &pComponent->getRenderData()->pGeometry->getVertexBufferView());
		pCommandList->IASetIndexBuffer(&pComponent->getRenderData()->pGeometry->getIndexBufferView());
		pCommandList->IASetPrimitiveTopology(pComponent->getRenderData()->primitiveTopologyType);


		size_t iObjCount = roundUp(iActualObjectCBCount, OBJECT_CB_RESIZE_MULTIPLE);
		size_t iMaterialCount = roundUp(vRegisteredMaterials.size(), OBJECT_CB_RESIZE_MULTIPLE);
		size_t iTextureCount = vLoadedTextures.size();



		// Texture descriptor table.

		auto heapHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(pCBVSRVHeap->GetGPUDescriptorHandleForHeapStart());

		STextureHandle tex;
		bool bHasTexture = false;

		if (pComponent->componentType == SCT_MESH)
		{
			SMeshComponent* pMeshComponent = dynamic_cast<SMeshComponent*>(pComponent);

			if (pMeshComponent->getMeshMaterial())
			{
				if (pMeshComponent->getMeshMaterial()->getMaterialProperties().getDiffuseTexture(&tex) == false)
				{
					bHasTexture = true;
				}
			}
		}
		else
		{
			SRuntimeMeshComponent* pRuntimeMeshComponent = dynamic_cast<SRuntimeMeshComponent*>(pComponent);

			if (pRuntimeMeshComponent->getMeshMaterial())
			{
				if (pRuntimeMeshComponent->getMeshMaterial()->getMaterialProperties().getDiffuseTexture(&tex) == false)
				{
					bHasTexture = true;
				}
			}
		}

		if (bHasTexture)
		{
			heapHandle.Offset(iObjCount + iMaterialCount + iCurrentFrameResourceIndex * (iObjCount + iMaterialCount + iTextureCount)
				+ tex.pRefToTexture->iTexSRVHeapIndex, iCBVSRVUAVDescriptorSize);
			pCommandList->SetGraphicsRootDescriptorTable(3, heapHandle);
		}



		// Object descriptor table.

		UINT iCBVIndex = iCurrentFrameResourceIndex * (iObjCount + iMaterialCount + iTextureCount) + pComponent->getRenderData()->iObjCBIndex;
		auto cbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(pCBVSRVHeap->GetGPUDescriptorHandleForHeapStart());
		cbvHandle.Offset(iCBVIndex, iCBVSRVUAVDescriptorSize);

		pCommandList->SetGraphicsRootDescriptorTable(1, cbvHandle);



		// Material descriptor table.

		size_t iMatCBIndex = 0;

		if (pComponent->meshData.getMeshMaterial())
		{
			iMatCBIndex = pComponent->meshData.getMeshMaterial()->iMatCBIndex;
		}

		iCBVIndex = iObjCount + iCurrentFrameResourceIndex * (iObjCount + iMaterialCount + iTextureCount) + iMatCBIndex;
		cbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(pCBVSRVHeap->GetGPUDescriptorHandleForHeapStart());
		cbvHandle.Offset(iCBVIndex, iCBVSRVUAVDescriptorSize);

		pCommandList->SetGraphicsRootDescriptorTable(2, cbvHandle);



		// Draw.

		pCommandList->DrawIndexedInstanced(pComponent->getRenderData()->iIndexCount, 1, pComponent->getRenderData()->iStartIndexLocation,
			pComponent->getRenderData()->iStartVertexLocation, 0);

		iLastFrameDrawCallCount++;
	}
}

bool SApplication::flushCommandQueue()
{
	iCurrentFence++;

	HRESULT hresult = pCommandQueue->Signal(pFence.Get(), iCurrentFence);
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::flushCommandQueue::ID3D12CommandQueue::Signal()");
		return true;
	}

	// Wait until the GPU has completed commands up to this fence point.
	if (pFence->GetCompletedValue() < iCurrentFence)
	{
		HANDLE hEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);

		// Fire event when GPU hits current fence.  
		hresult = pFence->SetEventOnCompletion(iCurrentFence, hEvent);
		if (FAILED(hresult))
		{
			SError::showErrorMessageBox(hresult, L"SApplication::flushCommandQueue::ID3D12Fence::SetEventOnCompletion()");
			return true;
		}

		// Wait until event is fired.
		WaitForSingleObject(hEvent, INFINITE);
		
		CloseHandle(hEvent);
	}

	return false;
}

void SApplication::calculateFrameStats()
{
	static int iFrameCount = 0;
	static float fTimeElapsed = 0.0f;

	iFrameCount++;

	if ((gameTimer.getTimeElapsedInSec() - fTimeElapsed) >= 1.0f)
	{
		float fTimeToRenderFrame = 1000.0f / iFrameCount;

		if (bShowFrameStatsInTitle)
		{
			std::wstring sFPS = L"FPS: " + std::to_wstring(iFrameCount);
			std::wstring sAvrTimeToRenderFrame = L"Avr. time to render a frame: " + std::to_wstring(fTimeToRenderFrame);

			std::wstring sWindowTitleText = sMainWindowTitle + L" (" + sFPS + L", " + sAvrTimeToRenderFrame + L")";

			SetWindowText(hMainWindow, sWindowTitleText.c_str());
		}

		iFPS = iFrameCount;
		this->fTimeToRenderFrame = fTimeToRenderFrame;

		iFrameCount = 0;
		fTimeElapsed = gameTimer.getTimeElapsedInSec();
	}
}

size_t SApplication::roundUp(size_t iNum, size_t iMultiple)
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

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 3> SApplication::getStaticSamples()
{
	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0,
		D3D12_FILTER_MIN_MAG_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP
	);

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		1,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP
	);

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		2,
		D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP
	);

	return { pointWrap, linearWrap, anisotropicWrap };
}

LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return SApplication::getApp()->msgProc(hwnd, msg, wParam, lParam);
}

bool SApplication::createMainWindow()
{
	WNDCLASSEX wc;
	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = MainWindowProc; 
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hApplicationInstance;
	wc.hCursor       = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.lpszMenuName  = 0;
	wc.lpszClassName = L"MainWindow";
	wc.hIcon		 = LoadIcon(NULL, IDI_APPLICATION);
	wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);
	wc.cbSize        = sizeof(wc);

	if( !RegisterClassEx(&wc) )
	{
		std::wstring sOutputText = L"An error occurred at SApplication::createMainWindow::RegisterClass(). Error code: " + std::to_wstring(GetLastError());
		MessageBox(0, sOutputText.c_str(), L"Error", 0);
		return true;
	}


	RECT R = { 0, 0, iMainWindowWidth, iMainWindowHeight };
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
	int iWidth  = R.right - R.left;
	int iHeight = R.bottom - R.top;


	hMainWindow = CreateWindow(L"MainWindow", sMainWindowTitle.c_str(), 
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, iWidth, iHeight, 0, 0, hApplicationInstance, 0); 
	if( !hMainWindow )
	{
		std::wstring sOutputText = L"An error occurred at SApplication::createMainWindow::CreateWindow(). Error code: " + std::to_wstring(GetLastError());
		MessageBox(0, sOutputText.c_str(), L"Error", 0);
		return true;
	}


	ShowWindow(hMainWindow, SW_SHOWMAXIMIZED);
	bWindowMaximized = true;
	UpdateWindow(hMainWindow);


	SetWindowText(hMainWindow, sMainWindowTitle.c_str());


	
	RAWINPUTDEVICE Rid[2];

	// Mouse
	Rid[0].usUsagePage = 0x01; 
	Rid[0].usUsage = 0x02; 
	Rid[0].dwFlags = 0;
	Rid[0].hwndTarget = hMainWindow;

	// Keyboard
	//Rid[1].usUsagePage = 0x01; 
	//Rid[1].usUsage = 0x06; 
	//Rid[1].dwFlags = 0;
	//Rid[1].hwndTarget = hMainWindow;

	if (RegisterRawInputDevices(Rid, 1, sizeof(Rid[0])) == FALSE) 
	//if (RegisterRawInputDevices(Rid, 2, sizeof(Rid[0])) == FALSE) 
	{
		SError::showErrorMessageBox(L"SApplication::createMainWindow::RegisterRawInputDevices()", std::to_wstring(GetLastError()));
		return true;
	}
	else
	{
		bRawInputReady = true;
	}

	return false;
}

bool SApplication::initD3DSecondStage()
{
	if (createSwapChain())
	{
		return true;
	}



	if (createRTVAndDSVDescriptorHeaps())
	{
		return true;
	}



	// Disable alt + enter.

	HRESULT hresult = pFactory->MakeWindowAssociation(hMainWindow, DXGI_MWA_NO_ALT_ENTER);
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::initD3DSecondStage::IDXGIFactory4::MakeWindowAssociation()");
		return true;
	}


	return false;
}

bool SApplication::initD3DFirstStage()
{
	DWORD debugFactoryFlags = 0;

#if defined(DEBUG) || defined(_DEBUG) 
	if (bD3DDebugLayerEnabled)
	// Enable the D3D12 debug layer.
	{
		Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));

		debugController->EnableDebugLayer();


		Microsoft::WRL::ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiInfoQueue.GetAddressOf()))))
		{
			debugFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;

			dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
			dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
		}
	}
#endif

	HRESULT hresult;

	// Create DXGI Factory

	hresult = CreateDXGIFactory2(debugFactoryFlags, IID_PPV_ARGS(&pFactory));

	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::initD3DFirstStage::CreateDXGIFactory1()");
		return true;
	}




	// Get supported hardware display adapter.

	if (getFirstSupportedDisplayAdapter(*pAdapter.GetAddressOf()))
	{
		MessageBox(0, 
			L"An error occurred at SApplication::initD3DFirstStage::getFirstSupportedDisplayAdapter(). Error: Can't find a supported display adapter.",
			L"Error",
			0);

		return true;
	}




	// Create device.

	hresult = D3D12CreateDevice(
		pAdapter.Get(),
		ENGINE_D3D_FEATURE_LEVEL,
		IID_PPV_ARGS(&pDevice));

	if (FAILED(hresult))
	{
		// Try to create device with WARP (software) adapter.

		Microsoft::WRL::ComPtr<IDXGIAdapter> WarpAdapter;
		pFactory->EnumWarpAdapter(IID_PPV_ARGS(&WarpAdapter));

		hresult = D3D12CreateDevice(
			WarpAdapter.Get(),
			ENGINE_D3D_FEATURE_LEVEL,
			IID_PPV_ARGS(&pDevice));

		if (FAILED(hresult))
		{
			SError::showErrorMessageBox(hresult, L"SApplication::initD3DFirstStage::D3D12CreateDevice() (WARP adapter)");
			return true;
		}
		else
		{
			bUsingWARPAdapter = true;
		}
	}




	// Create Fence and descriptor sizes.

	hresult = pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pFence));

	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::initD3DFirstStage::ID3D12Device::CreateFence()");
		return true;
	}

	iRTVDescriptorSize       = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	iDSVDescriptorSize       = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	iCBVSRVUAVDescriptorSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);



	if (checkMSAASupport())
	{
		return true;
	}



	if (createCommandObjects())
	{
		return true;
	}



	if (getFirstOutputDisplay(*pOutput.GetAddressOf()))
	{
		MessageBox(0, 
			L"An error occurred at SApplication::initDirect3D::getFirstOutputAdapter(). Error: Can't find any output adapters for current display adapter.",
			L"Error",
			0);

		return true;
	}


	if (getScreenParams(true))
	{
		return true;
	}

	return false;
}

bool SApplication::getFirstSupportedDisplayAdapter(IDXGIAdapter3*& pAdapter)
{
	pAdapter = nullptr;

	if (sPreferredDisplayAdapter != L"")
	{
		for (UINT adapterIndex = 0; ; ++adapterIndex)
		{
			IDXGIAdapter3* pAdapter1 = nullptr;

			if (pFactory->EnumAdapters(adapterIndex, reinterpret_cast<IDXGIAdapter**>(&pAdapter1)) == DXGI_ERROR_NOT_FOUND)
			{
				// No more adapters to enumerate.
				break;
			} 



			// Check to see if the adapter supports Direct3D version, but don't create the actual device yet.

			if (SUCCEEDED(D3D12CreateDevice(pAdapter1, ENGINE_D3D_FEATURE_LEVEL, _uuidof(ID3D12Device), nullptr)))
			{
				DXGI_ADAPTER_DESC desc;
				pAdapter1->GetDesc(&desc);

				if (desc.Description == sPreferredDisplayAdapter)
				{
					pAdapter = pAdapter1;
					return false;
				}
			}

			pAdapter1->Release();
			pAdapter1 = nullptr;
		}
	}

	for (UINT adapterIndex = 0; ; ++adapterIndex)
	{
		IDXGIAdapter3* pAdapter1 = nullptr;

		if (pFactory->EnumAdapters(adapterIndex, reinterpret_cast<IDXGIAdapter**>(&pAdapter1)) == DXGI_ERROR_NOT_FOUND)
		{
			// No more adapters to enumerate.
			break;
		} 



		// Check to see if the adapter supports Direct3D version, but don't create the actual device yet.

		if (SUCCEEDED(D3D12CreateDevice(pAdapter1, ENGINE_D3D_FEATURE_LEVEL, _uuidof(ID3D12Device), nullptr)))
		{
			pAdapter = pAdapter1;
			return false;
		}

		pAdapter1->Release();
		pAdapter1 = nullptr;
	}

	return true;
}

bool SApplication::getFirstOutputDisplay(IDXGIOutput*& pOutput)
{
	pOutput = nullptr;

	if (sPreferredOutputAdapter != L"")
	{
		for (UINT outputIndex = 0; ; ++outputIndex)
		{
			IDXGIOutput* pOutput1 = nullptr;

			if (pAdapter->EnumOutputs(outputIndex, &pOutput1) == DXGI_ERROR_NOT_FOUND)
			{
				// No more displays to enumerate.
				break;
			} 



			DXGI_OUTPUT_DESC desc;
			pOutput1->GetDesc(&desc);

			if (desc.DeviceName == sPreferredDisplayAdapter)
			{
				pOutput = pOutput1;

				return false;
			}

			pOutput1->Release();
			pOutput1 = nullptr;
		}
	}

	for (UINT outputIndex = 0; ; ++outputIndex)
	{
		IDXGIOutput* pOutput1 = nullptr;

		if (pAdapter->EnumOutputs(outputIndex, &pOutput1) == DXGI_ERROR_NOT_FOUND)
		{
			// No more adapters to enumerate.
			break;
		}


		pOutput = pOutput1;

		return false;
	}

	return true;
}

bool SApplication::checkMSAASupport()
{
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format           = BackBufferFormat;
	msQualityLevels.SampleCount      = MSAA_SampleCount;
	msQualityLevels.Flags            = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;

	HRESULT hresult = pDevice->CheckFeatureSupport(
		D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&msQualityLevels,
		sizeof(msQualityLevels));

	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::checkMSAASupport::ID3D12Device::CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS)");
		return true;
	}

	if (msQualityLevels.NumQualityLevels == 0)
	{
		/*MessageBox(0, L"An error occurred at SApplication::checkMSAASupport::CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS). "
			"Error: NumQualityLevels == 0.", L"Error", 0);*/
		return true;
	}

	MSAA_Quality = msQualityLevels.NumQualityLevels;

	return false;
}

bool SApplication::createCommandObjects()
{
	// Create Command Queue.

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	HRESULT hresult = pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&pCommandQueue));

	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::createCommandObjects::ID3D12Device::CreateCommandQueue()");
		return true;
	}




	// Create Command Allocator.

	hresult = pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(pCommandListAllocator.GetAddressOf()));

	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::createCommandObjects::ID3D12Device::CreateCommandAllocator()");
		return true;
	}




	// Create Command List.

	hresult = pDevice->CreateCommandList(
		0, // Create list for 1 GPU. See pDevice->GetNodeCount() and documentation for more info.
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		pCommandListAllocator.Get(), // Associated command allocator
		nullptr,
		IID_PPV_ARGS(pCommandList.GetAddressOf()));

	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::createCommandObjects::ID3D12Device::CreateCommandList()");
		return true;
	}



	// Start off in a closed state. This is because the first time we refer 
	// to the command list we will Reset() it, and it needs to be closed before
	// calling Reset().
	pCommandList->Close();


	return false;
}

bool SApplication::createSwapChain()
{
	// Release the previous swapchain.
	pSwapChain.Reset();


	DXGI_SWAP_CHAIN_DESC1 desc;
	desc.Width  = iMainWindowWidth;
	desc.Height = iMainWindowHeight;
	desc.Format = BackBufferFormat;
	desc.Stereo = false;

	desc.SampleDesc.Count   = 1;
	desc.SampleDesc.Quality = 0;

	desc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.BufferCount        = iSwapChainBufferCount;

	// If the size of the back buffer is not equal to the target output: stretch.
	desc.Scaling    = DXGI_SCALING_STRETCH;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	desc.AlphaMode  = DXGI_ALPHA_MODE_UNSPECIFIED;

	if (bVSyncEnabled)
	{
		desc.Flags      = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	}
	else
	{
		desc.Flags      = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
	}

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC fdesc;
	fdesc.RefreshRate.Numerator   = iRefreshRateNumerator;
	fdesc.RefreshRate.Denominator = iRefreshRateDenominator;
	fdesc.Scaling = iScaling;
	fdesc.ScanlineOrdering = iScanlineOrder;
	fdesc.Windowed = !bFullscreen;

	// Note: Swap chain uses queue to perform flush.
	HRESULT hresult = pFactory->CreateSwapChainForHwnd(pCommandQueue.Get(), hMainWindow, &desc, &fdesc, pOutput.Get(), pSwapChain.GetAddressOf());
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::createSwapChain::IDXGIFactory4::CreateSwapChainForHwnd()");
		return true;
	}

	return false;
}

bool SApplication::getScreenParams(bool bApplyResolution)
{
	UINT numModes = 0;

	HRESULT hresult = pOutput->GetDisplayModeList(BackBufferFormat, 0, &numModes, NULL);
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::initDirect3D::IDXGIOutput::GetDisplayModeList() (count)");
		return true;
	}

	std::vector<DXGI_MODE_DESC> vDisplayModes(numModes);

	hresult = pOutput->GetDisplayModeList(BackBufferFormat, 0, &numModes, &vDisplayModes[0]);
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::initDirect3D::IDXGIOutput::GetDisplayModeList() (list)");
		return true;
	}


	// Save params.
	
	bool bSetResolutionToDefault = true;

	if (bCustomWindowSize)
	{
		// Not default params. Look if this resolution is supported.

		for (int i = vDisplayModes.size() - 1; i > 0; i--)
		{
			if ((vDisplayModes[i].Width == iMainWindowWidth)
				&&
				(vDisplayModes[i].Height == iMainWindowHeight))
			{
				bSetResolutionToDefault = false;

				iRefreshRateNumerator   = vDisplayModes[i].RefreshRate.Numerator;
				iRefreshRateDenominator = vDisplayModes[i].RefreshRate.Denominator;

				iScanlineOrder          = vDisplayModes[i].ScanlineOrdering;

				/*if ((iMainWindowWidth != vDisplayModes.back().Width) && (iMainWindowHeight != vDisplayModes.back().Height))
				{
					iScaling = DXGI_MODE_SCALING_STRETCHED;
				}
				else
				{
					iScaling = vDisplayModes[i].Scaling;
				}*/

				break;
			}
		}
	}
	
	if (bSetResolutionToDefault)
	{
		// Set default params for this output.

		// Use the last element in the list because it has the highest resolution.

		if (bApplyResolution)
		{
			iMainWindowWidth        = vDisplayModes.back().Width;
			iMainWindowHeight       = vDisplayModes.back().Height;
		}

		iRefreshRateNumerator   = vDisplayModes.back().RefreshRate.Numerator;
		iRefreshRateDenominator = vDisplayModes.back().RefreshRate.Denominator;

		iScanlineOrder          = vDisplayModes.back().ScanlineOrdering;
		iScaling                = vDisplayModes.back().Scaling;

		//if (bFullscreen)
		//{
		//	// Use the last element in the list because it has the highest resolution.

		//	if (bApplyResolution)
		//	{
		//		iMainWindowWidth        = vDisplayModes.back().Width;
		//		iMainWindowHeight       = vDisplayModes.back().Height;
		//	}

		//	iRefreshRateNumerator   = vDisplayModes.back().RefreshRate.Numerator;
		//	iRefreshRateDenominator = vDisplayModes.back().RefreshRate.Denominator;

		//	iScanlineOrder          = vDisplayModes.back().ScanlineOrdering;
		//	iScaling                = vDisplayModes.back().Scaling;
		//}
		//else
		//{
		//	// Find previous element in the list with the same vDisplayModes.back().ScanlineOrdering && vDisplayModes.back().Scaling.

		//	for (int i = vDisplayModes.size() - 2; i > 0; i--)
		//	{
		//		if ((vDisplayModes[i].ScanlineOrdering == vDisplayModes.back().ScanlineOrdering)
		//			&&
		//			(vDisplayModes[i].Scaling == vDisplayModes.back().Scaling))
		//		{
		//			if (bApplyResolution)
		//			{
		//				iMainWindowWidth        = vDisplayModes[i].Width;
		//				iMainWindowHeight       = vDisplayModes[i].Height;
		//			}

		//			iRefreshRateNumerator   = vDisplayModes[i].RefreshRate.Numerator;
		//			iRefreshRateDenominator = vDisplayModes[i].RefreshRate.Denominator;

		//			iScanlineOrder          = vDisplayModes[i].ScanlineOrdering;
		//			iScaling                = vDisplayModes[i].Scaling;

		//			break;
		//		}
		//	}
		//}
	}

	

	return false;
}

bool SApplication::createRTVAndDSVDescriptorHeaps()
{
	// RTV

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = iSwapChainBufferCount + 1; // "+ 1" for MSAA Render Target
	rtvHeapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask       = 0;

	HRESULT hresult = pDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(pRTVHeap.GetAddressOf()));
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::createRTVAndDSVDescriptorHeaps::ID3D12Device::CreateDescriptorHeap() (RTV)");
		return true;
	}




	// DSV

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask       = 0;

	hresult = pDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(pDSVHeap.GetAddressOf()));
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::createRTVAndDSVDescriptorHeaps::ID3D12Device::CreateDescriptorHeap() (DSV)");
		return true;
	}

	return false;
}

bool SApplication::createCBVSRVHeap()
{
	size_t iObjCount = roundUp(iActualObjectCBCount, OBJECT_CB_RESIZE_MULTIPLE); // for SObjectConstants
	iObjCount += roundUp(vRegisteredMaterials.size(), OBJECT_CB_RESIZE_MULTIPLE); // for SMaterialConstants
	iObjCount += vLoadedTextures.size(); // one SRV per texture
	// new stuff goes here

	// Each frame resource contains N objects, so we need (iFrameResourcesCount * N)
	// + 1 for SRenderPassConstants per frame resource.
	UINT iDescriptorCount = (iObjCount + 1) * iFrameResourcesCount;

	// Save an offset to the start of the render pass CBVs.
	iRenderPassCBVOffset = iObjCount * iFrameResourcesCount;

	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.NumDescriptors = iDescriptorCount;
	cbvHeapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask       = 0;

	HRESULT hresult = pDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&pCBVSRVHeap));
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::createCBVDescriptorHeap::ID3D12Device::CreateDescriptorHeap()");
		return true;
	}

	return false;
}

void SApplication::createViews()
{
	UINT iObjectConstantBufferSizeInBytes = SMath::makeMultipleOf256(sizeof(SObjectConstants));

	size_t iObjectCount   = roundUp(iActualObjectCBCount, OBJECT_CB_RESIZE_MULTIPLE);
	size_t iMaterialCount = roundUp(vRegisteredMaterials.size(), OBJECT_CB_RESIZE_MULTIPLE);
	size_t iTextureCount  = vLoadedTextures.size();

	// Need (iFrameResourcesCount * iObjectCount) CBVs.
	for (int iFrameIndex = 0; iFrameIndex < iFrameResourcesCount; iFrameIndex++)
	{
		ID3D12Resource* pObjectsCB = vFrameResources[iFrameIndex]->pObjectsCB->getResource();

		for (int i = 0; i < iObjectCount; i++)
		{
			D3D12_GPU_VIRTUAL_ADDRESS currentObjectConstantBufferAddress = pObjectsCB->GetGPUVirtualAddress();

			// Offset to the ith object constant buffer in the buffer.
			currentObjectConstantBufferAddress += i * iObjectConstantBufferSizeInBytes;

			// Offset to the object CBV in the descriptor heap.
			int iIndexInHeap = iFrameIndex * (iObjectCount + iMaterialCount + iTextureCount) + i;
			auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(pCBVSRVHeap->GetCPUDescriptorHandleForHeapStart());
			handle.Offset(iIndexInHeap, iCBVSRVUAVDescriptorSize);

			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
			cbvDesc.BufferLocation = currentObjectConstantBufferAddress;
			cbvDesc.SizeInBytes = iObjectConstantBufferSizeInBytes;

			pDevice->CreateConstantBufferView(&cbvDesc, handle);
		}
	}



	UINT iMaterialCBSizeInBytes = SMath::makeMultipleOf256(sizeof(SMaterialConstants));

	// Need (iFrameResourcesCount * iMaterialCount) CBVs.
	for (int iFrameIndex = 0; iFrameIndex < iFrameResourcesCount; iFrameIndex++)
	{
		ID3D12Resource* pMaterialCB = vFrameResources[iFrameIndex]->pMaterialCB->getResource();

		for (int i = 0; i < iMaterialCount; i++)
		{
			D3D12_GPU_VIRTUAL_ADDRESS currentMaterialConstantBufferAddress = pMaterialCB->GetGPUVirtualAddress();

			// Offset to the ith material constant buffer in the buffer.
			currentMaterialConstantBufferAddress += i * iMaterialCBSizeInBytes;

			// Offset to the material CBV in the descriptor heap.
			int iIndexInHeap = iObjectCount + iFrameIndex * (iObjectCount + iMaterialCount + iTextureCount) + i;
			auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(pCBVSRVHeap->GetCPUDescriptorHandleForHeapStart());
			handle.Offset(iIndexInHeap, iCBVSRVUAVDescriptorSize);

			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
			cbvDesc.BufferLocation = currentMaterialConstantBufferAddress;
			cbvDesc.SizeInBytes = iMaterialCBSizeInBytes;

			pDevice->CreateConstantBufferView(&cbvDesc, handle);
		}
	}


	// Need one SRV per loaded texture.
	for (int iFrameIndex = 0; iFrameIndex < iFrameResourcesCount; iFrameIndex++)
	{
		for (size_t i = 0; i < vLoadedTextures.size(); i++)
		{
			int iIndexInHeap = iObjectCount + iMaterialCount + iFrameIndex * (iObjectCount + iMaterialCount + iTextureCount) + i;
			auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(pCBVSRVHeap->GetCPUDescriptorHandleForHeapStart());

			handle.Offset(iIndexInHeap, iCBVSRVUAVDescriptorSize);

			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = vLoadedTextures[i]->pResource->GetDesc().Format;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.MipLevels = vLoadedTextures[i]->pResource->GetDesc().MipLevels;
			srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

			pDevice->CreateShaderResourceView(vLoadedTextures[i]->pResource.Get(), &srvDesc, handle);

			vLoadedTextures[i]->iTexSRVHeapIndex = i;
		}
	}



	UINT iRenderPassCBSizeInBytes = SMath::makeMultipleOf256(sizeof(SRenderPassConstants));

	// Need one descriptor for render pass constants per frame resource.
	for (int iFrameIndex = 0; iFrameIndex < iFrameResourcesCount; iFrameIndex++)
	{
		ID3D12Resource* pRenderPassCB = vFrameResources[iFrameIndex]->pRenderPassCB->getResource();

		D3D12_GPU_VIRTUAL_ADDRESS renderPassCBAddress = pRenderPassCB->GetGPUVirtualAddress();

		// Offset in the descriptor heap.
		int iIndexInHeap = iRenderPassCBVOffset + iFrameIndex;
		auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(pCBVSRVHeap->GetCPUDescriptorHandleForHeapStart());
		handle.Offset(iIndexInHeap, iCBVSRVUAVDescriptorSize);

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
		cbvDesc.BufferLocation = renderPassCBAddress;
		cbvDesc.SizeInBytes = iRenderPassCBSizeInBytes;

		pDevice->CreateConstantBufferView(&cbvDesc, handle);
	}
}

void SApplication::createFrameResources()
{
	for (int i = 0; i < iFrameResourcesCount; i++)
	{
		vFrameResources.push_back(std::make_unique<SFrameResource>(pDevice.Get(), 1, 0));
	}
}

bool SApplication::createRootSignature()
{
	// The root signature defines the resources the shader programs expect.

	CD3DX12_DESCRIPTOR_RANGE cbvTable0;
	cbvTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // cbRenderPass

	CD3DX12_DESCRIPTOR_RANGE cbvTable1;
	cbvTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1); // cbObject

	CD3DX12_DESCRIPTOR_RANGE cbvTable2;
	cbvTable2.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2); // cbMaterial

	CD3DX12_DESCRIPTOR_RANGE srvTable0;
	srvTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // srv

	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[4];

	// Perfomance TIP: Order from most frequent to least frequent.
	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable0); // cbRenderPass
	slotRootParameter[1].InitAsDescriptorTable(1, &cbvTable1); // cbObject
	slotRootParameter[2].InitAsDescriptorTable(1, &cbvTable2); // cbMaterial
	slotRootParameter[3].InitAsDescriptorTable(1, &srvTable0, D3D12_SHADER_VISIBILITY_PIXEL); // srv

	// Static samples don't need a heap.
	auto staticSamples = getStaticSamples();

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter, static_cast<UINT>(staticSamples.size()), staticSamples.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);


	Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSignature = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;

	HRESULT hresult = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSignature.GetAddressOf(), errorBlob.GetAddressOf());


	if (errorBlob != nullptr)
	{
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}

	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::createRootSignature::D3D12SerializeRootSignature()");
		return true;
	}


	hresult = pDevice->CreateRootSignature(
		0,
		serializedRootSignature->GetBufferPointer(),
		serializedRootSignature->GetBufferSize(),
		IID_PPV_ARGS(&pRootSignature)
	);
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::createRootSignature::ID3D12Device::CreateRootSignature()");
		return true;
	}


	return false;
}

bool SApplication::createShadersAndInputLayout()
{
	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	mShaders["basicVS"] = SGeometry::compileShader(L"shaders/basic.hlsl", nullptr, "VS", "vs_5_1", bCompileShadersInRelease);
	mShaders["basicPS"] = SGeometry::compileShader(L"shaders/basic.hlsl", nullptr, "PS", "ps_5_1", bCompileShadersInRelease);
	mShaders["basicAlphaPS"] = SGeometry::compileShader(L"shaders/basic.hlsl", alphaTestDefines, "PS", "ps_5_1", bCompileShadersInRelease);

	vInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	return false;
}


bool SApplication::createPSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	memset(&psoDesc, 0, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

	psoDesc.InputLayout = { vInputLayout.data(), static_cast<UINT>(vInputLayout.size()) };
	psoDesc.pRootSignature = pRootSignature.Get();
	psoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["basicVS"]->GetBufferPointer()),
		mShaders["basicVS"]->GetBufferSize()
	};
	psoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["basicPS"]->GetBufferPointer()),
		mShaders["basicPS"]->GetBufferSize()
	};

	CD3DX12_RASTERIZER_DESC rastDesc(D3D12_DEFAULT);
	rastDesc.CullMode = D3D12_CULL_MODE_BACK;
	rastDesc.FillMode = D3D12_FILL_MODE_SOLID;
	rastDesc.MultisampleEnable = MSAA_Enabled;

	D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc;
	transparencyBlendDesc.BlendEnable = true;
	transparencyBlendDesc.LogicOpEnable = false;
	transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	transparencyBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	psoDesc.RasterizerState   = rastDesc;
	psoDesc.BlendState        = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask        = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets  = 1;
	psoDesc.RTVFormats[0]     = BackBufferFormat;
	psoDesc.SampleDesc.Count  = MSAA_Enabled ? MSAA_SampleCount : 1;
	psoDesc.SampleDesc.Quality = MSAA_Enabled ? (MSAA_Quality - 1) : 0;
	psoDesc.DSVFormat         = DepthStencilFormat;


	HRESULT hresult = pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pOpaquePSO));
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::createPSO::ID3D12Device::CreateGraphicsPipelineState()");
		return true;
	}


	D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPsoDesc = psoDesc;
	transparentPsoDesc.BlendState.RenderTarget[0] = transparencyBlendDesc;
	transparentPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	transparentPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["basicAlphaPS"]->GetBufferPointer()),
		mShaders["basicAlphaPS"]->GetBufferSize()
	};
	hresult = pDevice->CreateGraphicsPipelineState(&transparentPsoDesc, IID_PPV_ARGS(&pTransparentPSO));
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::createPSO::ID3D12Device::CreateGraphicsPipelineState()");
		return true;
	}


	D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentAlphaToCoveragePsoDesc = transparentPsoDesc;
	transparentAlphaToCoveragePsoDesc.BlendState.AlphaToCoverageEnable = true;
	hresult = pDevice->CreateGraphicsPipelineState(&transparentAlphaToCoveragePsoDesc, IID_PPV_ARGS(&pTransparentAlphaToCoveragePSO));
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::createPSO::ID3D12Device::CreateGraphicsPipelineState()");
		return true;
	}


	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueWireframePsoDesc = psoDesc;
	opaqueWireframePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	hresult = pDevice->CreateGraphicsPipelineState(&opaqueWireframePsoDesc, IID_PPV_ARGS(&pOpaqueWireframePSO));
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::createPSO::ID3D12Device::CreateGraphicsPipelineState()");
		return true;
	}


	D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentWireframePsoDesc = transparentPsoDesc;
	transparentWireframePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	hresult = pDevice->CreateGraphicsPipelineState(&transparentWireframePsoDesc, IID_PPV_ARGS(&pTransparentWireframePSO));
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::createPSO::ID3D12Device::CreateGraphicsPipelineState()");
		return true;
	}


	D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentAlphaToCoverageWireframePsoDesc = transparentAlphaToCoveragePsoDesc;
	transparentAlphaToCoverageWireframePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	hresult = pDevice->CreateGraphicsPipelineState(&transparentAlphaToCoverageWireframePsoDesc,
		IID_PPV_ARGS(&pTransparentAlphaToCoverageWireframePSO));
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::createPSO::ID3D12Device::CreateGraphicsPipelineState()");
		return true;
	}

	return false;
}

bool SApplication::resetCommandList()
{
	// A command list can be reset after it has been added to the command queue via ExecuteCommandList (was added in init()).

	HRESULT hresult;

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> pCurrentCommandListAllocator = pCurrentFrameResource->pCommandListAllocator;

	hresult = pCommandList->Reset(pCurrentCommandListAllocator.Get(), pOpaquePSO.Get());

	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::resetCommandList::ID3D12GraphicsCommandList::Reset()");
		return true;
	}
	else
	{
		return false;
	}
}

bool SApplication::createDefaultMaterial()
{
	bool bError = false;

	SMaterial* pDefaultMat = registerMaterial(sDefaultEngineMaterialName, bError);

	if (bError)
	{
		showMessageBox(L"Error", L"SApplication::createDefaultMaterial() error: failed to register the default material.");
		return true;
	}
	else
	{
		SMaterialProperties matProps;
		matProps.setDiffuseColor(SVector(1.0f, 0.0f, 0.0f, 1.0f));
		matProps.setSpecularColor(SVector(1.0f, 1.0f, 1.0f));
		matProps.setRoughness(0.0f);

		pDefaultMat->setMaterialProperties(matProps);

		return false;
	}
}

bool SApplication::executeCommandList()
{
	HRESULT hresult = pCommandList->Close();
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::executeCommandList::ID3D12GraphicsCommandList::Close()");
		return true;
	}

	ID3D12CommandList* commandLists[] = { pCommandList.Get() };

	pCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	return false;
}

void SApplication::updateMaterialInFrameResource(SMaterial * pMaterial, SUploadBuffer<SMaterialConstants>* pMaterialCB)
{
	SMaterialConstants matConstants;

	SMaterialProperties matProps = pMaterial->getMaterialProperties();

	SVector vDiffuse = matProps.getDiffuseColor();
	SVector vFresnel = matProps.getSpecularColor();

	matConstants.vDiffuseAlbedo = { vDiffuse.getX(), vDiffuse.getY(), vDiffuse.getZ(), vDiffuse.getW() };
	matConstants.vFresnelR0 = { vFresnel.getX(), vFresnel.getY(), vFresnel.getZ() };
	matConstants.fRoughness = matProps.getRoughness();

	matConstants.bHasDiffuseTexture = matProps.bHasDiffuseTexture;
	matConstants.bHasNormalTexture = matProps.bHasNormalTexture;

	matConstants.fCustomTransparency = matProps.fCustomTransparency;
	matConstants.vFinalDiffuseMult = matProps.vFinalDiffuseMult;

	DirectX::XMMATRIX vMatTransform = DirectX::XMLoadFloat4x4(&pMaterial->vMatTransform);
	DirectX::XMStoreFloat4x4(&matConstants.vMatTransform, DirectX::XMMatrixTranspose(vMatTransform));


	pMaterialCB->copyDataToElement(pMaterial->iMatCBIndex, matConstants);

	// Next FrameResource need to be updated too.
	pMaterial->iUpdateCBInFrameResourceCount--;

	pMaterial->iFrameResourceIndexLastUpdated = iCurrentFrameResourceIndex;

	if (pMaterial->iUpdateCBInFrameResourceCount == 0)
	{
		pMaterial->bLastFrameResourceIndexValid = false;
	}
	else
	{
		pMaterial->bLastFrameResourceIndexValid = true;
	}
}

ID3D12Resource* SApplication::getCurrentBackBufferResource(bool bNonMSAAResource) const
{
	if (MSAA_Enabled && bNonMSAAResource == false)
	{
		return pMSAARenderTarget.Get();
	}
	else
	{
		return pSwapChainBuffer[iCurrentBackBuffer].Get();
	}
}

D3D12_CPU_DESCRIPTOR_HANDLE SApplication::getCurrentBackBufferViewHandle() const
{
	if (MSAA_Enabled)
	{
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(
			pRTVHeap->GetCPUDescriptorHandleForHeapStart(),
			2,
			iRTVDescriptorSize);
	}
	else
	{
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(
			pRTVHeap->GetCPUDescriptorHandleForHeapStart(),
			iCurrentBackBuffer,
			iRTVDescriptorSize);
	}
}

D3D12_CPU_DESCRIPTOR_HANDLE SApplication::getDepthStencilViewHandle() const
{
	return pDSVHeap->GetCPUDescriptorHandleForHeapStart();
}

void SApplication::showDeviceRemovedReason()
{
	HRESULT hResult = pDevice->GetDeviceRemovedReason();

	SError::showErrorMessageBox(hResult, L"SApplication::showDeviceRemovedReason()");
}

void SApplication::removeComponentsFromGlobalVectors(SContainer* pContainer)
{
	std::vector<SComponent*> vOpaqueMeshComponents;
	std::vector<SComponent*> vTransparentMeshComponents;

	pContainer->getAllMeshComponents(&vOpaqueMeshComponents, &vTransparentMeshComponents);

	size_t iLeftComponents = vOpaqueMeshComponents.size();

	for (long long i = 0; i < vAllRenderableSpawnedOpaqueComponents.size(); i++)
	{
		for (long long j = 0; j < vOpaqueMeshComponents.size(); j++)
		{
			if (vAllRenderableSpawnedOpaqueComponents[i] == vOpaqueMeshComponents[j])
			{
				vAllRenderableSpawnedOpaqueComponents.erase(vAllRenderableSpawnedOpaqueComponents.begin() + i);
				i--;

				iLeftComponents--;

				break;
			}
		}

		if (iLeftComponents == 0)
		{
			break;
		}
	}

	if (iLeftComponents != 0)
	{
		showMessageBox(L"Error", L"SApplication::despawnContainerFromLevel() error: not all opaque components were removed.");
	}



	iLeftComponents = vTransparentMeshComponents.size();

	for (long long i = 0; i < vAllRenderableSpawnedTransparentComponents.size(); i++)
	{
		for (long long j = 0; j < vTransparentMeshComponents.size(); j++)
		{
			if (vAllRenderableSpawnedTransparentComponents[i] == vTransparentMeshComponents[j])
			{
				vAllRenderableSpawnedTransparentComponents.erase(vAllRenderableSpawnedTransparentComponents.begin() + i);
				i--;

				iLeftComponents--;

				break;
			}
		}

		if (iLeftComponents == 0)
		{
			break;
		}
	}

	if (iLeftComponents != 0)
	{
		showMessageBox(L"Error", L"SApplication::despawnContainerFromLevel() error: not all transparent components were removed.");
	}
}

SApplication::SApplication(HINSTANCE hInstance)
{
	hApplicationInstance = hInstance;
	pApp           = this;

	pVideoSettings = new SVideoSettings(this);
	pProfiler      = new SProfiler(this);

	pCurrentLevel  = new SLevel(this);

#if defined(DEBUG) || defined(_DEBUG)
	bCompileShadersInRelease = false;
#else
	bCompileShadersInRelease = true;
#endif
}

SApplication::~SApplication()
{
	bExitCalled = true; // delete containers when the level will despawn them

	if (pCurrentLevel)
	{
		delete pCurrentLevel;
	}

	if(bInitCalled)
	{
		flushCommandQueue();

		if (bFullscreen)
		{
			// From docs: "Before releasing a swap chain, first switch to windowed mode".
			pSwapChain->SetFullscreenState(false, NULL);
		}
	}

	mtxSpawnDespawn.lock();
	mtxSpawnDespawn.unlock();


	for (size_t i = 0; i < vLoadedTextures.size(); i++)
	{
		unsigned long iLeft = vLoadedTextures[i]->pResource.Reset();

		if (iLeft != 0)
		{
			showMessageBox(L"Error", L"SApplication::~SApplication() error: texture ref count is not 0.");
		}

		delete vLoadedTextures[i];
	}

	vLoadedTextures.clear();


	for (size_t i = 0; i < vRegisteredMaterials.size(); i++)
	{
		delete vRegisteredMaterials[i];
	}

	vRegisteredMaterials.clear();


	delete pVideoSettings;
	delete pProfiler;
}

void SApplication::initDisableD3DDebugLayer()
{
	bD3DDebugLayerEnabled = false;
}

void SApplication::initCompileShadersInRelease()
{
	bCompileShadersInRelease = true;
}

bool SApplication::init()
{
	// Create Output and ask it about screen resolution.
	if (initD3DFirstStage())
	{
		return true;
	}

	// Create window with supported resolution.
	if (createMainWindow())
	{
		return true;
	}

	if (initD3DSecondStage())
	{
		return true;
	}

	bInitCalled = true;

	// Do the initial resize code.
	onResize();

	HRESULT hresult = pCommandList->Reset(pCommandListAllocator.Get(), nullptr);
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::init::ID3D12GraphicsCommandList::Reset()");
		return true;
	}

	if (createRootSignature())
	{
		return true;
	}

	if (createShadersAndInputLayout())
	{
		return true;
	}

	createFrameResources();

	if (createCBVSRVHeap())
	{
		return true;
	}

	createViews();

	if (createPSO())
	{
		return true;
	}

	if (createDefaultMaterial())
	{
		return true;
	}

	// Execute init commands.

	hresult = pCommandList->Close();
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::init::ID3D12GraphicsCommandList::Close()");
		return true;
	}

	ID3D12CommandList* vCommandListsToExecute[] = { pCommandList.Get() };
	pCommandQueue->ExecuteCommandLists(_countof(vCommandListsToExecute), vCommandListsToExecute);

	

	// Wait for all command to finish.

	if (flushCommandQueue())
	{
		return true;
	}

	return false;
}

LRESULT SApplication::msgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_SIZE:
	{
		// Save new window size.
		iMainWindowWidth  = LOWORD(lParam);
		iMainWindowHeight = HIWORD(lParam);

		fWindowCenterX = iMainWindowWidth / 2.0f;
		fWindowCenterY = iMainWindowHeight / 2.0f;

		if (bInitCalled)
		{
			if( wParam == SIZE_MINIMIZED )
			{
				bWindowMaximized = false;
				bWindowMinimized = true;

				onMinimizeEvent();
			}
			else if( wParam == SIZE_MAXIMIZED )
			{
				bWindowMaximized = true;
				bWindowMinimized = false;
				onResize();

				onMaximizeEvent();
			}
			else if( wParam == SIZE_RESTORED )
			{
				if (bWindowMinimized)
				{
					bWindowMinimized = false;
					onResize();

					onRestoreEvent();
				}
				else if (bWindowMaximized)
				{
					bWindowMaximized = false;
					onResize();

					onRestoreEvent();
				}
				else if(bResizingMoving == false)
				{
					// API call such as SetWindowPos or pSwapChain->SetFullscreenState.
					onResize();
				}
			}
		}

		return 0;
	}
	case WM_ENTERSIZEMOVE:
	{
		// The user grabs the resize bars.

		bResizingMoving = true;

		return 0;
	}
	case WM_EXITSIZEMOVE:
	{
		// The user releases the resize bars.

		bResizingMoving = false;

		onResize();

		return 0;
	}
	case WM_MENUCHAR:
	{
		// The WM_MENUCHAR message is sent when a menu is active and the user presses 
		// a key that does not correspond to any mnemonic or accelerator key. 
		// Don't make *beep* sound when we alt-enter.
		return MAKELRESULT(0, MNC_CLOSE);
	}
	case WM_GETMINMAXINFO:
	{
		// Catch this message so to prevent the window from becoming too small.
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200; 
		return 0;
	}
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_XBUTTONDOWN:
	{
		SMouseKey mousekey;

		if (pressedMouseKey.getButton() != SMB_NONE)
		{
			mousekey.setOtherKey(wParam, pressedMouseKey);
		}
		else
		{
			mousekey.determineKey(wParam);
			pressedMouseKey.setKey(mousekey.getButton());
		}
		

		onMouseDown(mousekey, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

		std::vector<SContainer*>* pvRenderableContainers = nullptr;
		pCurrentLevel->getRenderableContainers(pvRenderableContainers);

		for (size_t i = 0; i < pvRenderableContainers->size(); i++)
		{
			if (pvRenderableContainers->operator[](i)->isUserInputCallsEnabled())
			{
				pvRenderableContainers->operator[](i)->onMouseDown(mousekey, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			}
		}

		std::vector<SContainer*>* pvNotRenderableContainers = nullptr;
		pCurrentLevel->getNotRenderableContainers(pvNotRenderableContainers);
		for (size_t i = 0; i < pvNotRenderableContainers->size(); i++)
		{
			if (pvNotRenderableContainers->operator[](i)->isUserInputCallsEnabled())
			{
				pvNotRenderableContainers->operator[](i)->onMouseDown(mousekey, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			}
		}
		return 0;
	}
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	case WM_XBUTTONUP:
	{
		SMouseKey keyDownLeft(wParam);
		
		if (keyDownLeft.getButton() != pressedMouseKey.getButton())
		{
			onMouseUp(pressedMouseKey, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

			std::vector<SContainer*>* pvRenderableContainers = nullptr;
			pCurrentLevel->getRenderableContainers(pvRenderableContainers);

			for (size_t i = 0; i < pvRenderableContainers->size(); i++)
			{
				if (pvRenderableContainers->operator[](i)->isUserInputCallsEnabled())
				{
					pvRenderableContainers->operator[](i)->onMouseUp(pressedMouseKey, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
				}
			}

			std::vector<SContainer*>* pvNotRenderableContainers = nullptr;
			pCurrentLevel->getNotRenderableContainers(pvNotRenderableContainers);
			for (size_t i = 0; i < pvNotRenderableContainers->size(); i++)
			{
				if (pvNotRenderableContainers->operator[](i)->isUserInputCallsEnabled())
				{
					pvNotRenderableContainers->operator[](i)->onMouseUp(pressedMouseKey, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
				}
			}

			pressedMouseKey.setKey(SMB_NONE);
		}
		else
		{
			onMouseUp(keyDownLeft, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

			std::vector<SContainer*>* pvRenderableContainers = nullptr;
			pCurrentLevel->getRenderableContainers(pvRenderableContainers);

			for (size_t i = 0; i < pvRenderableContainers->size(); i++)
			{
				if (pvRenderableContainers->operator[](i)->isUserInputCallsEnabled())
				{
					pvRenderableContainers->operator[](i)->onMouseUp(keyDownLeft, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
				}
			}

			std::vector<SContainer*>* pvNotRenderableContainers = nullptr;
			pCurrentLevel->getNotRenderableContainers(pvNotRenderableContainers);
			for (size_t i = 0; i < pvNotRenderableContainers->size(); i++)
			{
				if (pvNotRenderableContainers->operator[](i)->isUserInputCallsEnabled())
				{
					pvNotRenderableContainers->operator[](i)->onMouseUp(keyDownLeft, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
				}
			}
		}
		
		return 0;
	}
	case WM_MOUSEMOVE:
	{
		if (bMouseCursorShown == false)
		{
			POINT pos;
			pos.x = fWindowCenterX;
			pos.y = fWindowCenterY;

			ClientToScreen(hMainWindow, &pos);

			SetCursorPos(pos.x, pos.y);
		}

		return 0;
	}
	case WM_INPUT:
	{
		UINT dataSize;

		GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, NULL, &dataSize, sizeof(RAWINPUTHEADER));

		if (dataSize > 0)
		{
			LPBYTE lpb = new BYTE[dataSize];

			if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, lpb, &dataSize, sizeof(RAWINPUTHEADER)) != dataSize)
			{
				SError::showErrorMessageBox(L"SApplication::msgProc::GetRawInputData()", L"GetRawInputData() does not return correct size.");
				return 0;
			}

			RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(lpb);



			// If you want to uncomment this then uncomment the keyboard stuff in createMainWindow().

			//if (raw->header.dwType == RIM_TYPEKEYBOARD) 
			//{
			//	hResult = StringCchPrintf(szTempOutput, STRSAFE_MAX_CCH, TEXT(" Kbd: make=%04x Flags:%04x Reserved:%04x ExtraInformation:%08x, msg=%04x VK=%04x \n"), 
			//		raw->data.keyboard.MakeCode, 
			//		raw->data.keyboard.Flags, 
			//		raw->data.keyboard.Reserved, 
			//		raw->data.keyboard.ExtraInformation, 
			//		raw->data.keyboard.Message, 
			//		raw->data.keyboard.VKey);
			//	if (FAILED(hResult))
			//	{
			//		// TODO: write error handler
			//	}
			//	OutputDebugString(szTempOutput);
			//}
			//else 
			
			if (raw->header.dwType == RIM_TYPEMOUSE) 
			{
				onMouseMove(raw->data.mouse.lLastX, raw->data.mouse.lLastY);

				std::vector<SContainer*>* pvRenderableContainers = nullptr;
				pCurrentLevel->getRenderableContainers(pvRenderableContainers);

				for (size_t i = 0; i < pvRenderableContainers->size(); i++)
				{
					if (pvRenderableContainers->operator[](i)->isUserInputCallsEnabled())
					{
						pvRenderableContainers->operator[](i)->onMouseMove(raw->data.mouse.lLastX, raw->data.mouse.lLastY);
					}
				}

				std::vector<SContainer*>* pvNotRenderableContainers = nullptr;
				pCurrentLevel->getNotRenderableContainers(pvNotRenderableContainers);
				for (size_t i = 0; i < pvNotRenderableContainers->size(); i++)
				{
					if (pvNotRenderableContainers->operator[](i)->isUserInputCallsEnabled())
					{
						pvNotRenderableContainers->operator[](i)->onMouseMove(raw->data.mouse.lLastX, raw->data.mouse.lLastY);
					}
				}
			} 


			delete[] lpb;
		}

		// Don't return, because we need to call DefWindowProc to cleanup.
	}
	case WM_MOUSEWHEEL:
	{
		int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);

		bool bUp = false;

		if (zDelta > 0)
		{
			bUp = true;
		}

		onMouseWheelMove(bUp, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

		std::vector<SContainer*>* pvRenderableContainers = nullptr;
		pCurrentLevel->getRenderableContainers(pvRenderableContainers);

		for (size_t i = 0; i < pvRenderableContainers->size(); i++)
		{
			if (pvRenderableContainers->operator[](i)->isUserInputCallsEnabled())
			{
				pvRenderableContainers->operator[](i)->onMouseWheelMove(bUp, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			}
		}

		std::vector<SContainer*>* pvNotRenderableContainers = nullptr;
		pCurrentLevel->getNotRenderableContainers(pvNotRenderableContainers);
		for (size_t i = 0; i < pvNotRenderableContainers->size(); i++)
		{
			if (pvNotRenderableContainers->operator[](i)->isUserInputCallsEnabled())
			{
				pvNotRenderableContainers->operator[](i)->onMouseWheelMove(bUp, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			}
		}
		return 0;
	}
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	{
		SKeyboardKey key(wParam, lParam);
		if (key.getButton() != SKB_NONE)
		{
			onKeyboardButtonDown(key);

			std::vector<SContainer*>* pvRenderableContainers = nullptr;
			pCurrentLevel->getRenderableContainers(pvRenderableContainers);

			for (size_t i = 0; i < pvRenderableContainers->size(); i++)
			{
				if (pvRenderableContainers->operator[](i)->isUserInputCallsEnabled())
				{
					pvRenderableContainers->operator[](i)->onKeyboardButtonDown(key);
				}
			}

			std::vector<SContainer*>* pvNotRenderableContainers = nullptr;
			pCurrentLevel->getNotRenderableContainers(pvNotRenderableContainers);
			for (size_t i = 0; i < pvNotRenderableContainers->size(); i++)
			{
				if (pvNotRenderableContainers->operator[](i)->isUserInputCallsEnabled())
				{
					pvNotRenderableContainers->operator[](i)->onKeyboardButtonDown(key);
				}
			}
		}
		return 0;
	}
	case WM_KEYUP:
	case WM_SYSKEYUP:
	{
		SKeyboardKey key(wParam, lParam);
		if (key.getButton() != SKB_NONE)
		{
			onKeyboardButtonUp(key);

			std::vector<SContainer*>* pvRenderableContainers = nullptr;
			pCurrentLevel->getRenderableContainers(pvRenderableContainers);

			for (size_t i = 0; i < pvRenderableContainers->size(); i++)
			{
				if (pvRenderableContainers->operator[](i)->isUserInputCallsEnabled())
				{
					pvRenderableContainers->operator[](i)->onKeyboardButtonUp(key);
				}
			}

			std::vector<SContainer*>* pvNotRenderableContainers = nullptr;
			pCurrentLevel->getNotRenderableContainers(pvNotRenderableContainers);
			for (size_t i = 0; i < pvNotRenderableContainers->size(); i++)
			{
				if (pvNotRenderableContainers->operator[](i)->isUserInputCallsEnabled())
				{
					pvNotRenderableContainers->operator[](i)->onKeyboardButtonUp(key);
				}
			}
		}
		return 0;
	}
	case WM_DESTROY:
	{
		onCloseEvent();

		if (bInitCalled)
		{
			flushCommandQueue();
		}
		
		PostQuitMessage(0);
		return 0;
	}
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

int SApplication::run()
{
	if (bInitCalled)
	{
		MSG msg = {0};

		gameTimer.reset();

		bRunCalled = true;

		STimer frameTimer;
		frameTimer.start();
		gameTimer.tick();

		update(); // so pCurrentFrameResource will be assigned before onTick()
		draw();

		onRun();

		while(msg.message != WM_QUIT)
		{
			if(PeekMessage( &msg, 0, 0, 0, PM_REMOVE ))
			{
				TranslateMessage( &msg );
				DispatchMessage( &msg );
			}
			else
			{
				gameTimer.tick();

				if (bCallTick)
				{
					onTick(gameTimer.getDeltaTimeBetweenFramesInSec());
				}

				std::vector<SContainer*>* pvRenderableContainers = nullptr;
				pCurrentLevel->getRenderableContainers(pvRenderableContainers);

				std::vector<SContainer*>* pvNotRenderableContainers = nullptr;
				pCurrentLevel->getNotRenderableContainers(pvNotRenderableContainers);

				for (size_t i = 0; i < pvRenderableContainers->size(); i++)
				{
					if (pvRenderableContainers->operator[](i)->getCallTick())
					{
						pvRenderableContainers->operator[](i)->onTick(gameTimer.getDeltaTimeBetweenFramesInSec());
					}
				}
				for (size_t i = 0; i < pvNotRenderableContainers->size(); i++)
				{
					if (pvNotRenderableContainers->operator[](i)->getCallTick())
					{
						pvNotRenderableContainers->operator[](i)->onTick(gameTimer.getDeltaTimeBetweenFramesInSec());
					}
				}


				update();
				draw();

				calculateFrameStats();


				if (fFPSLimit >= 1.0f)
				{
 					double dTimeToRenderFrameInMS = frameTimer.getElapsedTimeInMS();

					if (dDelayBetweenFramesInMS > dTimeToRenderFrameInMS)
					{
						Sleep(static_cast<unsigned long>(dDelayBetweenFramesInMS - dTimeToRenderFrameInMS));
					}

					frameTimer.start();
				}
			}
		}

		return static_cast<int>(msg.wParam);
	}
	else
	{
		MessageBox(0, L"An error occurred at SApplication::run(). Error: init() should be called first.", L"Error", 0);
		return 1;
	}
}

bool SApplication::minimizeWindow()
{
	if (pApp)
	{
		if (pApp->bRunCalled)
		{
			PostMessage(pApp->hMainWindow, WM_SYSCOMMAND, SC_MINIMIZE, 0);
			return false;
		}
		else
		{
			MessageBox(0, L"An error occurred at SApplication::minimizeWindow(). Error: run() should be called first.", L"Error", 0);
			return true;
		}
	}
	else
	{
		MessageBox(0, L"An error occurred at SApplication::minimizeWindow(). Error: an application instance is not created (pApp was nullptr).", L"Error", 0);
		return true;
	}
}

bool SApplication::maximizeWindow()
{
	if (pApp)
	{
		if (pApp->bRunCalled)
		{
			PostMessage(pApp->hMainWindow, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
			return false;
		}
		else
		{
			MessageBox(0, L"An error occurred at SApplication::maximizeWindow(). Error: run() should be called first.", L"Error", 0);
			return true;
		}
	}
	else
	{
		MessageBox(0, L"An error occurred at SApplication::maximizeWindow(). Error: an application instance is not created (pApp was nullptr).", L"Error", 0);
		return true;
	}
}

bool SApplication::restoreWindow()
{
	if (pApp)
	{
		if (pApp->bRunCalled)
		{
			//PostMessage(pApp->hMainWindow, WM_SYSCOMMAND, SC_RESTORE, 0);
			ShowWindow(pApp->hMainWindow, SW_RESTORE);
			return false;
		}
		else
		{
			MessageBox(0, L"An error occurred at SApplication::restoreWindow(). Error: run() should be called first.", L"Error", 0);
			return true;
		}
	}
	else
	{
		MessageBox(0, L"An error occurred at SApplication::restoreWindow(). Error: an application instance is not created (pApp was nullptr).", L"Error", 0);
		return true;
	}
}

bool SApplication::hideWindow()
{
	if (pApp)
	{
		if (pApp->bRunCalled)
		{
			pApp->onHideEvent();
			ShowWindow(pApp->hMainWindow, SW_HIDE);
			return false;
		}
		else
		{
			MessageBox(0, L"An error occurred at SApplication::hideWindow(). Error: run() should be called first.", L"Error", 0);
			return true;
		}
	}
	else
	{
		MessageBox(0, L"An error occurred at SApplication::hideWindow(). Error: an application instance is not created (pApp was nullptr).", L"Error", 0);
		return true;
	}
}

bool SApplication::showWindow()
{
	if (pApp)
	{
		if (pApp->bRunCalled)
		{
			pApp->onShowEvent();
			ShowWindow(pApp->hMainWindow, SW_SHOW);
			return false;
		}
		else
		{
			MessageBox(0, L"An error occurred at SApplication::showWindow(). Error: run() should be called first.", L"Error", 0);
			return true;
		}
	}
	else
	{
		MessageBox(0, L"An error occurred at SApplication::showWindow(). Error: an application instance is not created (pApp was nullptr).", L"Error", 0);
		return true;
	}
}
