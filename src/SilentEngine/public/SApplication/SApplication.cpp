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

#pragma comment(lib, "Winmm.lib")

// DirectXTK
#include "DDSTextureLoader.h"
#include "SimpleMath.h"
#include "ResourceUploadBatch.h"
#include "DirectXHelpers.h"

// Custom
#include "SilentEngine/Private/SError/SError.h"
#include "SilentEngine/Public/STimer/STimer.h"
#include "SilentEngine/Public/SPrimitiveShapeGenerator/SPrimitiveShapeGenerator.h"
#include "SilentEngine/Public/EntityComponentSystem/SContainer/SContainer.h"
#include "SilentEngine/Private/EntityComponentSystem/SComponent/SComponent.h"
#include "SilentEngine/Public/EntityComponentSystem/SMeshComponent/SMeshComponent.h"
#include "SilentEngine/Public/EntityComponentSystem/SRuntimeMeshComponent/SRuntimeMeshComponent.h"
#include "SilentEngine/Private/SShader/SShader.h"
#include "SilentEngine/Private/SMiscHelpers/SMiscHelpers.h"
#include "SilentEngine/Public/GUI/SGUISimpleText/SGUISimpleText.h"
#include "SilentEngine/Public/GUI/SGUIImage/SGUIImage.h"
#include "SilentEngine/Public/GUI/SGUILayout/SGUILayout.h"


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
			SError::showErrorMessageBoxAndLog("run() should be called first.");
			return true;
		}
	}
	else
	{
		SError::showErrorMessageBoxAndLog("an application instance is not created (pApp was nullptr).");
		return true;
	}
}

void SApplication::setGlobalVisualSettings(const SGlobalVisualSettings& settings)
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

	std::lock_guard<std::mutex> guard(mtxDraw);

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
		size_t iNewMaterialCBIndex = 0;

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
			flushCommandQueue();

			for (size_t i = 0; i < vRegisteredMaterials.size(); i++)
			{
				vRegisteredMaterials[i]->iUpdateCBInFrameResourceCount = SFRAME_RES_COUNT;
			}

			createCBVSRVUAVHeap();
			createViews();
		}

		return pMat;
	}
	else
	{
		bErrorOccurred = true;
		return nullptr;
	}
}

SMaterial* SApplication::getRegisteredMaterial(const std::string& sMaterialName)
{
	SMaterial* pMaterial = nullptr;

	std::lock_guard<std::mutex> guard(mtxDraw);

	for (size_t i = 0; i < vRegisteredMaterials.size(); i++)
	{
		if (vRegisteredMaterials[i]->getMaterialName() == sMaterialName)
		{
			pMaterial = vRegisteredMaterials[i];
			break;
		}
	}

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

	std::lock_guard<std::mutex> guard(mtxDraw);

	for (size_t i = 0; i < vRegisteredMaterials.size(); i++)
	{
		if (vRegisteredMaterials[i]->getMaterialName() == sMaterialName)
		{
			if (vRegisteredMaterials[i]->bUsedInBundle == false)
			{
				bRegistered = true;
				break;
			}
		}
	}

	if (bRegistered == false)
	{
		return true;
	}


	// Remove material.
	// Find if any spawned object is using this material.

	std::vector<SComponent*> vAllSpawnedMeshComponents = vAllRenderableSpawnedOpaqueComponents;
	vAllSpawnedMeshComponents.insert(vAllSpawnedMeshComponents.end(), vAllRenderableSpawnedTransparentComponents.begin(),
		vAllRenderableSpawnedTransparentComponents.end());

	for (size_t i = 0; i < vAllSpawnedMeshComponents.size(); i++)
	{
		if (vAllSpawnedMeshComponents[i]->meshData.getMeshMaterial())
		{
			// Not the default material.
			if (vAllSpawnedMeshComponents[i]->meshData.getMeshMaterial()->getMaterialName() == sMaterialName)
			{
				if (vAllSpawnedMeshComponents[i]->componentType == SComponentType::SCT_MESH)
				{
					dynamic_cast<SMeshComponent*>(vAllSpawnedMeshComponents[i])->unbindMaterial();
				}
				else if (vAllSpawnedMeshComponents[i]->componentType == SComponentType::SCT_RUNTIME_MESH)
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
		createCBVSRVUAVHeap();
		createViews();
	}

	return false;
}

bool SApplication::unregisterGUIObject(SGUIObject* pGUIObject)
{
	if (pGUIObject == nullptr)
	{
		return true;
	}

	if (pGUIObject->bIsRegistered == false)
	{
		return true;
	}

//#if defined(DEBUG) || defined(_DEBUG)
//	if (pGUIObject->bIsSystemObject)
//	{
//		return true;
//	}
//#endif

	if (pGUIObject->objectType == SGUIType::SGT_LAYOUT)
	{
		SGUILayout* pLayout = dynamic_cast<SGUILayout*>(pGUIObject);
		
		if (pLayout->getChilds()->size() != 0)
		{
			SError::showErrorMessageBoxAndLog("can't unregister a layout with childs, remove all childs from this layout first, then unregister the layout.");
			return true;
		}
	}

	if (pGUIObject->layoutData.pLayout)
	{
		SError::showErrorMessageBoxAndLog("can't unregister an object that is in a layout, remove the object from the layout first, and only then unregister the object.");
		return true;
	}

	std::lock_guard<std::mutex> guard(mtxDraw);

	// Check if this GUI object is loaded.
	bool bFound = false;

#if defined(DEBUG) || defined(_DEBUG)
	if (pGUIObject->objectType == SGUIType::SGT_LAYOUT)
	{
		SGUILayout* pLayout = dynamic_cast<SGUILayout*>(pGUIObject);

		for (size_t i = 0; i < vGUILayers.size(); i++)
		{
			for (size_t j = 0; j < vGUILayers[i].vGUIObjects.size(); j++)
			{
				if (vGUILayers[i].vGUIObjects[j] == pLayout->pDebugLayoutFillImage)
				{
					delete vGUILayers[i].vGUIObjects[j];
					vGUILayers[i].vGUIObjects.erase(vGUILayers[i].vGUIObjects.begin() + i);
					bFound = true;

					if (vGUILayers[i].vGUIObjects.size() == 0 && i != 0)
					{
						vGUILayers.erase(vGUILayers.begin() + i);
					}

					break;
				}
			}

			if (bFound)
			{
				break;
			}
		}

		if (bFound == false)
		{
			return true;
		}
	}
#endif

	bFound = false;

	for (size_t i = 0; i < vGUILayers.size(); i++)
	{
		for (size_t j = 0; j < vGUILayers[i].vGUIObjects.size(); j++)
		{
			if (vGUILayers[i].vGUIObjects[j] == pGUIObject)
			{
				delete vGUILayers[i].vGUIObjects[j];
				vGUILayers[i].vGUIObjects.erase(vGUILayers[i].vGUIObjects.begin() + i);
				bFound = true;

				if (vGUILayers[i].vGUIObjects.size() == 0 && i != 0)
				{
					vGUILayers.erase(vGUILayers.begin() + i);
				}

				break;
			}
		}

		if (bFound)
		{
			break;
		}
	}
	
	if (bFound == false)
	{
		return true;
	}

	// Remove the SRV to this texture.

	flushCommandQueue();

	// Recreate cbv heap.
	createCBVSRVUAVHeap();
	createViews();

	return false;
}

void SApplication::registerGUIObject(SGUIObject* pGUIObject, bool bWillBeUsedInLayout)
{
	if (pGUIObject->bIsRegistered)
	{
		return;
	}

	if (pGUIObject->checkRequiredResourcesBeforeRegister())
	{
		return;
	}

	if (bWillBeUsedInLayout == false && (pGUIObject->vSizeToKeep.getX() < 0.0f || pGUIObject->vSizeToKeep.getY() < 0.0f))
	{
		SError::showErrorMessageBoxAndLog("you need to specify the size to keep using setSizeToKeep().");
		return;
	}

	{
		std::lock_guard<std::mutex> guard(mtxDraw);

		pGUIObject->bIsRegistered = true;
		pGUIObject->bIsVisible = false;
		pGUIObject->bToBeUsedInLayout = bWillBeUsedInLayout;

		// First entry with layer index 0 always exists.
		vGUILayers[0].vGUIObjects.push_back(pGUIObject);


		// Add the SRVs to this object.

		flushCommandQueue();

		createCBVSRVUAVHeap();
		createViews();


		if (pGUIObject->objectType == SGUIType::SGT_SIMPLE_TEXT)
		{
			SGUISimpleText* pText = dynamic_cast<SGUISimpleText*>(pGUIObject);
			pText->initFontResource();
		}

		if (bWillBeUsedInLayout)
		{
			pGUIObject->pos = DirectX::XMFLOAT2(0.5f, 0.5f);
			pGUIObject->vSizeToKeep = SVector(1.0f, 1.0f);
			pGUIObject->scale = DirectX::XMFLOAT2(1.0f, 1.0f);
			pGUIObject->screenScale = DirectX::XMFLOAT2(1.0f, 1.0f);
		}
	}

	if (pGUIObject->iZLayer != 0)
	{
		// locks mtxDraw
		moveGUIObjectToLayer(pGUIObject, pGUIObject->iZLayer);
	}

#if defined(DEBUG) || defined(_DEBUG)
	if (pGUIObject->objectType == SGUIType::SGT_LAYOUT)
	{
		SGUILayout* pLayout = dynamic_cast<SGUILayout*>(pGUIObject);
		registerGUIObject(pLayout->pDebugLayoutFillImage, false);
		if (pGUIObject->iZLayer != 0)
		{
			// locks mtxDraw
			moveGUIObjectToLayer(pLayout->pDebugLayoutFillImage, pGUIObject->iZLayer);
		}
	}
#endif
}

std::vector<SGUILayer>* SApplication::getLoadedGUIObjects()
{
	return &vGUILayers;
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

	std::lock_guard<std::mutex> guard(mtxDraw);

	for (size_t i = 0; i < vLoadedTextures.size(); i++)
	{
		if (vLoadedTextures[i]->sTextureName == sTextureName)
		{
			bErrorOccurred = true;

			return STextureHandle();
		}
	}




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

	if (fs::path(sPathToTexture).extension().string() != ".dds")
	{
		bErrorOccurred = true;

		return STextureHandle();
	}



	// Load texture.

	STextureInternal* pTexture = new STextureInternal();
	pTexture->sTextureName = sTextureName;
	pTexture->sPathToTexture = sPathToTexture;

	/*flushCommandQueue();
	resetCommandList();*/
	
	DirectX::ResourceUploadBatch resourceUpload(pDevice.Get());
	resourceUpload.Begin();
	HRESULT hresult = DirectX::CreateDDSTextureFromFile(pDevice.Get(), resourceUpload, sPathToTexture.c_str(), pTexture->pResource.ReleaseAndGetAddressOf());
	//HRESULT hresult = DirectX::CreateDDSTextureFromFile12(pDevice.Get(), pCommandList.Get(), sPathToTexture.c_str(),
	//	pTexture->pResource, pTexture->pUploadHeap);

	if (FAILED(hresult))
	{
		SError::showErrorMessageBoxAndLog(hresult);

		bErrorOccurred = true;
		
		delete pTexture;

		return STextureHandle();
	}

	auto uploadResourcesFinished = resourceUpload.End(pCommandQueue.Get());

	uploadResourcesFinished.wait();

	/*if (executeCommandList())
	{
		bErrorOccurred = true;

		delete pTexture;

		return STextureHandle();
	}

	if (flushCommandQueue())
	{
		bErrorOccurred = true;

		delete pTexture;

		return STextureHandle();
	}*/



	// Check if the texture size is x4.

	if (pTexture->pResource->GetDesc().Width % 4 != 0   || pTexture->pResource->GetDesc().Height % 4 != 0
		|| pTexture->pResource->GetDesc().Width != pTexture->pResource->GetDesc().Height)
	{
		SError::showErrorMessageBoxAndLog("the texture size should be a multiple of 4.");
		
		delete pTexture;

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

	flushCommandQueue();

	// Recreate cbv heap.
	createCBVSRVUAVHeap();
	createViews();



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

	std::lock_guard<std::mutex> guard(mtxDraw);

	for (size_t i = 0; i < vLoadedTextures.size(); i++)
	{
		STextureHandle tex;
		tex.bRegistered = true;
		tex.sTextureName = vLoadedTextures[i]->sTextureName;
		tex.sPathToTexture = vLoadedTextures[i]->sPathToTexture;
		tex.pRefToTexture = vLoadedTextures[i];

		vTextures.push_back(tex);
	}

	return vTextures;
}

bool SApplication::unloadTextureFromGPU(STextureHandle& textureHandle)
{
	if (textureHandle.bRegistered == false)
	{
		return true;
	}


	std::lock_guard<std::mutex> guard(mtxDraw);

	// Find if any spawned object is using a material with this texture.

	std::vector<SComponent*> vAllSpawnedMeshComponents = vAllRenderableSpawnedOpaqueComponents;
	vAllSpawnedMeshComponents.insert(vAllSpawnedMeshComponents.end(), vAllRenderableSpawnedTransparentComponents.begin(),
		vAllRenderableSpawnedTransparentComponents.end());

	for (size_t i = 0; i < vAllSpawnedMeshComponents.size(); i++)
	{
		if (vAllSpawnedMeshComponents[i]->meshData.getMeshMaterial())
		{
			// Not the default material.
			SMaterialProperties matProps = vAllSpawnedMeshComponents[i]->meshData.getMeshMaterial()->getMaterialProperties();
			STextureHandle texHandle;
			if (matProps.getDiffuseTexture(&texHandle) == false)
			{
				if (texHandle.getTextureName() == textureHandle.getTextureName())
				{
					if (vAllSpawnedMeshComponents[i]->componentType == SComponentType::SCT_MESH)
					{
						dynamic_cast<SMeshComponent*>(vAllSpawnedMeshComponents[i])->unbindMaterial();
					}
					else if (vAllSpawnedMeshComponents[i]->componentType == SComponentType::SCT_RUNTIME_MESH)
					{
						dynamic_cast<SRuntimeMeshComponent*>(vAllSpawnedMeshComponents[i])->unbindMaterial();
					}
				}
			}

			// ADD OTHER TEXTURES HERE
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
				SError::showErrorMessageBoxAndLog("texture ref count is not 0.");
			}

			delete vLoadedTextures[i];
			vLoadedTextures.erase(vLoadedTextures.begin() + i);

			break;
		}
	}



	// Remove the SRV to this texture.

	flushCommandQueue();

	// Recreate cbv heap.
	createCBVSRVUAVHeap();
	createViews();
	

	return false;
}

SShader* SApplication::compileCustomShader(const std::wstring& sPathToShaderFile, 
	const SCustomShaderProperties& customProps, SCustomShaderResources** pOutCustomResources)
{
	// See if the file exists.

	std::ifstream shaderFile(sPathToShaderFile);

	if (shaderFile.is_open() == false)
	{
		SError::showErrorMessageBoxAndLog("could not open the shader file.");
		return nullptr;
	}
	else
	{
		shaderFile.close();
	}



	// See if the file format is .hlsl.

	if (fs::path(sPathToShaderFile).extension().string() != ".hlsl")
	{
		return nullptr;
	}


	std::vector<SMaterial*> vCustomMaterials;
	if (customProps.customMaterials.vCustomMaterialNames.size() > 0)
	{
		bool bError = false;

		for (size_t i = 0; i < customProps.customMaterials.vCustomMaterialNames.size(); i++)
		{
			vCustomMaterials.push_back(registerMaterialBundleElement(customProps.customMaterials.vCustomMaterialNames[i], bError));

			if (bError)
			{
				// last vMaterial element is nullptr
				if (vCustomMaterials.size() > 1)
				{
					for (size_t i = 0; i < vCustomMaterials.size() - 1; i++)
					{
						delete vCustomMaterials[i];
					}
				}

				vCustomMaterials.clear();

				return nullptr;
			}
		}
	}


	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		"ALPHA_TEST", "1",
		NULL, NULL
	};


	SShader* pNewShader = new SShader(sPathToShaderFile);

	std::lock_guard<std::mutex> guard(mtxDraw);
	
	if (vCustomMaterials.size() > 0)
	{
		pNewShader->pCustomShaderResources = new SCustomShaderResources();
		pNewShader->pCustomShaderResources->vMaterials = vCustomMaterials;
		pNewShader->pCustomShaderResources->bUsingInstancing = customProps.bWillUseInstancing;

		createRootSignature(pNewShader->pCustomShaderResources, customProps.customMaterials.bWillUseTextures, customProps.bWillUseInstancing);

		pNewShader->pCustomShaderResources->vFrameResourceBundles =
			createBundledMaterialResource(pNewShader, pNewShader->pCustomShaderResources->vMaterials.size());

		if (pOutCustomResources)
		{
			*pOutCustomResources = pNewShader->pCustomShaderResources;
		}
	}
	else if (customProps.bWillUseInstancing)
	{
		pNewShader->pCustomShaderResources = new SCustomShaderResources();
		pNewShader->pCustomShaderResources->bUsingInstancing = customProps.bWillUseInstancing;

		createRootSignature(pNewShader->pCustomShaderResources, customProps.customMaterials.bWillUseTextures, customProps.bWillUseInstancing);

		if (pOutCustomResources)
		{
			*pOutCustomResources = pNewShader->pCustomShaderResources;
		}
	}

	pNewShader->pVS = SMiscHelpers::compileShader(sPathToShaderFile, nullptr, L"VS", SE_VS_SM, bCompileShadersInRelease);
	pNewShader->pPS = SMiscHelpers::compileShader(sPathToShaderFile, nullptr, L"PS", SE_PS_SM, bCompileShadersInRelease);
	pNewShader->pAlphaPS = SMiscHelpers::compileShader(sPathToShaderFile, alphaTestDefines, L"PS", SE_PS_SM, bCompileShadersInRelease);


	if (createPSO(pNewShader))
	{
		releaseShader(pNewShader);

		return nullptr;
	}

	vCompiledUserShaders.push_back(pNewShader);

	return pNewShader;
}

std::vector<SShader*>* SApplication::getCompiledCustomShaders()
{
	return &vCompiledUserShaders;
}

bool SApplication::unloadCompiledShaderFromGPU(SShader* pShader)
{
	if (pShader == nullptr) return false;

	std::lock_guard<std::mutex> guard(mtxDraw);

	flushCommandQueue();


	removeShaderFromObjects(pShader, &vOpaqueMeshesByCustomShader);
	removeShaderFromObjects(pShader, &vTransparentMeshesByCustomShader);


	bool bFound = false;

	for (size_t i = 0; i < vCompiledUserShaders.size(); i++)
	{
		if (vCompiledUserShaders[i] == pShader)
		{
			releaseShader(pShader);

			vCompiledUserShaders.erase(vCompiledUserShaders.begin() + i);

			bFound = true;
			break;
		}
	}

	if (bFound)
	{
		return false;
	}
	else
	{
		return true;
	}
}

SComputeShader* SApplication::registerCustomComputeShader(const std::string& sUniqueShaderName)
{
	for (size_t i = 0; i < vUserComputeShaders.size(); i++)
	{
		if (vUserComputeShaders[i]->sComputeShaderName == sUniqueShaderName)
		{
			return nullptr;
		}
	}

	SComputeShader* pNewComputeShader = new SComputeShader(pDevice.Get(), pCommandList.Get(), bCompileShadersInRelease, sUniqueShaderName);

	std::lock_guard<std::mutex> guard(mtxDraw);
	vUserComputeShaders.push_back(pNewComputeShader);

	return pNewComputeShader;
}

std::vector<SComputeShader*>* SApplication::getRegisteredComputeShaders()
{
	return &vUserComputeShaders;
}

void SApplication::unregisterCustomComputeShader(SComputeShader* pComputeShader)
{
	if (pComputeShader->bCopyingComputeResult)
	{
		// will cause deadlock (we are in the draw() right now)
		SError::showErrorMessageBoxAndLog("cannot unregister the compute shader while we are in the copyComputeResults() function.");
		return;
	}

	std::lock_guard<std::mutex> guard(mtxDraw);

	for (size_t i = 0; i < vUserComputeShaders.size(); i++)
	{
		if (vUserComputeShaders[i] == pComputeShader)
		{
			flushCommandQueue();

			delete pComputeShader;

			vUserComputeShaders.erase(vUserComputeShaders.begin() + i);

			break;
		}
	}
}

SLevel* SApplication::getCurrentLevel() const
{
	return pCurrentLevel;
}

void SApplication::setDisableKeyboardRepeat(bool bDisable)
{
	bDisableKeyboardRepeat = bDisable;
}

bool SApplication::spawnContainerInLevel(SContainer* pContainer)
{
	if (pContainer->bSpawnedInLevel)
	{
		return true;
	}

	std::lock_guard<std::mutex> guard(mtxDraw);

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
		SError::showErrorMessageBoxAndLog("exceeded MAX_LIGHTS (this container was not spawned).");
		return true;
	}

	// Add lights.
	for (size_t i = 0; i < pContainer->vComponents.size(); i++)
	{
		pContainer->vComponents[i]->addLightComponentsToVector(getCurrentLevel()->vSpawnedLightComponents);
	}



	// We need 1 CB for each SCT_MESH, SCT_RUNTIME_MESH component.
	size_t iCBCount = pContainer->getMeshComponentsCount();

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
		flushCommandQueue();

		iActualObjectCBCount += iCBCount;

		bool bExpanded = false;
		size_t iNewObjectsCBIndex = 0;

		for (size_t i = 0; i < vFrameResources.size(); i++)
		{
			iNewObjectsCBIndex = vFrameResources[i]->addNewObjectCB(iCBCount, &bExpanded);

			pContainer->createVertexBufferForRuntimeMeshComponents(vFrameResources[i].get());
		}

		pContainer->setStartIndexInCB(iNewObjectsCBIndex);



		// Allocate instanced data (if using instancing)
		pContainer->createInstancingDataForFrameResource(&vFrameResources);



		resetCommandList();

		for (size_t i = 0; i < pContainer->vComponents.size(); i++)
		{
			pContainer->vComponents[i]->setCBIndexForMeshComponents(&iNewObjectsCBIndex);
		}

		if (executeCommandList())
		{
			return true;
		}

		if (flushCommandQueue())
		{
			return true;
		}


		std::vector<SContainer*>* pvRenderableContainers = nullptr;
		pCurrentLevel->getRenderableContainers(pvRenderableContainers);

		pvRenderableContainers->push_back(pContainer);

		vAllRenderableSpawnedContainers.push_back(pContainer);


		pContainer->getAllMeshComponents(&vAllRenderableSpawnedOpaqueComponents,
			&vAllRenderableSpawnedTransparentComponents);

		pContainer->addMeshesByShader(&vOpaqueMeshesByCustomShader, &vTransparentMeshesByCustomShader);


		pContainer->registerAll3DSoundComponents();


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
			// (not using cbv for SObjectConstants - don't recreate heap)
			//createCBVSRVUAVHeap();
			//createViews();
		}
	}

	pContainer->setSpawnedInLevel(true);

	return false;
}

void SApplication::despawnContainerFromLevel(SContainer* pContainer)
{
	if (pContainer->bSpawnedInLevel == false)
	{
		return;
	}
	
	std::lock_guard<std::mutex> guard(mtxDraw);

	// Remove lights.
	for (size_t i = 0; i < pContainer->vComponents.size(); i++)
	{
		pContainer->vComponents[i]->removeLightComponentsFromVector(getCurrentLevel()->vSpawnedLightComponents);
	}

	// We need 1 for each SCT_MESH, SCT_RUNTIME_MESH component.
	size_t iCBCount = pContainer->getMeshComponentsCount();

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

		// Deallocate instanced data (if using instancing)
		pContainer->removeInstancingDataForFrameResources(&vFrameResources);

		
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

		pContainer->removeMeshesByShader(&vOpaqueMeshesByCustomShader, &vTransparentMeshesByCustomShader);


		pContainer->unregisterAll3DSoundComponents();


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
			// (not using cbv for SObjectConstants - don't recreate heap)
			//createCBVSRVUAVHeap();
			//createViews();
		}
	}

	pContainer->setSpawnedInLevel(false);

	if (bExitCalled)
	{
		delete pContainer;
	}
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
		SError::showErrorMessageBoxAndLog("this function should be called before init() call.");
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
		SError::showErrorMessageBoxAndLog("this function should be called before init() call.");
		return true;
	}
}

bool SApplication::setInitFullscreen(bool bFullscreen)
{
	if (bInitCalled == false)
	{
		bHideTitleBar = bFullscreen;
		this->bFullscreen = bFullscreen;
		return false;
	}
	else
	{
		SError::showErrorMessageBoxAndLog("this function should be called before init() call.");
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
		SError::showErrorMessageBoxAndLog("this function should be called before init() call.");
		return true;
	}
}

bool SApplication::setInitPhysicsTicksPerSecond(int iTicksPerSecond)
{
	if (bInitCalled == false)
	{
		if (iTicksPerSecond <= 0)
		{
			SError::showErrorMessageBoxAndLog("iTicksPerSecond can't be 0 or negative.");
			return true;
		}
		else if (iTicksPerSecond > 500)
		{
			SError::showErrorMessageBoxAndLog("iTicksPerSecond can't be bigger than 500.");
			return true;
		}

		this->iPhysicsTicksPerSecond = iTicksPerSecond;

		return false;
	}
	else
	{
		SError::showErrorMessageBoxAndLog("this function should be called before init().");
		return true;
	}
}

void SApplication::setBackBufferFillColor(const SVector& vColor)
{
	backBufferFillColor[0] = vColor.getX();
	backBufferFillColor[1] = vColor.getY();
	backBufferFillColor[2] = vColor.getZ();
}

void SApplication::setEnableWireframeMode(bool bEnable)
{
	std::lock_guard<std::mutex> guard(mtxDraw);

	bUseFillModeWireframe = bEnable;
}

void SApplication::setMSAAEnabled(bool bEnable)
{
	if (MSAA_Enabled != bEnable)
	{
		MSAA_Enabled = bEnable;

		if (bInitCalled)
		{
			std::lock_guard<std::mutex> guard(mtxDraw);

			createPSO();
			onResize();

			for (size_t i = 0; i < vGUILayers.size(); i++)
			{
				for (size_t j = 0; j < vGUILayers[i].vGUIObjects.size(); j++)
				{
					vGUILayers[i].vGUIObjects[j]->onMSAAChange();
				}
			}
		}
	}
}

bool SApplication::setMSAASampleCount(MSAASampleCount eSampleCount)
{
	if (pDevice)
	{
		if (MSAA_SampleCount != static_cast<std::underlying_type<MSAASampleCount>::type>(eSampleCount))
		{
			MSAA_SampleCount = static_cast<std::underlying_type<MSAASampleCount>::type>(eSampleCount);

			if (checkMSAASupport())
			{
				return true;
			}

			if (MSAA_Enabled && bInitCalled)
			{
				std::lock_guard<std::mutex> guard(mtxDraw);

				createPSO();
				onResize();

				for (size_t i = 0; i < vGUILayers.size(); i++)
				{
					for (size_t j = 0; j < vGUILayers[i].vGUIObjects.size(); j++)
					{
						vGUILayers[i].vGUIObjects[j]->onMSAAChange();
					}
				}
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
	MSAASampleCount sampleCount = MSAASampleCount::SC_2;

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
//
//bool SApplication::setFullscreen(bool bFullscreen)
//{
//	if (bInitCalled)
//	{
//		if (this->bFullscreen != bFullscreen)
//		{
//			std::lock_guard<std::mutex> guard(mtxDraw);
//
//			this->bFullscreen = bFullscreen;
//
//			HRESULT hresult;
//
//			if (bFullscreen)
//			{
//				// ?
//				//hresult = pSwapChain->SetFullscreenState(bFullscreen, pOutput.Get());
//			}
//			else
//			{
//				// From docs: "pTarget - If you pass FALSE to Fullscreen, you must set this parameter to NULL."
//				//hresult = pSwapChain->SetFullscreenState(bFullscreen, NULL);
//			}
//
//			if (FAILED(hresult))
//			{
//				SError::showErrorMessageBoxAndLog(hresult);
//				return true;
//			}
//			else
//			{
//				// Resize the buffers.
//				onResize();
//			}
//		}
//
//		return false;
//	}
//	else
//	{
//		SError::showErrorMessageBoxAndLog("init() should be called first.");
//		return true;
//	}
//}

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

			
			std::lock_guard<std::mutex> guard(mtxDraw);

			flushCommandQueue();

			if (bFullscreen == false)
			{
				pSwapChain->ResizeTarget(&desc); // resize window
			}

			// Resize the buffers.
			onResize();

			if (bFullscreen == false)
			{
				// Set window to center.
				RECT rc;

				GetWindowRect(hMainWindow, &rc);

				int xPos = (GetSystemMetrics(SM_CXSCREEN) - rc.right) / 2;
				int yPos = (GetSystemMetrics(SM_CYSCREEN) - rc.bottom) / 2;

				SetWindowPos(hMainWindow, 0, xPos, yPos, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
			}
		}
		
		return false;
	}
	else
	{
		SError::showErrorMessageBoxAndLog("init() should be called first.");
		return true;
	}
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
			pos.x = static_cast<LONG>(vPos.getX());
			pos.y = static_cast<LONG>(vPos.getY());

			if (ClientToScreen(hMainWindow, &pos) == 0)
			{
				SError::showErrorMessageBoxAndLog(std::to_string(GetLastError()));
				return true;
			}

			if (SetCursorPos(pos.x, pos.y) == 0)
			{
				SError::showErrorMessageBoxAndLog(std::to_string(GetLastError()));
				return true;
			}

			return false;
		}
		else
		{
			SError::showErrorMessageBoxAndLog("the cursor is hidden.");
			return true;
		}
	}
	else
	{
		SError::showErrorMessageBoxAndLog("init() should be called first.");
		return true;
	}
}

void SApplication::setFPSLimit(float fFPSLimit)
{
	if (fFPSLimit <= 0.1f)
	{
		this->fFPSLimit = 0.0f;
		dDelayBetweenFramesInNS = 0.0;
	}
	else
	{
		this->fFPSLimit = fFPSLimit;
		dDelayBetweenFramesInNS = 1000000000.0 / fFPSLimit;
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

SAudioEngine* SApplication::getAudioEngine()
{
	return pAudioEngine;
}

SApplication* SApplication::getApp()
{
	return pApp;
}

bool SApplication::getCursorPos(SVector& vPos)
{
	if (bInitCalled)
	{
		if (bMouseCursorShown == false)
		{
			return true;
		}

		POINT pos;

		if (GetCursorPos(&pos) == 0)
		{
			SError::showErrorMessageBoxAndLog(std::to_string(GetLastError()));
			return true;
		}

		if (ScreenToClient(hMainWindow, &pos) == 0)
		{
			SError::showErrorMessageBoxAndLog(std::to_string(GetLastError()));
			return true;
		}

		if (bFullscreen)
		{
			// maybe running lower resolution
			// in this case, the window size will have a native resolution and not (iMainWindowWidth, iMainWindowHeight)
			RECT rect;
			int iWindowWidth = 0;
			int iWindowHeight = 0;
			if (GetWindowRect(hMainWindow, &rect))
			{
				iWindowWidth = rect.right - rect.left;
				iWindowHeight = rect.bottom - rect.top;
			}
			else
			{
				return true;
			}

			vPos.setX(static_cast<float>(pos.x) / iWindowWidth);
			vPos.setY(static_cast<float>(pos.y) / iWindowHeight);
		}
		else
		{
			vPos.setX(static_cast<float>(pos.x) / iMainWindowWidth);
			vPos.setY(static_cast<float>(pos.y) / iMainWindowHeight);
		}

		return false;
	}
	else
	{
		SError::showErrorMessageBoxAndLog("init() shound be called first.");
		return true;
	}
}

bool SApplication::getCursor3DPosAndDir(SVector& vPos, SVector& vDir)
{
	if (bRunCalled == false)
	{
		SError::showErrorMessageBoxAndLog("run() shound be called first.");
		return true;
	}

	if (bMouseCursorShown == false)
	{
		return true;
	}

	DirectX::XMFLOAT4X4 proj = SMath::getIdentityMatrix4x4();
	DirectX::XMStoreFloat4x4(&proj, camera.getProjMatrix());

	SVector vCursorPos;
	getCursorPos(vCursorPos);

	float fXPosViewSpace = (2.0f * vCursorPos.getX()) / proj(0, 0);
	float fYPosViewSpace = (-2.0f * vCursorPos.getY()) / proj(1, 1);

	DirectX::XMVECTOR vRayDirViewSpace = DirectX::XMVectorSet(fXPosViewSpace, fYPosViewSpace, 1.0f, 0.0f);

	auto det = DirectX::XMMatrixDeterminant(camera.getViewMatrix());

	// Direction to world space.
	DirectX::XMFLOAT3 vRayDirWorldSpaceFloat3;
	DirectX::XMVECTOR vRayDirWorldSpace = DirectX::XMVector3TransformCoord(vRayDirViewSpace, DirectX::XMMatrixInverse(&det, camera.getViewMatrix()));
	vRayDirWorldSpace = DirectX::XMVector3Normalize(vRayDirWorldSpace);
	DirectX::XMStoreFloat3(&vRayDirWorldSpaceFloat3, vRayDirWorldSpace);

	vPos = camera.getCameraLocationInWorld();
	vDir = SVector(vRayDirWorldSpaceFloat3.x, vRayDirWorldSpaceFloat3.y, vRayDirWorldSpaceFloat3.z);

	return false;
}

bool SApplication::getWindowSize(SVector& vSize)
{
	if (bInitCalled)
	{
		vSize.setX(static_cast<float>(iMainWindowWidth));
		vSize.setY(static_cast<float>(iMainWindowHeight));

		return false;
	}
	else
	{
		SError::showErrorMessageBoxAndLog("init() should be called first.");
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

void SApplication::openInternetURL(const std::wstring& sURL)
{
	ShellExecute(NULL, L"open", sURL.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

void SApplication::makeOneCopyOfScreenPixelsToCustomBuffer(unsigned char* pPixels)
{
	bSaveBackBufferPixelsForUser = true;

	this->pPixels = pPixels;
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
			SError::showErrorMessageBoxAndLog(hresult);
			return true;
		}

		std::vector<DXGI_MODE_DESC> vDisplayModes(numModes);

		hresult = pOutput->GetDisplayModeList(BackBufferFormat, 0, &numModes, &vDisplayModes[0]);
		if (FAILED(hresult))
		{
			SError::showErrorMessageBoxAndLog(hresult);
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
			SError::showErrorMessageBoxAndLog(hresult);
			return 0.0f;
		}
		else
		{
			return desc.RefreshRate.Numerator / static_cast<float>(desc.RefreshRate.Denominator);
		}
	}
	else
	{
		SError::showErrorMessageBoxAndLog("init() should be called first.");
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
			SError::showErrorMessageBoxAndLog(hresult);
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
		SError::showErrorMessageBoxAndLog("init() should be called first.");
		return true;
	}
}

bool SApplication::isFullscreen() const
{
	return bFullscreen;
}

SCamera* SApplication::getCamera()
{
	return &camera;
}

SVector SApplication::getBackBufferFillColor() const
{
	return SVector(backBufferFillColor[0], backBufferFillColor[1], backBufferFillColor[2]);
}

bool SApplication::isWireframeModeEnabled() const
{
	return bUseFillModeWireframe;
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
		SError::showErrorMessageBoxAndLog("run() should be called first.");
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
		SError::showErrorMessageBoxAndLog("run() should be called first.");
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
		SError::showErrorMessageBoxAndLog("run() should be called first.");
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
		SError::showErrorMessageBoxAndLog("run() should be called first.");
		return true;
	}
}

void SApplication::setDrawGUI(bool bDraw)
{
	bDrawGUI = bDraw;
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
			SError::showErrorMessageBoxAndLog(hresult);
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
			SError::showErrorMessageBoxAndLog(hresult);
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
				SError::showErrorMessageBoxAndLog(hresult);
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

		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
		hresult = pDevice->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&msaaRenderTargetDesc,
			D3D12_RESOURCE_STATE_COMMON,
			&msaaClear,
			IID_PPV_ARGS(pMSAARenderTarget.GetAddressOf()));
		if (FAILED(hresult))
		{
			SError::showErrorMessageBoxAndLog(hresult);
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

		heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

		hresult = pDevice->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&depthStencilDesc,
			D3D12_RESOURCE_STATE_COMMON,
			&optClear,
			IID_PPV_ARGS(pDepthStencilBuffer.GetAddressOf()));
		if (FAILED(hresult))
		{
			SError::showErrorMessageBoxAndLog(hresult);
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

		CD3DX12_RESOURCE_BARRIER transition =
			CD3DX12_RESOURCE_BARRIER::Transition(pDepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		pCommandList->ResourceBarrier(1, &transition);



		// Execute the resize commands.

		hresult = pCommandList->Close();
		if (FAILED(hresult))
		{
			SError::showErrorMessageBoxAndLog(hresult);
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
		ScreenViewport.MinDepth = fMinDepth;
		ScreenViewport.MaxDepth = fMaxDepth;

		ScissorRect = { 0, 0, iMainWindowWidth, iMainWindowHeight };


		for (size_t i = 0; i < vGUILayers.size(); i++)
		{
			for (size_t j = 0; j < vGUILayers[i].vGUIObjects.size(); j++)
			{
				vGUILayers[i].vGUIObjects[j]->setViewport(ScreenViewport);
			}
		}


		camera.setCameraAspectRatio(static_cast<float>(iMainWindowWidth) / iMainWindowHeight);
		camera.updateViewMatrix();



		// Update blur.

		if (pBlurEffect)
		{
			pBlurEffect->resizeResources(iMainWindowWidth, iMainWindowHeight);
		}


		return false;
	}
	else
	{
		SError::showErrorMessageBoxAndLog("init() should be called first.");

		return true;
	}
}

void SApplication::update()
{
	if (iCurrentFrameResourceIndex + 1 == iFrameResourcesCount)
	{
		iCurrentFrameResourceIndex = 0;
	}
	else
	{
		iCurrentFrameResourceIndex++;
	}

	pCurrentFrameResource = vFrameResources[iCurrentFrameResourceIndex].get();


	std::chrono::time_point<std::chrono::steady_clock> timeInSleep = std::chrono::steady_clock::now();
	bool bDontCount = false;


	// Has the GPU finished processing the commands of the current frame resource?
	// If not, wait until the GPU has completed commands up to this fence point.
	if (pCurrentFrameResource->iFence != 0 && pFence->GetCompletedValue() < pCurrentFrameResource->iFence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
		if (eventHandle != NULL)
		{
			HRESULT hresult = pFence->SetEventOnCompletion(pCurrentFrameResource->iFence, eventHandle);
			if (FAILED(hresult))
			{
				SError::showErrorMessageBoxAndLog(hresult);
				return;
			}

			WaitForSingleObject(eventHandle, INFINITE);
			CloseHandle(eventHandle);
		}
		else
		{
			SError::showErrorMessageBoxAndLog(std::to_string(GetLastError()));
			return;
		}
	}
	else
	{
#if defined(DEBUG) || defined(_DEBUG)
		frameStats.fTimeSpentWaitingForGPUInUpdateInMS = 0.0f;
#endif
		pProfiler->fTimeSpentWaitingForGPUBetweenFramesInMS = 0.0f;
		bDontCount = true;
	}

	if (bDontCount == false)
	{
		pProfiler->fTimeSpentWaitingForGPUBetweenFramesInMS
			= static_cast<float>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - timeInSleep).count()
				/ 1000000.0);

#if defined(DEBUG) || defined(_DEBUG)
		frameStats.fTimeSpentWaitingForGPUInUpdateInMS = pProfiler->fTimeSpentWaitingForGPUBetweenFramesInMS;
#endif
	}

#if defined(DEBUG) || defined(_DEBUG)
	std::chrono::time_point<std::chrono::steady_clock> timeOnUpdate = std::chrono::steady_clock::now();
#endif

	updateMaterials();
	updateObjectCBs();
	updateMainPassCB();

#if defined(DEBUG) || defined(_DEBUG)
	frameStats.fTimeSpentOnUpdateInMS
		= static_cast<float>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - timeOnUpdate).count()
			/ 1000000.0);
#endif
}

void SApplication::updateMaterials()
{
	std::lock_guard<std::mutex> guard(mtxDraw);

	for (size_t i = 0; i < vRegisteredMaterials.size(); i++)
	{
		SMaterial* pMaterial = vRegisteredMaterials[i];

		pMaterial->mtxUpdateMat.lock();

		if (pMaterial->iUpdateCBInFrameResourceCount > 0)
		{
			updateMaterialInFrameResource(pMaterial);
		}

		pMaterial->mtxUpdateMat.unlock();
	}


	// Update material bundles.
	if (vFrameResources[iCurrentFrameResourceIndex]->vMaterialBundles.size() > 0)
	{
		for (size_t i = 0; i < vFrameResources[iCurrentFrameResourceIndex]->vMaterialBundles.size(); i++)
		{
			for (size_t j = 0; j <
			vFrameResources[iCurrentFrameResourceIndex]->vMaterialBundles[i]->pShaderUsingThisResource->pCustomShaderResources->vMaterials.size(); j++)
			{
				SMaterial* pMaterial =
					vFrameResources[iCurrentFrameResourceIndex]->vMaterialBundles[i]->pShaderUsingThisResource->pCustomShaderResources->vMaterials[j];

				pMaterial->mtxUpdateMat.lock();

				if (pMaterial->iUpdateCBInFrameResourceCount > 0)
				{
					updateMaterialInFrameResource(pMaterial,
					vFrameResources[iCurrentFrameResourceIndex]->vMaterialBundles[i]->
						pShaderUsingThisResource->pCustomShaderResources->vFrameResourceBundles[iCurrentFrameResourceIndex],
						j);
				}

				pMaterial->mtxUpdateMat.unlock();
			}
		}
	}
}

void SApplication::updateObjectCBs()
{
	std::lock_guard<std::mutex> guard(mtxDraw);

	SUploadBuffer<SObjectConstants>* pCurrentObjectCB = pCurrentFrameResource->pObjectsCB.get();

	std::vector<SContainer*>* pvRenderableContainers = nullptr;
	pCurrentLevel->getRenderableContainers(pvRenderableContainers);

	for (size_t i = 0; i < pvRenderableContainers->size(); i++)
	{
		for (size_t j = 0; j < pvRenderableContainers->operator[](i)->vComponents.size(); j++)
		{
			updateComponentAndChilds(pvRenderableContainers->operator[](i)->vComponents[j], pCurrentObjectCB);
		}
	}
}

void SApplication::updateComponentAndChilds(SComponent* pComponent, SUploadBuffer<SObjectConstants>* pCurrentObjectCB)
{
	if (pComponent->componentType == SComponentType::SCT_MESH)
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
			objConstants.iCustomProperty = pMeshComponent->renderData.iCustomShaderProperty;

			pCurrentObjectCB->copyDataToElement(pMeshComponent->renderData.iObjCBIndex, objConstants);

			// Next FrameResource need to be updated too.
			pMeshComponent->renderData.iUpdateCBInFrameResourceCount--;

			pMeshComponent->mtxComponentProps.unlock();
		}
	}
	else if (pComponent->componentType == SComponentType::SCT_RUNTIME_MESH)
	{
		SRuntimeMeshComponent* pRuntimeMeshComponent = dynamic_cast<SRuntimeMeshComponent*>(pComponent);


		if (pRuntimeMeshComponent->bNoMeshDataOnSpawn == false && pRuntimeMeshComponent->bNewMeshData)
		{
			pRuntimeMeshComponent->mtxDrawComponent.lock();

			auto pVertexBuffer = pCurrentFrameResource->vRuntimeMeshVertexBuffers[pRuntimeMeshComponent->iIndexInFrameResourceVertexBuffer].get();

			std::vector<SVertex> vMeshShaderData = pRuntimeMeshComponent->meshData.toShaderVertex();

			pVertexBuffer->copyData(vMeshShaderData.data(), vMeshShaderData.size() * sizeof(SVertex));

			/*for (UINT64 i = 0; i < vMeshShaderData.size(); i++)
			{
				pVertexBuffer->copyDataToElement(i, vMeshShaderData[i]);
			}*/

			pRuntimeMeshComponent->renderData.pGeometry->pVertexBufferGPU = pVertexBuffer->getResource();

			pRuntimeMeshComponent->bNewMeshData = false;

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
			objConstants.iCustomProperty = pRuntimeMeshComponent->renderData.iCustomShaderProperty;

			pCurrentObjectCB->copyDataToElement(pRuntimeMeshComponent->renderData.iObjCBIndex, objConstants);

			// Next FrameResource need to be updated too.
			pRuntimeMeshComponent->renderData.iUpdateCBInFrameResourceCount--;

			pRuntimeMeshComponent->mtxComponentProps.unlock();
		}
	}


	std::vector<SComponent*> vChilds = pComponent->getChildComponents();

	for (size_t i = 0; i < vChilds.size(); i++)
	{
		updateComponentAndChilds(pComponent->getChildComponents()[i], pCurrentObjectCB);
	}
}

void SApplication::updateMainPassCB()
{
	camera.updateViewMatrix();

	DirectX::XMMATRIX view = camera.getViewMatrix();
	DirectX::XMMATRIX proj = camera.getProjMatrix();

	cameraBoundingFrustumOnLastMainPassUpdate = camera.cameraBoundingFrustum;

	DirectX::XMVECTOR viewDet = DirectX::XMMatrixDeterminant(view);
	DirectX::XMVECTOR projDet = DirectX::XMMatrixDeterminant(proj);

	DirectX::XMMATRIX viewProj = DirectX::XMMatrixMultiply(view, proj);

	DirectX::XMVECTOR viewProjDet = DirectX::XMMatrixDeterminant(viewProj);
	
	DirectX::XMMATRIX invView     = DirectX::XMMatrixInverse(&viewDet, view);
	DirectX::XMMATRIX invProj     = DirectX::XMMatrixInverse(&projDet, proj);
	DirectX::XMMATRIX invViewProj = DirectX::XMMatrixInverse(&viewProjDet, viewProj);

	DirectX::XMStoreFloat4x4(&mainRenderPassCB.vView, XMMatrixTranspose(view));
	DirectX::XMStoreFloat4x4(&mainRenderPassCB.vInvView, XMMatrixTranspose(invView));
	DirectX::XMStoreFloat4x4(&mainRenderPassCB.vProj, XMMatrixTranspose(proj));
	DirectX::XMStoreFloat4x4(&mainRenderPassCB.vInvProj, XMMatrixTranspose(invProj));
	DirectX::XMStoreFloat4x4(&mainRenderPassCB.vViewProj, XMMatrixTranspose(viewProj));
	DirectX::XMStoreFloat4x4(&mainRenderPassCB.vInvViewProj, XMMatrixTranspose(invViewProj));
	SVector cameraLoc = camera.getCameraLocationInWorld();
	mainRenderPassCB.vCameraPos = { cameraLoc.getX(), cameraLoc.getY(), cameraLoc.getZ() };
	mainRenderPassCB.vRenderTargetSize = DirectX::XMFLOAT2(static_cast<float>(iMainWindowWidth), static_cast<float>(iMainWindowHeight));
	mainRenderPassCB.vInvRenderTargetSize = DirectX::XMFLOAT2(1.0f / iMainWindowWidth, 1.0f / iMainWindowHeight);
	mainRenderPassCB.fNearZ = camera.getCameraNearClipPlane();
	mainRenderPassCB.fFarZ = camera.getCameraFarClipPlane();
	mainRenderPassCB.fTotalTime = gameTimer.getTimeElapsedInSec();
	mainRenderPassCB.fDeltaTime = gameTimer.getDeltaTimeBetweenTicksInSec();
	mainRenderPassCB.iDirectionalLightCount = 0;
	mainRenderPassCB.iPointLightCount = 0;
	mainRenderPassCB.iSpotLightCount = 0;
	mainRenderPassCB.iTextureFilterIndex = static_cast<std::underlying_type<TEX_FILTER_MODE>::type>(textureFilterIndex);

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

	SCameraEffects cameraEffects = camera.getCameraEffects();

	mainRenderPassCB.vCameraMultiplyColor = { cameraEffects.vCameraMultiplyColor.getX(), cameraEffects.vCameraMultiplyColor.getY(),
		cameraEffects.vCameraMultiplyColor.getZ() };
	mainRenderPassCB.fGamma = cameraEffects.fGamma;
	mainRenderPassCB.fSaturation = cameraEffects.fSaturation;

	mainRenderPassCB.iMainWindowHeight = iMainWindowHeight;
	mainRenderPassCB.iMainWindowWidth = iMainWindowWidth;

	SLevel* pLevel = getCurrentLevel();

	if (pLevel)
	{
		std::lock_guard<std::mutex> guard(mtxDraw);

		size_t iCurrentIndex = 0;
		size_t iTypeIndex = 0;
		std::vector<SLightComponentType> vTypes = { SLightComponentType::SLCT_DIRECTIONAL, SLightComponentType::SLCT_POINT, SLightComponentType::SLCT_SPOT };

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

						if (vTypes[iTypeIndex] == SLightComponentType::SLCT_DIRECTIONAL)
						{
							mainRenderPassCB.iDirectionalLightCount++;
						}
						else if (vTypes[iTypeIndex] == SLightComponentType::SLCT_POINT)
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
	}


	SUploadBuffer<SRenderPassConstants>* pCurrentPassCB = pCurrentFrameResource->pRenderPassCB.get();
	pCurrentPassCB->copyDataToElement(0, mainRenderPassCB);
}

void SApplication::draw()
{
	std::lock_guard<std::mutex> guard(mtxDraw);

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> pCurrentCommandListAllocator = pCurrentFrameResource->pCommandListAllocator;

	// Should be only called if the GPU is not using it (i.e. command queue is empty).

	HRESULT hresult = pCurrentCommandListAllocator->Reset();
	if (FAILED(hresult))
	{
		SError::showErrorMessageBoxAndLog(hresult);
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
		SError::showErrorMessageBoxAndLog(hresult);
		return;
	}



	executeCustomComputeShaders(true);


	// Record new commands in the command list:

	// Set the viewport and scissor rect. This needs to be reset whenever the command list is reset.

	pCommandList->RSSetViewports(1, &ScreenViewport);
	pCommandList->RSSetScissorRects(1, &ScissorRect);



	// Translate back buffer state from present state to render target state.

	CD3DX12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(getCurrentBackBufferResource(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	pCommandList->ResourceBarrier(1, &transition);



	// Clear buffers.

	// nullptr - "ClearRenderTargetView clears the entire resource view".
	pCommandList->ClearRenderTargetView(getCurrentBackBufferViewHandle(), backBufferFillColor, 0, nullptr);
	pCommandList->ClearDepthStencilView(getDepthStencilViewHandle(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);


	
	// Binds the RTV and DSV to the rendering pipeline.

	D3D12_CPU_DESCRIPTOR_HANDLE backBufferCpuHandle = getCurrentBackBufferViewHandle();
	D3D12_CPU_DESCRIPTOR_HANDLE depthBufferCpuHandle = getDepthStencilViewHandle();
	pCommandList->OMSetRenderTargets(1, &backBufferCpuHandle, true, &depthBufferCpuHandle);



	// CBV/SRV heap.

	ID3D12DescriptorHeap* descriptorHeaps[] = { pCBVSRVUAVHeap.Get() };
	pCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	pCommandList->SetGraphicsRootSignature(pRootSignature.Get());


	// Render pass cb.

	pCommandList->SetGraphicsRootConstantBufferView(0, pCurrentFrameResource->pRenderPassCB.get()->getResource()->GetGPUVirtualAddress());



	// Draw.

	iLastFrameDrawCallCount = 0;

	drawOpaqueComponents();

	setTransparentPSO();

	drawTransparentComponents();

	if (bDrawGUI)
	{
		drawGUIObjects();
	}

	transition
		= CD3DX12_RESOURCE_BARRIER::Transition(getCurrentBackBufferResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	pCommandList->ResourceBarrier(1, &transition);

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
	

	if (getCamera()->getCameraEffects().screenBlurEffect.bEnableScreenBlur)
	{
		// transitions texture (final back buffer texture) from present to copy_source state.
		pBlurEffect->addBlurToTexture(pCommandList.Get(), pBlurRootSignature.Get(), pBlurHorizontalPSO.Get(), pBlurVerticalPSO.Get(),
			getCurrentBackBufferResource(true), getCamera()->getCameraEffects().screenBlurEffect.iBlurStrength);

		// Prepare to copy blurred output to the back buffer.
		// back buffer resource is in copy_source state.
		CD3DX12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(getCurrentBackBufferResource(true),
			D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
		pCommandList->ResourceBarrier(1, &transition);

		pCommandList->CopyResource(getCurrentBackBufferResource(true), pBlurEffect->getOutput());

		// Transition to PRESENT state.
		transition = CD3DX12_RESOURCE_BARRIER::Transition(getCurrentBackBufferResource(true),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);
		pCommandList->ResourceBarrier(1, &transition);
	}

	executeCustomComputeShaders(false);



	// SHOULD BE LAST draw() STEP (before command list close):
	if (bSaveBackBufferPixelsForUser)
	{
		// Write commands to save back buffer pixels for user.
		saveBackBufferPixels();
	}

	// Stop recording commands.

	hresult = pCommandList->Close();
	if (FAILED(hresult))
	{
		SError::showErrorMessageBoxAndLog(hresult);
		return;
	}



	// Add the command list to the command queue for execution.

	ID3D12CommandList* commandLists[] = { pCommandList.Get() };
	pCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);


	doOptionalPauseForUserComputeShaders();


	if (bSaveBackBufferPixelsForUser)
	{
		flushCommandQueue();


		D3D12_RANGE readbackBufferRange{ 0, iPixelsBufferSize };
		unsigned char* pMappedData = nullptr;

		pPixelsReadBackBuffer->Map(0, &readbackBufferRange, reinterpret_cast<void**>(&pMappedData));

		std::memcpy(pPixels, pMappedData, iPixelsBufferSize);

		pPixelsReadBackBuffer->Unmap(0, nullptr);


		bSaveBackBufferPixelsForUser = false;

		pPixels = nullptr;

		pPixelsReadBackBuffer->Release();
	}


	// Swap back & front buffers.

	UINT SyncInterval = 0;

	if (bVSyncEnabled)
	{
		// Max: 4.
		SyncInterval = 1;
	}

	if (bVSyncEnabled)
	{
		hresult = pSwapChain->Present(SyncInterval, 0);
	}
	else
	{
		hresult = pSwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
	}
	if (FAILED(hresult))
	{
		SError::showErrorMessageBoxAndLog(hresult);
		return;
	}

	pDXTKGraphicsMemory->Commit(pCommandQueue.Get());
	
	if (iCurrentBackBuffer == (iSwapChainBufferCount - 1))
	{
		iCurrentBackBuffer = 0;
	}
	else
	{
		iCurrentBackBuffer++;
	}

	mtxFenceUpdate.lock();

	iCurrentFence++;
	pCurrentFrameResource->iFence = iCurrentFence;

	// Add an instruction to the command queue to set a new fence point.
	// This fence point won't be set until the GPU finishes processing all the commands prior to this Signal().
	pCommandQueue->Signal(pFence.Get(), iCurrentFence);

	mtxFenceUpdate.unlock();
}

void SApplication::drawOpaqueComponents()
{
	bool bUsingCustomResources = false;

	for (size_t i = 0; i < vOpaqueMeshesByCustomShader.size(); i++)
	{
		if (i != 0)
		{
			if (vOpaqueMeshesByCustomShader[i].pShader->pCustomShaderResources)
			{
				pCommandList->SetGraphicsRootSignature(vOpaqueMeshesByCustomShader[i].pShader->pCustomShaderResources->pCustomRootSignature.Get());

				pCommandList->SetGraphicsRootConstantBufferView(0, pCurrentFrameResource->pRenderPassCB.get()->getResource()->GetGPUVirtualAddress());

				bUsingCustomResources = true;
			}

			if (bUseFillModeWireframe)
			{
				pCommandList->SetPipelineState(vOpaqueMeshesByCustomShader[i].pShader->pOpaqueWireframePSO.Get());
			}
			else
			{
				pCommandList->SetPipelineState(vOpaqueMeshesByCustomShader[i].pShader->pOpaquePSO.Get());
			}
		}

		for (size_t j = 0; j < vOpaqueMeshesByCustomShader[i].vMeshComponentsWithThisShader.size(); j++)
		{
			if (vOpaqueMeshesByCustomShader[i].vMeshComponentsWithThisShader[j]->getContainer()->isVisible())
			{
				drawComponent(vOpaqueMeshesByCustomShader[i].vMeshComponentsWithThisShader[j], bUsingCustomResources);
			}
		}

		if (i != 0)
		{
			if (vOpaqueMeshesByCustomShader[i].pShader->pCustomShaderResources)
			{
				pCommandList->SetGraphicsRootSignature(pRootSignature.Get());
				pCommandList->SetGraphicsRootConstantBufferView(0, pCurrentFrameResource->pRenderPassCB.get()->getResource()->GetGPUVirtualAddress());

				bUsingCustomResources = false;
			}
		}
	}
}

void SApplication::drawTransparentComponents()
{
	bool bUsingCustomResources = false;

	for (size_t i = 0; i < vTransparentMeshesByCustomShader.size(); i++)
	{
		if (i != 0)
		{
			if (vTransparentMeshesByCustomShader[i].pShader->pCustomShaderResources)
			{
				pCommandList->SetGraphicsRootSignature(vTransparentMeshesByCustomShader[i].pShader->pCustomShaderResources->pCustomRootSignature.Get());

				pCommandList->SetGraphicsRootConstantBufferView(0, pCurrentFrameResource->pRenderPassCB.get()->getResource()->GetGPUVirtualAddress());

				bUsingCustomResources = true;
			}

			if (bUseFillModeWireframe)
			{
				pCommandList->SetPipelineState(vTransparentMeshesByCustomShader[i].pShader->pTransparentWireframePSO.Get());
			}
			else
			{
				if (MSAA_Enabled)
				{
					pCommandList->SetPipelineState(vTransparentMeshesByCustomShader[i].pShader->pTransparentAlphaToCoveragePSO.Get());
				}
				else
				{
					pCommandList->SetPipelineState(vTransparentMeshesByCustomShader[i].pShader->pTransparentPSO.Get());
				}
			}
		}

		for (size_t j = 0; j < vTransparentMeshesByCustomShader[i].vMeshComponentsWithThisShader.size(); j++)
		{
			if (vTransparentMeshesByCustomShader[i].vMeshComponentsWithThisShader[j]->getContainer()->isVisible())
			{
				drawComponent(vTransparentMeshesByCustomShader[i].vMeshComponentsWithThisShader[j], bUsingCustomResources);
			}
		}

		if (i != 0)
		{
			if (vTransparentMeshesByCustomShader[i].pShader->pCustomShaderResources)
			{
				pCommandList->SetGraphicsRootSignature(pRootSignature.Get());
				pCommandList->SetGraphicsRootConstantBufferView(0, pCurrentFrameResource->pRenderPassCB.get()->getResource()->GetGPUVirtualAddress());

				bUsingCustomResources = false;
			}
		}
	}
}

void SApplication::drawGUIObjects()
{
	for (size_t i = 0; i < vGUILayers.size(); i++)
	{
		for (size_t j = 0; j < vGUILayers[i].vGUIObjects.size(); j++)
		{
			if (vGUILayers[i].vGUIObjects[j]->isVisible())
			{
				if (vGUILayers[i].vGUIObjects[j]->objectType == SGUIType::SGT_IMAGE)
				{
					SGUIImage* pImage = dynamic_cast<SGUIImage*>(vGUILayers[i].vGUIObjects[j]);

					pImage->pSpriteBatch->Begin(pCommandList.Get());

					auto heapHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(pCBVSRVUAVHeap->GetGPUDescriptorHandleForHeapStart());
					heapHandle.Offset(pImage->iIndexInHeap, iCBVSRVUAVDescriptorSize);

					// Position.
					SVector position = pImage->getFullPosition();
					DirectX::SimpleMath::Vector2 pos = DirectX::SimpleMath::Vector2(position.getX(), position.getY());
					pos.x *= iMainWindowWidth;
					pos.y *= iMainWindowHeight;

					DirectX::XMUINT2 texSize = DirectX::GetTextureSize(pImage->pTexture.Get());

					// Origin.
					DirectX::SimpleMath::Vector2 origin = pImage->origin;
					origin.x *= texSize.x;
					origin.y *= texSize.y;

					// Source rect.
					RECT sourceRect;
					sourceRect.left = static_cast<LONG>(pImage->sourceRect.getX() * texSize.x);
					sourceRect.top = static_cast<LONG>(pImage->sourceRect.getY() * texSize.y);
					sourceRect.right = static_cast<LONG>(pImage->sourceRect.getZ() * texSize.x);
					sourceRect.bottom = static_cast<LONG>(pImage->sourceRect.getW() * texSize.y);

					// Scaling.
					DirectX::XMFLOAT2 scaling = pImage->scale; // usual scale
					SVector screenScaling = pImage->getFullScreenScaling();
					scaling.x *= screenScaling.getX();
					scaling.y *= screenScaling.getY();

					pImage->pSpriteBatch->Draw(heapHandle,
						texSize, pos, &sourceRect, DirectX::XMLoadFloat4(&pImage->color), pImage->fRotationInRad, origin,
						scaling);

					pImage->pSpriteBatch->End();
					iLastFrameDrawCallCount++;
				}
				else if (vGUILayers[i].vGUIObjects[j]->objectType == SGUIType::SGT_SIMPLE_TEXT)
				{
					SGUISimpleText* pText = dynamic_cast<SGUISimpleText*>(vGUILayers[i].vGUIObjects[j]);

					pText->pSpriteBatch->Begin(pCommandList.Get());

					// Origin.
					DirectX::SimpleMath::Vector2 texSize = pText->pSpriteFont->MeasureString(pText->sWrappedText.c_str());
					DirectX::SimpleMath::Vector2 origin = pText->origin;
					origin.x *= texSize.x;
					origin.y *= texSize.y;

					// Position.
					SVector position = pText->getFullPosition();
					DirectX::SimpleMath::Vector2 pos = DirectX::SimpleMath::Vector2(position.getX(), position.getY());
					pos.x *= iMainWindowWidth;
					pos.y *= iMainWindowHeight;

					// Scaling.
					DirectX::XMFLOAT2 scaling = pText->scale;
					SVector screenScaling = pText->getFullScreenScaling();
					scaling.x *= screenScaling.getX();
					scaling.y *= screenScaling.getY();

					if (pText->bDrawOutline)
					{
						pText->pSpriteFont->DrawString(pText->pSpriteBatch.get(), pText->sWrappedText.c_str(),
							pos + DirectX::SimpleMath::Vector2(1.f, 1.f), DirectX::XMLoadFloat4(&pText->outlineColor), pText->fRotationInRad,
							origin, scaling);
						pText->pSpriteFont->DrawString(pText->pSpriteBatch.get(), pText->sWrappedText.c_str(),
							pos + DirectX::SimpleMath::Vector2(-1.f, 1.f), DirectX::XMLoadFloat4(&pText->outlineColor), pText->fRotationInRad,
							origin, scaling);
						pText->pSpriteFont->DrawString(pText->pSpriteBatch.get(), pText->sWrappedText.c_str(),
							pos + DirectX::SimpleMath::Vector2(-1.f, -1.f), DirectX::XMLoadFloat4(&pText->outlineColor), pText->fRotationInRad,
							origin, scaling);
						pText->pSpriteFont->DrawString(pText->pSpriteBatch.get(), pText->sWrappedText.c_str(),
							pos + DirectX::SimpleMath::Vector2(1.f, -1.f), DirectX::XMLoadFloat4(&pText->outlineColor), pText->fRotationInRad,
							origin, scaling);
					}

					if (pText->bDrawShadow)
					{
						pText->pSpriteFont->DrawString(pText->pSpriteBatch.get(), pText->sWrappedText.c_str(),
							pos + DirectX::SimpleMath::Vector2(1.f, 1.f), DirectX::Colors::Black, pText->fRotationInRad,
							origin, scaling);
						pText->pSpriteFont->DrawString(pText->pSpriteBatch.get(), pText->sWrappedText.c_str(),
							pos + DirectX::SimpleMath::Vector2(-1.f, 1.f), DirectX::Colors::Black, pText->fRotationInRad,
							origin, scaling);
					}

					pText->pSpriteFont->DrawString(pText->pSpriteBatch.get(), pText->sWrappedText.c_str(), pos, DirectX::XMLoadFloat4(&pText->color),
						pText->fRotationInRad, origin, scaling);

					pText->pSpriteBatch->End();
					iLastFrameDrawCallCount++;
				}
			}
		}
	}
}

void SApplication::drawComponent(SComponent* pComponent, bool bUsingCustomResources)
{
	bool bDrawThisComponent = false;
	bool bUseFrustumCulling = true;

	bool bUsingInstancing = false;

	if (pComponent->componentType == SComponentType::SCT_MESH)
	{
		SMeshComponent* pMeshComponent = dynamic_cast<SMeshComponent*>(pComponent);

		pMeshComponent->mtxComponentProps.lock();
		if (pMeshComponent->isVisible() && (pMeshComponent->getMeshData()->getVerticesCount() > 0))
		{
			bDrawThisComponent = true;


			bUsingInstancing = pMeshComponent->bUseInstancing;

			if (pMeshComponent->bUseInstancing)
			{
				if (pMeshComponent->vFrameResourcesInstancedData.size() > 0)
				{
					if (pMeshComponent->vFrameResourcesInstancedData[0]->getElementCount() == 0)
					{
						bDrawThisComponent = false;
					}
				}
				else
				{
					bDrawThisComponent = false;
				}
			}


			if (pMeshComponent->bVertexBufferUsedInComputeShader)
			{
				bUseFrustumCulling = false;
			}
		}
		pMeshComponent->mtxComponentProps.unlock();
	}
	else if (pComponent->componentType == SComponentType::SCT_RUNTIME_MESH)
	{
		SRuntimeMeshComponent* pRuntimeMeshComponent = dynamic_cast<SRuntimeMeshComponent*>(pComponent);

		pRuntimeMeshComponent->mtxComponentProps.lock();
		if (pRuntimeMeshComponent->isVisible() && pRuntimeMeshComponent->getMeshData()->getVerticesCount() > 0)
		{
			bDrawThisComponent = true;

			if (pRuntimeMeshComponent->bDisableFrustumCulling)
			{
				bUseFrustumCulling = false;
			}
		}
		pRuntimeMeshComponent->mtxComponentProps.unlock();
	}

	if (bDrawThisComponent == false)
	{
		return;
	}

	if (bUsingInstancing == false && pComponent->fCullDistance > 0.0f)
	{
		SVector vToOrigin = pComponent->getLocationInWorld() - camera.getCameraLocationInWorld();
		if (vToOrigin.length() >= pComponent->fCullDistance)
		{
			return; // culled by distance
		}
	}

	if (bUseFrustumCulling && bUsingInstancing == false)
	{
		if (doFrustumCulling(pComponent) == false)
		{
			return; // mesh is outside of the view frustum
		}
	}


	if (pComponent->getRenderData()->primitiveTopologyType == D3D_PRIMITIVE_TOPOLOGY_LINELIST)
	{
		pCommandList->SetPipelineState(pOpaqueLineTopologyPSO.Get());
	}

	D3D12_VERTEX_BUFFER_VIEW vertBufView = pComponent->getRenderData()->pGeometry->getVertexBufferView();
	D3D12_INDEX_BUFFER_VIEW indBufView = pComponent->getRenderData()->pGeometry->getIndexBufferView();

	pCommandList->IASetVertexBuffers(0, 1, &vertBufView);
	pCommandList->IASetIndexBuffer(&indBufView);
	pCommandList->IASetPrimitiveTopology(pComponent->getRenderData()->primitiveTopologyType);


	size_t iMaterialCount = roundUp(vRegisteredMaterials.size(), OBJECT_CB_RESIZE_MULTIPLE);
	size_t iTextureCount = vLoadedTextures.size();



	// Texture descriptor table.

	auto heapHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(pCBVSRVUAVHeap->GetGPUDescriptorHandleForHeapStart());

	STextureHandle tex;
	bool bHasTexture = false;

	size_t iMatCBIndex = 0;

	if (pComponent->pCustomShader && pComponent->pCustomShader->pCustomShaderResources
		&& pComponent->pCustomShader->pCustomShaderResources->vMaterials.size() > 0)
	{
		if (pComponent->pCustomShader->pCustomShaderResources->vMaterials[0]->getMaterialProperties().getDiffuseTexture(&tex) == false)
		{
			bHasTexture = true;
		}
	}

	if (pComponent->componentType == SComponentType::SCT_MESH)
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
	else if (pComponent->componentType == SComponentType::SCT_RUNTIME_MESH)
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
		heapHandle.Offset(iPerFrameResEndOffset + tex.pRefToTexture->iTexSRVHeapIndex, iCBVSRVUAVDescriptorSize);
		pCommandList->SetGraphicsRootDescriptorTable(3, heapHandle);
	}



	// Object descriptor table.

	// (uncomment 'recreate cbv heap' in spawn/despawnContainer if
	// will use views)
	pCommandList->SetGraphicsRootConstantBufferView(1,
		pCurrentFrameResource->pObjectsCB.get()->getResource()->GetGPUVirtualAddress() +
		pComponent->getRenderData()->iObjCBIndex * pCurrentFrameResource->pObjectsCB->getElementSize());
	// (uncomment 'recreate cbv heap' in spawn/despawnContainer if
	// will use views)



	// Instancing data.
	UINT iDrawInstanceCount = 1;

	if (bUsingInstancing)
	{
		// Do frustum culling anyway.

		UINT64 drawCount = 0;
		doFrustumCullingOnInstancedMesh(dynamic_cast<SMeshComponent*>(pComponent), drawCount);

#if defined(DEBUG) || defined(_DEBUG)
		if (drawCount > UINT_MAX)
		{
			SError::showErrorMessageBoxAndLog("the number of visible instances is " + std::to_string(drawCount) + " but the allowed maximum is "
				+ std::to_string(UINT_MAX) + ". Please, reduce the number of instances.");
		}
#endif

		iDrawInstanceCount = static_cast<UINT>(drawCount);

		pCommandList->SetGraphicsRootShaderResourceView(4,
			dynamic_cast<SMeshComponent*>(pComponent)->vFrameResourcesInstancedData[iCurrentFrameResourceIndex]->getResource()->GetGPUVirtualAddress());
	}




	// Material descriptor table.

	if (pComponent->meshData.getMeshMaterial())
	{
		iMatCBIndex = pComponent->meshData.getMeshMaterial()->iMatCBIndex;
	}

	/*UINT iCBVIndex = iCurrentFrameResourceIndex * iMaterialCount + iMatCBIndex;
	auto cbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(pCBVSRVUAVHeap->GetGPUDescriptorHandleForHeapStart());
	cbvHandle.Offset(iCBVIndex, iCBVSRVUAVDescriptorSize);

	pCommandList->SetGraphicsRootDescriptorTable(2, cbvHandle);*/

	bool bUsingMaterialBundle = false;

	if (bUsingCustomResources)
	{
		if (pComponent->pCustomShader->pCustomShaderResources->vFrameResourceBundles.size() > 0)
		{
			bUsingMaterialBundle = true;
		}
	}

	if (bUsingMaterialBundle)
	{
		pCommandList->SetGraphicsRootShaderResourceView(2,
			pComponent->pCustomShader->pCustomShaderResources->vFrameResourceBundles[iCurrentFrameResourceIndex]->getResource()->GetGPUVirtualAddress());
	}
	else
	{
		pCommandList->SetGraphicsRootConstantBufferView(2,
			pCurrentFrameResource->pMaterialCB.get()->getResource()->GetGPUVirtualAddress()
			+ iMatCBIndex * pCurrentFrameResource->pMaterialCB->getElementSize());
	}



	// Draw.

	if (iDrawInstanceCount != 0)
	{
		pCommandList->DrawIndexedInstanced(pComponent->getRenderData()->iIndexCount, iDrawInstanceCount, pComponent->getRenderData()->iStartIndexLocation,
			pComponent->getRenderData()->iStartVertexLocation, 0);

		iLastFrameDrawCallCount++;
	}


	if (pComponent->getRenderData()->primitiveTopologyType == D3D_PRIMITIVE_TOPOLOGY_LINELIST)
	{
		if (bUseFillModeWireframe)
		{
			pCommandList->SetPipelineState(pOpaqueWireframePSO.Get());
		}
		else
		{
			pCommandList->SetPipelineState(pOpaquePSO.Get());
		}
	}
}

bool SApplication::flushCommandQueue()
{
	mtxFenceUpdate.lock();

	iCurrentFence++;

	HRESULT hresult = pCommandQueue->Signal(pFence.Get(), iCurrentFence);

	mtxFenceUpdate.unlock();

	if (FAILED(hresult))
	{
		SError::showErrorMessageBoxAndLog(hresult);
		return true;
	}

	// Wait until the GPU has completed commands up to this fence point.
	if (pFence->GetCompletedValue() < iCurrentFence)
	{
		HANDLE hEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
		if (hEvent != NULL)
		{
			// Fire event when GPU hits current fence.  
			hresult = pFence->SetEventOnCompletion(iCurrentFence, hEvent);
			if (FAILED(hresult))
			{
				SError::showErrorMessageBoxAndLog(hresult);
				return true;
			}

			// Wait until event is fired.
			WaitForSingleObject(hEvent, INFINITE);

			CloseHandle(hEvent);
		}
		else
		{
			SError::showErrorMessageBoxAndLog(std::to_string(GetLastError()));
			return true;
		}
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

	size_t iRemainder = iNum % iMultiple;
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

void SApplication::internalPhysicsTickThread()
{
	double dNSInMS = 1000000.0;
	double dTimeToSleepInNS = 1000.0 / iPhysicsTicksPerSecond * dNSInMS;

	std::chrono::time_point<std::chrono::steady_clock> timeUserOnPhysicsTick;


	while (bTerminatePhysics == false)
	{
		if (dTimeToSleepInNS > 0.0)
		{
			timeBeginPeriod(1);

			nanosleep(static_cast<long long>(dTimeToSleepInNS));

			timeEndPeriod(1);
		}



		gamePhysicsTimer.tick();

		timeUserOnPhysicsTick = std::chrono::steady_clock::now();

		onPhysicsTick(gamePhysicsTimer.getDeltaTimeBetweenTicksInSec());

		float fTimeSpentOnPhysicsTickInMS
			= static_cast<float>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - timeUserOnPhysicsTick).count()
				/ dNSInMS);

		dTimeToSleepInNS = (1000.0 / iPhysicsTicksPerSecond - fTimeSpentOnPhysicsTickInMS) * dNSInMS;

#if defined(DEBUG) || defined(_DEBUG)
		frameStats.fTimeSpentOnUserPhysicsTickFunctionInMS = fTimeSpentOnPhysicsTickInMS;
#endif
	}

	promiseFinishedPhysics.set_value(false);
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
	wc.lpszClassName = sMainWindowClassName.c_str();
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

	hMainWindow = CreateWindow(sMainWindowClassName.c_str(), sMainWindowTitle.c_str(),
		bHideTitleBar ? WS_POPUP : WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, iWidth, iHeight, 0, 0, hApplicationInstance, 0);
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
		SError::showErrorMessageBoxAndLog(std::to_string(GetLastError()));
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
		SError::showErrorMessageBoxAndLog(hresult);
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
		SError::showErrorMessageBoxAndLog(hresult);
		return true;
	}




	// Get supported hardware display adapter.

	if (getFirstSupportedDisplayAdapter(*pAdapter.GetAddressOf()))
	{
		SError::showErrorMessageBoxAndLog("can't find a supported display adapter.");

		return true;
	}




	// Create device.

	hresult = D3D12CreateDevice(
		pAdapter.Get(),
		ENGINE_D3D_FEATURE_LEVEL,
		IID_PPV_ARGS(&pDevice));

	if (FAILED(hresult))
	{
		SError::showErrorMessageBoxAndLog(hresult);
		
		// Try to create device with WARP (software) adapter.

		Microsoft::WRL::ComPtr<IDXGIAdapter> WarpAdapter;
		pFactory->EnumWarpAdapter(IID_PPV_ARGS(&WarpAdapter));

		hresult = D3D12CreateDevice(
			WarpAdapter.Get(),
			ENGINE_D3D_FEATURE_LEVEL,
			IID_PPV_ARGS(&pDevice));

		if (FAILED(hresult))
		{
			SError::showErrorMessageBoxAndLog(hresult);
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
		SError::showErrorMessageBoxAndLog(hresult);
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
		SError::showErrorMessageBoxAndLog("can't find any output adapters for current display adapter.");

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

		HRESULT hr = pAdapter->EnumOutputs(outputIndex, &pOutput1);
		if (hr == DXGI_ERROR_NOT_FOUND)
		{	// No more adapters to enumerate.
			break;
		}
		else if (FAILED(hr))
		{
			SError::showErrorMessageBoxAndLog(hr);
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
		SError::showErrorMessageBoxAndLog(hresult);
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
		SError::showErrorMessageBoxAndLog(hresult);
		return true;
	}




	// Create Command Allocator.

	hresult = pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(pCommandListAllocator.GetAddressOf()));

	if (FAILED(hresult))
	{
		SError::showErrorMessageBoxAndLog(hresult);
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
		SError::showErrorMessageBoxAndLog(hresult);
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
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
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
	fdesc.Windowed = true;

	// Note: Swap chain uses queue to perform flush.
	HRESULT hresult = pFactory->CreateSwapChainForHwnd(pCommandQueue.Get(), hMainWindow, &desc, &fdesc, pOutput.Get(), pSwapChain.GetAddressOf());
	if (FAILED(hresult))
	{
		SError::showErrorMessageBoxAndLog(hresult);
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
		SError::showErrorMessageBoxAndLog(hresult);
		return true;
	}

	std::vector<DXGI_MODE_DESC> vDisplayModes(numModes);

	hresult = pOutput->GetDisplayModeList(BackBufferFormat, 0, &numModes, &vDisplayModes[0]);
	if (FAILED(hresult))
	{
		SError::showErrorMessageBoxAndLog(hresult);
		return true;
	}


	// Save params.
	
	bool bSetResolutionToDefault = true;

	if (bCustomWindowSize)
	{
		// Not default params. Look if this resolution is supported.

		for (int i = static_cast<int>(vDisplayModes.size()) - 1; i > 0; i--)
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
		SError::showErrorMessageBoxAndLog(hresult);
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
		SError::showErrorMessageBoxAndLog(hresult);
		return true;
	}

	return false;
}

bool SApplication::createCBVSRVUAVHeap()
{
	UINT iDescCount = static_cast<UINT>(roundUp(vRegisteredMaterials.size(), OBJECT_CB_RESIZE_MULTIPLE)); // for SMaterialConstants

	// --------------------------------------
	// new stuff per frame resource goes here
	// make sure to recreate cbv heap (like in the end of the spawnContainerInLevel())
	// --------------------------------------

	UINT iDescriptorCount = iDescCount * iFrameResourcesCount;

	iPerFrameResEndOffset = iDescriptorCount;

	iDescriptorCount += static_cast<UINT>(vLoadedTextures.size()); // one SRV per texture
	// get gui item count
	size_t iGUIItemCount = 0;
	for (size_t i = 0; i < vGUILayers.size(); i++)
	{
		iGUIItemCount += vGUILayers[i].vGUIObjects.size();
	}
	iDescriptorCount += static_cast<UINT>(iGUIItemCount); // one SRV per gui item
	iDescriptorCount += BLUR_VIEW_COUNT; // for blur effect


	// new global stuff goes here


	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.NumDescriptors = iDescriptorCount;
	cbvHeapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask       = 0;

	HRESULT hresult = pDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&pCBVSRVUAVHeap));
	if (FAILED(hresult))
	{
		SError::showErrorMessageBoxAndLog(hresult);
		return true;
	}

	return false;
}

void SApplication::createViews()
{
	size_t iMatCount = roundUp(vRegisteredMaterials.size(), OBJECT_CB_RESIZE_MULTIPLE);
	if (iMatCount > INT_MAX)
	{
		SError::showErrorMessageBoxAndLog("cannot create CBVs because an overflow will occur.");
		return;
	}

	int iMaterialCount = static_cast<int>(iMatCount);

	UINT64 iMaterialCBSizeInBytes = static_cast<UINT64>(SMath::makeMultipleOf256(sizeof(SMaterialConstants)));

	if ((iFrameResourcesCount - 1) * iMaterialCount + (iMaterialCount - 1) > INT_MAX)
	{
		SError::showErrorMessageBoxAndLog("cannot create CBVs because an overflow will occur.");
		return;
	}

	for (int iFrameIndex = 0; iFrameIndex < iFrameResourcesCount; iFrameIndex++)
	{
		ID3D12Resource* pMaterialCB = vFrameResources[iFrameIndex]->pMaterialCB->getResource();

		for (int i = 0; i < iMaterialCount; i++)
		{
			D3D12_GPU_VIRTUAL_ADDRESS currentMaterialConstantBufferAddress = pMaterialCB->GetGPUVirtualAddress();

			// Offset to the ith material constant buffer in the buffer.
			currentMaterialConstantBufferAddress += i * iMaterialCBSizeInBytes;

			// Offset to the material CBV in the descriptor heap.
			int iIndexInHeap = iFrameIndex * iMaterialCount + i;
			auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(pCBVSRVUAVHeap->GetCPUDescriptorHandleForHeapStart());
			handle.Offset(iIndexInHeap, iCBVSRVUAVDescriptorSize);

			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
			cbvDesc.BufferLocation = currentMaterialConstantBufferAddress;
			cbvDesc.SizeInBytes = static_cast<UINT>(iMaterialCBSizeInBytes);

			pDevice->CreateConstantBufferView(&cbvDesc, handle);
		}
	}


	// Need one SRV per loaded texture.
	if (iPerFrameResEndOffset + vLoadedTextures.size() > INT_MAX)
	{
		SError::showErrorMessageBoxAndLog("cannot create SRVs because an overflow will occur.");
		return;
	}

	for (size_t i = 0; i < vLoadedTextures.size(); i++)
	{
		int iIndexInHeap = iPerFrameResEndOffset + static_cast<int>(i);
		auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(pCBVSRVUAVHeap->GetCPUDescriptorHandleForHeapStart());

		handle.Offset(iIndexInHeap, iCBVSRVUAVDescriptorSize);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = vLoadedTextures[i]->pResource->GetDesc().Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = vLoadedTextures[i]->pResource->GetDesc().MipLevels;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

		pDevice->CreateShaderResourceView(vLoadedTextures[i]->pResource.Get(), &srvDesc, handle);

		vLoadedTextures[i]->iTexSRVHeapIndex = static_cast<int>(i);
	}


	// GUI SRVs
	int iCurrentIndex = 0;
	for (size_t i = 0; i < vGUILayers.size(); i++)
	{
		for (size_t j = 0; j < vGUILayers[i].vGUIObjects.size(); j++)
		{
			int iIndexInHeap = iPerFrameResEndOffset + static_cast<int>(vLoadedTextures.size()) + iCurrentIndex;

			if (vGUILayers[i].vGUIObjects[j]->objectType == SGUIType::SGT_IMAGE)
			{
				SGUIImage* pText = dynamic_cast<SGUIImage*>(vGUILayers[i].vGUIObjects[j]);

				auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(pCBVSRVUAVHeap->GetCPUDescriptorHandleForHeapStart());

				handle.Offset(iIndexInHeap, iCBVSRVUAVDescriptorSize);

				DirectX::CreateShaderResourceView(pDevice.Get(), pText->pTexture.Get(), handle);

				pText->iIndexInHeap = iIndexInHeap;
			}
			else if (vGUILayers[i].vGUIObjects[j]->objectType == SGUIType::SGT_SIMPLE_TEXT)
			{
				SGUISimpleText* pText = dynamic_cast<SGUISimpleText*>(vGUILayers[i].vGUIObjects[j]);

				auto cpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(pCBVSRVUAVHeap->GetCPUDescriptorHandleForHeapStart());
				cpuHandle.Offset(iIndexInHeap, iCBVSRVUAVDescriptorSize);

				auto gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(pCBVSRVUAVHeap->GetGPUDescriptorHandleForHeapStart());
				gpuHandle.Offset(iIndexInHeap, iCBVSRVUAVDescriptorSize);

				pText->cpuHandle = cpuHandle;
				pText->gpuHandle = gpuHandle;

				if (pText->bInitFontCalled)
				{
					// already registered, initFontResources() was called and now referencing wrong cpu/gpu handles
					// refresh them
					pText->initFontResource();
				}
			}

			iCurrentIndex++;
		}
	}

	// get gui item count
	size_t iGUIItemCount = 0;
	for (size_t i = 0; i < vGUILayers.size(); i++)
	{
		iGUIItemCount += vGUILayers[i].vGUIObjects.size();
	}

	if (pBlurEffect)
	{
		// Need 2 SRV & 2 UAV for blur.

		int iIndexInHeap = iPerFrameResEndOffset + static_cast<int>(vLoadedTextures.size() + iGUIItemCount);

		auto cpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(pCBVSRVUAVHeap->GetCPUDescriptorHandleForHeapStart());
		cpuHandle.Offset(iIndexInHeap, iCBVSRVUAVDescriptorSize);

		auto gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(pCBVSRVUAVHeap->GetGPUDescriptorHandleForHeapStart());
		gpuHandle.Offset(iIndexInHeap, iCBVSRVUAVDescriptorSize);

		pBlurEffect->assignHeapHandles(cpuHandle, gpuHandle, iCBVSRVUAVDescriptorSize);
	}
}

void SApplication::createFrameResources()
{
	for (int i = 0; i < iFrameResourcesCount; i++)
	{
		vFrameResources.push_back(std::make_unique<SFrameResource>(pDevice.Get(), 0));
	}
}

bool SApplication::createRootSignature(SCustomShaderResources* pCustomShaderResources, bool bUseTextures, bool bUseInstancing)
{
	// The root signature defines the resources the shader programs expect.

	CD3DX12_DESCRIPTOR_RANGE texTable;
	if (pCustomShaderResources && bUseTextures)
	{
		texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, static_cast<UINT>(pCustomShaderResources->vMaterials.size()), 0);
	}
	else
	{
		texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	}

	// Root parameter can be a table, root descriptor or root constants.
	std::vector<CD3DX12_ROOT_PARAMETER> vRootParameters(bUseInstancing ? 5 : 4);

	// Perfomance TIP: Order from most frequent to least frequent.
	vRootParameters[0].InitAsConstantBufferView(0); // cbRenderPass
	vRootParameters[1].InitAsConstantBufferView(1); // cbObject

	bool bCustomMaterials = false;
	if (pCustomShaderResources)
	{
		if (pCustomShaderResources->vMaterials.size() > 0)
		{
			bCustomMaterials = true;
			
		}
	}

	if (bCustomMaterials)
	{
		vRootParameters[2].InitAsShaderResourceView(0, 1); // materials
	}
	else
	{
		vRootParameters[2].InitAsConstantBufferView(2); // cbMaterial
	}


	vRootParameters[3].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL); // textures

	if (bUseInstancing)
	{
		vRootParameters[4].InitAsShaderResourceView(1, 1);
	}

	// Static samples don't need a heap.
	auto staticSamples = getStaticSamples();

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(bUseInstancing ? 5 : 4, &vRootParameters[0], static_cast<UINT>(staticSamples.size()), staticSamples.data(),
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
		SError::showErrorMessageBoxAndLog(hresult);
		return true;
	}


	if (pCustomShaderResources)
	{
		hresult = pDevice->CreateRootSignature(
			0,
			serializedRootSignature->GetBufferPointer(),
			serializedRootSignature->GetBufferSize(),
			IID_PPV_ARGS(&pCustomShaderResources->pCustomRootSignature)
		);
	}
	else
	{
		hresult = pDevice->CreateRootSignature(
			0,
			serializedRootSignature->GetBufferPointer(),
			serializedRootSignature->GetBufferSize(),
			IID_PPV_ARGS(&pRootSignature)
		);
	}
	
	if (FAILED(hresult))
	{
		SError::showErrorMessageBoxAndLog(hresult);
		return true;
	}


	return false;
}

bool SApplication::createBlurRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE srvTable;
	srvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	CD3DX12_DESCRIPTOR_RANGE uavTable;
	uavTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);


	CD3DX12_ROOT_PARAMETER slotRootParameter[3];

	// Perfomance TIP: Order from most frequent to least frequent.
	slotRootParameter[0].InitAsConstants(11, 0);
	slotRootParameter[1].InitAsDescriptorTable(1, &srvTable);
	slotRootParameter[2].InitAsDescriptorTable(1, &uavTable);


	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(3, slotRootParameter,
		0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}

	if (FAILED(hr))
	{
		SError::showErrorMessageBoxAndLog(hr);
		return true;
	}


	hr = pDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&pBlurRootSignature)
	);
	if (FAILED(hr))
	{
		SError::showErrorMessageBoxAndLog(hr);
		return true;
	}


	return false;
}

bool SApplication::createShadersAndInputLayout()
{
	// Also change in SApplication::compileCustomShader().
	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		"ALPHA_TEST", "1",
		NULL, NULL
	};
	// Also change in SApplication::compileCustomShader().

	mShaders["basicVS"] = SMiscHelpers::compileShader(L"shaders/basic.hlsl", nullptr, L"VS", SE_VS_SM, bCompileShadersInRelease);
	mShaders["basicPS"] = SMiscHelpers::compileShader(L"shaders/basic.hlsl", nullptr, L"PS", SE_PS_SM, bCompileShadersInRelease);
	mShaders["basicAlphaPS"] = SMiscHelpers::compileShader(L"shaders/basic.hlsl", alphaTestDefines, L"PS", SE_PS_SM, bCompileShadersInRelease);
	mShaders["horzBlurCS"] = SMiscHelpers::compileShader(L"shaders/compute_blur.hlsl", nullptr, L"horzBlurCS", SE_CS_SM, bCompileShadersInRelease);
	mShaders["vertBlurCS"] = SMiscHelpers::compileShader(L"shaders/compute_blur.hlsl", nullptr, L"vertBlurCS", SE_CS_SM, bCompileShadersInRelease);


	// All meshes with default shader will be here.

	SShaderObjects so;
	so.pShader = nullptr;

	vOpaqueMeshesByCustomShader.push_back(so);
	vTransparentMeshesByCustomShader.push_back(so);


	vInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "CUSTOM", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	return false;
}


bool SApplication::createPSO(SShader* pPSOsForCustomShader)
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	memset(&psoDesc, 0, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

	psoDesc.InputLayout = { vInputLayout.data(), static_cast<UINT>(vInputLayout.size()) };
	if (pPSOsForCustomShader)
	{
		if (pPSOsForCustomShader->pCustomShaderResources)
		{
			psoDesc.pRootSignature = pPSOsForCustomShader->pCustomShaderResources->pCustomRootSignature.Get();
		}
		else
		{
			psoDesc.pRootSignature = pRootSignature.Get();
		}
	}
	else
	{
		psoDesc.pRootSignature = pRootSignature.Get();
	}

	if (pPSOsForCustomShader)
	{
		psoDesc.VS =
		{
			reinterpret_cast<BYTE*>(pPSOsForCustomShader->pVS->GetBufferPointer()),
			pPSOsForCustomShader->pVS->GetBufferSize()
		};
		psoDesc.PS =
		{
			reinterpret_cast<BYTE*>(pPSOsForCustomShader->pPS->GetBufferPointer()),
			pPSOsForCustomShader->pPS->GetBufferSize()
		};
	}
	else
	{
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
	}

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


	HRESULT hresult;
	if (pPSOsForCustomShader)
	{
		hresult = pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pPSOsForCustomShader->pOpaquePSO));
		if (FAILED(hresult))
		{
			SError::showErrorMessageBoxAndLog(hresult);
			return true;
		}
	}
	else
	{
		hresult = pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pOpaquePSO));
		if (FAILED(hresult))
		{
			SError::showErrorMessageBoxAndLog(hresult);
			return true;
		}

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoLineDesc = psoDesc;
		psoLineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		hresult = pDevice->CreateGraphicsPipelineState(&psoLineDesc, IID_PPV_ARGS(&pOpaqueLineTopologyPSO));
		if (FAILED(hresult))
		{
			SError::showErrorMessageBoxAndLog(hresult);
			return true;
		}
	}


	D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPsoDesc = psoDesc;
	transparentPsoDesc.BlendState.RenderTarget[0] = transparencyBlendDesc;
	transparentPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	if (pPSOsForCustomShader)
	{
		transparentPsoDesc.PS =
		{
			reinterpret_cast<BYTE*>(pPSOsForCustomShader->pAlphaPS->GetBufferPointer()),
			pPSOsForCustomShader->pAlphaPS->GetBufferSize()
		};
		hresult = pDevice->CreateGraphicsPipelineState(&transparentPsoDesc, IID_PPV_ARGS(&pPSOsForCustomShader->pTransparentPSO));
		if (FAILED(hresult))
		{
			SError::showErrorMessageBoxAndLog(hresult);
			return true;
		}
	}
	else
	{
		transparentPsoDesc.PS =
		{
			reinterpret_cast<BYTE*>(mShaders["basicAlphaPS"]->GetBufferPointer()),
			mShaders["basicAlphaPS"]->GetBufferSize()
		};
		hresult = pDevice->CreateGraphicsPipelineState(&transparentPsoDesc, IID_PPV_ARGS(&pTransparentPSO));
		if (FAILED(hresult))
		{
			SError::showErrorMessageBoxAndLog(hresult);
			return true;
		}
	}
	


	D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentAlphaToCoveragePsoDesc;
	transparentAlphaToCoveragePsoDesc = transparentPsoDesc;
	transparentAlphaToCoveragePsoDesc.BlendState.AlphaToCoverageEnable = true;

	if (pPSOsForCustomShader)
	{
		hresult = pDevice->CreateGraphicsPipelineState(&transparentAlphaToCoveragePsoDesc, IID_PPV_ARGS(&pPSOsForCustomShader->pTransparentAlphaToCoveragePSO));
		if (FAILED(hresult))
		{
			SError::showErrorMessageBoxAndLog(hresult);
			return true;
		}
	}
	else
	{
		hresult = pDevice->CreateGraphicsPipelineState(&transparentAlphaToCoveragePsoDesc, IID_PPV_ARGS(&pTransparentAlphaToCoveragePSO));
		if (FAILED(hresult))
		{
			SError::showErrorMessageBoxAndLog(hresult);
			return true;
		}
	}



	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueWireframePsoDesc = psoDesc;
	opaqueWireframePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;

	if (pPSOsForCustomShader)
	{
		hresult = pDevice->CreateGraphicsPipelineState(&opaqueWireframePsoDesc, IID_PPV_ARGS(&pPSOsForCustomShader->pOpaqueWireframePSO));
		if (FAILED(hresult))
		{
			SError::showErrorMessageBoxAndLog(hresult);
			return true;
		}
	}
	else
	{
		hresult = pDevice->CreateGraphicsPipelineState(&opaqueWireframePsoDesc, IID_PPV_ARGS(&pOpaqueWireframePSO));
		if (FAILED(hresult))
		{
			SError::showErrorMessageBoxAndLog(hresult);
			return true;
		}
	}
	


	D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentWireframePsoDesc = transparentPsoDesc;
	transparentWireframePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;

	if (pPSOsForCustomShader)
	{
		
		hresult = pDevice->CreateGraphicsPipelineState(&transparentWireframePsoDesc, IID_PPV_ARGS(&pPSOsForCustomShader->pTransparentWireframePSO));
		if (FAILED(hresult))
		{
			SError::showErrorMessageBoxAndLog(hresult);
			return true;
		}
	}
	else
	{
		hresult = pDevice->CreateGraphicsPipelineState(&transparentWireframePsoDesc, IID_PPV_ARGS(&pTransparentWireframePSO));
		if (FAILED(hresult))
		{
			SError::showErrorMessageBoxAndLog(hresult);
			return true;
		}


		// PSO for blur.

		D3D12_COMPUTE_PIPELINE_STATE_DESC horzBlurPSO = {};
		horzBlurPSO.pRootSignature = pBlurRootSignature.Get();
		horzBlurPSO.CS =
		{
			reinterpret_cast<BYTE*>(mShaders["horzBlurCS"]->GetBufferPointer()),
			mShaders["horzBlurCS"]->GetBufferSize()
		};
		horzBlurPSO.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
		hresult = pDevice->CreateComputePipelineState(&horzBlurPSO, IID_PPV_ARGS(&pBlurHorizontalPSO));
		if (FAILED(hresult))
		{
			SError::showErrorMessageBoxAndLog(hresult);
			return true;
		}


		D3D12_COMPUTE_PIPELINE_STATE_DESC vertBlurPSO = {};
		vertBlurPSO.pRootSignature = pBlurRootSignature.Get();
		vertBlurPSO.CS =
		{
			reinterpret_cast<BYTE*>(mShaders["vertBlurCS"]->GetBufferPointer()),
			mShaders["vertBlurCS"]->GetBufferSize()
		};
		vertBlurPSO.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
		hresult = pDevice->CreateComputePipelineState(&vertBlurPSO, IID_PPV_ARGS(&pBlurVerticalPSO));
		if (FAILED(hresult))
		{
			SError::showErrorMessageBoxAndLog(hresult);
			return true;
		}
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
		SError::showErrorMessageBoxAndLog(hresult);
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
		SError::showErrorMessageBoxAndLog(hresult);
		return true;
	}

	ID3D12CommandList* commandLists[] = { pCommandList.Get() };

	pCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	return false;
}

void SApplication::updateMaterialInFrameResource(SMaterial * pMaterial,
	SUploadBuffer<SMaterialConstants>* pCustomResource,
	size_t iElementIndexInResource)
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

	if (pCustomResource)
	{
		pCustomResource->copyDataToElement(iElementIndexInResource, matConstants);
	}
	else
	{
		pCurrentFrameResource->pMaterialCB->copyDataToElement(pMaterial->iMatCBIndex, matConstants);
	}

	// Next FrameResource need to be updated too.
	pMaterial->iUpdateCBInFrameResourceCount--;
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

	SError::showErrorMessageBoxAndLog(hResult);
}

void SApplication::removeComponentsFromGlobalVectors(SContainer* pContainer)
{
	std::vector<SComponent*> vOpaqueMeshComponents;
	std::vector<SComponent*> vTransparentMeshComponents;

	pContainer->getAllMeshComponents(&vOpaqueMeshComponents, &vTransparentMeshComponents);

	size_t iLeftComponents = vOpaqueMeshComponents.size();

	for (long long i = 0; i < static_cast<long long>(vAllRenderableSpawnedOpaqueComponents.size()); i++)
	{
		for (long long j = 0; j < static_cast<long long>(vOpaqueMeshComponents.size()); j++)
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

	for (long long i = 0; i < static_cast<long long>(vAllRenderableSpawnedTransparentComponents.size()); i++)
	{
		for (long long j = 0; j < static_cast<long long>(vTransparentMeshComponents.size()); j++)
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

void SApplication::moveGUIObjectToLayer(SGUIObject* pObject, int iNewLayer)
{
	if (iNewLayer < 0)
	{
		SError::showErrorMessageBoxAndLog("layer value should be positive.");
		return;
	}

	{
		std::lock_guard<std::mutex> guard(mtxDraw);

		// Find object.
		size_t iObjectLayerIndex = 0;
		size_t iObjectIndex = 0;
		bool bFound = false;
		for (size_t i = 0; i < vGUILayers.size(); i++)
		{
			for (size_t j = 0; j < vGUILayers[i].vGUIObjects.size(); j++)
			{
				if (vGUILayers[i].vGUIObjects[j] == pObject)
				{
					iObjectLayerIndex = i;
					iObjectIndex = j;
					bFound = true;
					break;
				}
			}

			if (bFound)
			{
				break;
			}
		}

		if (bFound == false)
		{
			SError::showErrorMessageBoxAndLog("could not find the specified GUI object.");
			return;
		}

		// See if the GUI layer with this new layer index exists.
		bFound = false;
		bool bNeedToInsert = false;
		size_t iTargetLayerIndex = 0;
		size_t iInsertIndex = 0;
		for (size_t i = 0; i < vGUILayers.size(); i++)
		{
			if (vGUILayers[i].iLayer > iNewLayer)
			{
				bFound = false;
				bNeedToInsert = true;
				iInsertIndex = i;
				break;
			}
			else if (vGUILayers[i].iLayer == iNewLayer)
			{
				bFound = true;
				iTargetLayerIndex = i;
				break;
			}
		}

		if (bFound == false)
		{
			// Create this new layer.

			if (bNeedToInsert)
			{
				vGUILayers.insert(vGUILayers.begin() + iInsertIndex, SGUILayer{ iNewLayer, std::vector<SGUIObject*>() });
				iTargetLayerIndex = iInsertIndex;
			}
			else
			{
				vGUILayers.push_back(SGUILayer{ iNewLayer, std::vector<SGUIObject*>() });
				iTargetLayerIndex = vGUILayers.size() - 1;
			}
		}

		// Remove object from current position.
		SGUIObject* pObjectToMove = vGUILayers[iObjectLayerIndex].vGUIObjects[iObjectIndex];
		vGUILayers[iObjectLayerIndex].vGUIObjects.erase(vGUILayers[iObjectLayerIndex].vGUIObjects.begin() + iObjectIndex);
		if (vGUILayers[iObjectLayerIndex].vGUIObjects.size() == 0 && iObjectLayerIndex != 0)
		{
			vGUILayers.erase(vGUILayers.begin() + iObjectLayerIndex);
		}

		// Move object to new layer.
		vGUILayers[iTargetLayerIndex].vGUIObjects.push_back(pObjectToMove);

		pObjectToMove->iZLayer = iNewLayer;
	}

#if defined(DEBUG) || defined(_DEBUG)
	if (pObject->objectType == SGUIType::SGT_LAYOUT)
	{
		SGUILayout* pLayout = dynamic_cast<SGUILayout*>(pObject);

		if (pLayout->pDebugLayoutFillImage->bIsRegistered)
		{
			moveGUIObjectToLayer(pLayout->pDebugLayoutFillImage, iNewLayer);
		}
	}
#endif
}

void SApplication::refreshHeap()
{
	std::lock_guard<std::mutex> guard(mtxDraw);

	flushCommandQueue();

	createCBVSRVUAVHeap();
	createViews();
}

void SApplication::releaseShader(SShader* pShader)
{
	pShader->pVS.Release();
	pShader->pPS.Release();
	pShader->pAlphaPS.Release();

	unsigned long iLeft = pShader->pOpaquePSO.Reset();

	if (iLeft != 0)
	{
		showMessageBox(L"Error", L"SApplication::releaseShader() error: opaque shader pso ref count is not 0.");
	}

	iLeft = pShader->pTransparentPSO.Reset();

	if (iLeft != 0)
	{
		showMessageBox(L"Error", L"SApplication::releaseShader() error: transparent shader pso ref count is not 0.");
	}

	iLeft = pShader->pTransparentAlphaToCoveragePSO.Reset();

	if (iLeft != 0)
	{
		showMessageBox(L"Error", L"SApplication::releaseShader() error: transparent alpha to coverage shader pso ref count is not 0.");
	}

	iLeft = pShader->pOpaqueWireframePSO.Reset();

	if (iLeft != 0)
	{
		showMessageBox(L"Error", L"SApplication::releaseShader() error: opaque wireframe pso ref count is not 0.");
	}

	iLeft = pShader->pTransparentWireframePSO.Reset();

	if (iLeft != 0)
	{
		showMessageBox(L"Error", L"SApplication::releaseShader() error: transparent wireframe pso ref count is not 0.");
	}

	if (pShader->pCustomShaderResources)
	{
		iLeft = pShader->pCustomShaderResources->pCustomRootSignature.Reset();

		if (iLeft != 0)
		{
			showMessageBox(L"Error", L"SApplication::releaseShader() error: custom shader resources root signature ref count is not 0.");
		}

		for (size_t i = 0; i < vFrameResources.size(); i++)
		{
			vFrameResources[i]->removeMaterialBundle(pShader);
		}

		for (size_t i = 0; i < pShader->pCustomShaderResources->vMaterials.size(); i++)
		{
			delete pShader->pCustomShaderResources->vMaterials[i];
		}

		delete pShader->pCustomShaderResources;
	}

	delete pShader;
}

void SApplication::removeShaderFromObjects(SShader* pShader, std::vector<SShaderObjects>* pObjectsByShader)
{
	for (size_t i = 0; i < pObjectsByShader->size(); i++)
	{
		if (pObjectsByShader->operator[](i).pShader == pShader)
		{
			for (size_t j = 0; j < pObjectsByShader->operator[](i).vMeshComponentsWithThisShader.size(); j++)
			{
				if (pObjectsByShader->operator[](i).vMeshComponentsWithThisShader[j]->componentType == SComponentType::SCT_MESH)
				{
					dynamic_cast<SMeshComponent*>(pObjectsByShader->operator[](i).vMeshComponentsWithThisShader[j])->pCustomShader = nullptr;

					// Add to default engine shader.
					pObjectsByShader->operator[](0).vMeshComponentsWithThisShader.push_back(pObjectsByShader->operator[](i).vMeshComponentsWithThisShader[j]);
				}
				else if (pObjectsByShader->operator[](i).vMeshComponentsWithThisShader[j]->componentType == SComponentType::SCT_RUNTIME_MESH)
				{
					dynamic_cast<SRuntimeMeshComponent*>(pObjectsByShader->operator[](i).vMeshComponentsWithThisShader[j])->pCustomShader = nullptr;

					// Add to default engine shader.
					pObjectsByShader->operator[](0).vMeshComponentsWithThisShader.push_back(pObjectsByShader->operator[](i).vMeshComponentsWithThisShader[j]);
				}
			}

			pObjectsByShader->erase(pObjectsByShader->begin() + i);


			break;
		}
	}
}

void SApplication::forceChangeMeshShader(SShader* pOldShader, SShader* pNewShader, SComponent* pComponent, bool bUsesTransparency)
{
	std::vector<SShaderObjects>* pObjectsByShader;

	if (bUsesTransparency)
	{
		pObjectsByShader = &vTransparentMeshesByCustomShader;
	}
	else
	{
		pObjectsByShader = &vOpaqueMeshesByCustomShader;
	}


	bool bFound = false;

	bool bFoundNew = false;
	size_t iNewVectorIndex = 0;

	std::lock_guard<std::mutex> guard(mtxDraw);

	for (size_t i = 0; i < pObjectsByShader->size(); i++)
	{
		if (pObjectsByShader->operator[](i).pShader == pOldShader)
		{
			for (size_t j = 0; j < pObjectsByShader->operator[](i).vMeshComponentsWithThisShader.size(); j++)
			{
				if (pObjectsByShader->operator[](i).vMeshComponentsWithThisShader[j] == pComponent)
				{
					bFound = true;

					pObjectsByShader->operator[](i).vMeshComponentsWithThisShader.erase(
						pObjectsByShader->operator[](i).vMeshComponentsWithThisShader.begin() + j);

					if ((pObjectsByShader->operator[](i).vMeshComponentsWithThisShader.size() == 0)
						&& (pObjectsByShader->operator[](i).pShader != nullptr))
					{
						pObjectsByShader->erase(pObjectsByShader->begin() + i);
					}

					break;
				}
			}

			if (bFound && bFoundNew)
			{
				break;
			}
		}
		else if (pObjectsByShader->operator[](i).pShader == pNewShader)
		{
			bFoundNew = true;
			iNewVectorIndex = i;

			if (bFound)
			{
				break;
			}
		}
	}

	if (bFound == false)
	{
		SError::showErrorMessageBoxAndLog("could not find specified old shader / object.");
		return;
	}


	if (bFoundNew)
	{
		pObjectsByShader->operator[](iNewVectorIndex).vMeshComponentsWithThisShader.push_back(pComponent);
	}
	else
	{
		SShaderObjects so;
		so.pShader = pNewShader;
		so.vMeshComponentsWithThisShader.push_back(pComponent);

		pObjectsByShader->push_back(so);
	}
}

void SApplication::saveBackBufferPixels()
{
	if (BackBufferFormat == DXGI_FORMAT_R8G8B8A8_UNORM)
	{
		ID3D12Resource* pBackBuffer = getCurrentBackBufferResource(true);
		D3D12_RESOURCE_DESC desc = pBackBuffer->GetDesc();

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;

		pDevice->GetCopyableFootprints(&desc, 0, 1, 0, &footprint, nullptr, nullptr, &iPixelsBufferSize);

		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_READBACK);
		CD3DX12_RESOURCE_DESC buf = CD3DX12_RESOURCE_DESC::Buffer(iPixelsBufferSize);

		HRESULT hresult = pDevice->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&buf,
				D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&pPixelsReadBackBuffer));

		CD3DX12_TEXTURE_COPY_LOCATION dst(pPixelsReadBackBuffer.Get(), footprint);
		CD3DX12_TEXTURE_COPY_LOCATION src(pBackBuffer, 0);

		pCommandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

		/*pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
			getCurrentBackBufferResource(true), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_SOURCE));

		pCommandList->CopyResource(pPixelsReadBackBuffer.Get(), getCurrentBackBufferResource(true));

		pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
			getCurrentBackBufferResource(true), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_PRESENT));*/
	}
	else
	{
		SError::showErrorMessageBoxAndLog("unsupported back buffer format.");
	}
}

void SApplication::executeCustomComputeShaders(bool bBeforeDraw)
{
	bool bExecutedAtLeastOne = false;

	for (size_t i = 0; i < vUserComputeShaders.size(); i++)
	{
		if (vUserComputeShaders[i]->bExecuteShader && (vUserComputeShaders[i]->bExecuteShaderBeforeDraw == bBeforeDraw))
		{
			vUserComputeShaders[i]->mtxComputeSettings.lock();

			executeCustomComputeShader(vUserComputeShaders[i]);

			vUserComputeShaders[i]->mtxComputeSettings.unlock();

			bExecutedAtLeastOne = true;
		}
	}

	if (bBeforeDraw && bExecutedAtLeastOne)
	{
		if (bUseFillModeWireframe)
		{
			pCommandList->SetPipelineState(pOpaqueWireframePSO.Get());
		}
		else
		{
			pCommandList->SetPipelineState(pOpaquePSO.Get());
		}
	}
}

void SApplication::executeCustomComputeShader(SComputeShader* pComputeShader)
{
	pCommandList->SetComputeRootSignature(pComputeShader->pComputeRootSignature.Get());
	pCommandList->SetPipelineState(pComputeShader->pComputePSO.Get());

	for (size_t i = 0; i < pComputeShader->vShaderResources.size(); i++)
	{
		if (pComputeShader->vShaderResources[i]->bIsUAV)
		{
			pCommandList->SetComputeRootUnorderedAccessView(static_cast<UINT>(i),
				pComputeShader->vShaderResources[i]->pResource->GetGPUVirtualAddress());
		}
		else
		{
			pCommandList->SetComputeRootShaderResourceView(static_cast<UINT>(i),
				pComputeShader->vShaderResources[i]->pResource->GetGPUVirtualAddress());
		}
	}

	for (size_t i = 0; i < pComputeShader->vUsedRootIndex.size(); i++)
	{
		std::vector<float> vValuesToCopy;
		UINT iUsedConstantsOnThisRootIndex = 0;

		for (size_t j = 0; j < pComputeShader->v32BitConstants.size(); j++)
		{
			if (pComputeShader->v32BitConstants[j].iRootParamIndex == pComputeShader->vUsedRootIndex[i])
			{
				iUsedConstantsOnThisRootIndex++;
				vValuesToCopy.push_back(pComputeShader->v32BitConstants[j]._32BitConstant);
			}
		}

		pCommandList->SetComputeRoot32BitConstants(pComputeShader->vUsedRootIndex[i], static_cast<UINT>(vValuesToCopy.size()), vValuesToCopy.data(), 0);
	}

	pCommandList->Dispatch(pComputeShader->iThreadGroupCountX, pComputeShader->iThreadGroupCountY, pComputeShader->iThreadGroupCountZ);
}

void SApplication::doOptionalPauseForUserComputeShaders()
{
	bool bAtLeastOneShaderNeedsToWait = false;

	for (size_t i = 0; i < vUserComputeShaders.size(); i++)
	{
		if (vUserComputeShaders[i]->bWaitForComputeShaderRightAfterDraw && vUserComputeShaders[i]->bWaitForComputeShaderToFinish &&
			vUserComputeShaders[i]->bExecuteShader)
		{
			bAtLeastOneShaderNeedsToWait = true;

			break;
		}
	}

	if (bAtLeastOneShaderNeedsToWait)
	{
		for (size_t i = 0; i < vUserComputeShaders.size(); i++)
		{
			if (vUserComputeShaders[i]->bWaitForComputeShaderRightAfterDraw && vUserComputeShaders[i]->bWaitForComputeShaderToFinish &&
				vUserComputeShaders[i]->bExecuteShader)
			{
				copyUserComputeResults(vUserComputeShaders[i]);
			}
		}
	}
	else
	{
		// Remember current fence and wait for it to finish.

		for (size_t i = 0; i < vUserComputeShaders.size(); i++)
		{
			if (vUserComputeShaders[i]->bWaitForComputeShaderRightAfterDraw == false && vUserComputeShaders[i]->bWaitForComputeShaderToFinish &&
				vUserComputeShaders[i]->bExecuteShader)
			{
				vUserComputeShaders[i]->mtxFencesVector.lock();

				if (vUserComputeShaders[i]->vFinishFences.size() == 0)
				{
					mtxFenceUpdate.lock();

					iCurrentFence++;

					HRESULT hresult = pCommandQueue->Signal(pFence.Get(), iCurrentFence);

					vUserComputeShaders[i]->vFinishFences.push_back(iCurrentFence);

					mtxFenceUpdate.unlock();

					if (FAILED(hresult))
					{
						SError::showErrorMessageBoxAndLog(hresult);
						vUserComputeShaders[i]->mtxFencesVector.unlock();
						continue;
					}

					vUserComputeShaders[i]->mtxFencesVector.unlock();
				}
				else
				{
					// Wait until the GPU has completed commands up to this fence point.
					if (pFence->GetCompletedValue() >= vUserComputeShaders[i]->vFinishFences[0])
					{
						// Finished.
						vUserComputeShaders[i]->mtxFencesVector.unlock();
						// unlock because user can call stopShaderExecution() inside of the function below, and
						// this will cause deadlock (see stopShaderExecution()).
						copyUserComputeResults(vUserComputeShaders[i]);

						vUserComputeShaders[i]->mtxFencesVector.lock();
						if (vUserComputeShaders[i]->vFinishFences.size() > 0)
						{
							vUserComputeShaders[i]->vFinishFences.erase(vUserComputeShaders[i]->vFinishFences.begin());
						}
						vUserComputeShaders[i]->mtxFencesVector.unlock();
					}
					else
					{
						vUserComputeShaders[i]->mtxFencesVector.unlock();
						continue;
					}
				}
			}
		}
	}
}

void SApplication::copyUserComputeResults(SComputeShader* pComputeShader)
{
	std::vector<char*> vDataPointers(pComputeShader->vResourceNamesToCopyFrom.size());
	std::vector<size_t> vDataSizes(pComputeShader->vResourceNamesToCopyFrom.size());

	for (size_t i = 0; i < pComputeShader->vResourceNamesToCopyFrom.size(); i++)
	{
		SComputeShaderResource* pResourceToCopyFrom = nullptr;

		// Find resource.
		for (size_t k = 0; k < pComputeShader->vShaderResources.size(); k++)
		{
			if (pComputeShader->vShaderResources[k]->sResourceName == pComputeShader->vResourceNamesToCopyFrom[i])
			{
				pResourceToCopyFrom = pComputeShader->vShaderResources[k];

				break;
			}
		}

		if (pResourceToCopyFrom)
		{
			Microsoft::WRL::ComPtr<ID3D12Resource> pReadBackBuffer;
			CD3DX12_RESOURCE_DESC buf = CD3DX12_RESOURCE_DESC::Buffer(pResourceToCopyFrom->iDataSizeInBytes);

			CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_READBACK);

			HRESULT hresult = pDevice->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&buf,
				D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&pReadBackBuffer));
			if (FAILED(hresult))
			{
				SError::showErrorMessageBoxAndLog(hresult);
				return;
			}


			resetCommandList();

			CD3DX12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(pResourceToCopyFrom->pResource.Get(),
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
			pCommandList->ResourceBarrier(1, &transition);


			pCommandList->CopyResource(pReadBackBuffer.Get(), pResourceToCopyFrom->pResource.Get());

			transition = CD3DX12_RESOURCE_BARRIER::Transition(pResourceToCopyFrom->pResource.Get(),
				D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			pCommandList->ResourceBarrier(1, &transition);

			executeCommandList();
			flushCommandQueue();



			D3D12_RANGE readbackBufferRange{ 0, pResourceToCopyFrom->iDataSizeInBytes };
			char* pCopiedData = new char[pResourceToCopyFrom->iDataSizeInBytes];

			char* pMappedData = nullptr;

			pReadBackBuffer->Map(0, &readbackBufferRange, reinterpret_cast<void**>(&pMappedData));

			std::memcpy(pCopiedData, pMappedData, pResourceToCopyFrom->iDataSizeInBytes);

			pReadBackBuffer->Unmap(0, nullptr);


			pReadBackBuffer->Release();

			vDataPointers[i] = pCopiedData;
			vDataSizes[i] = pResourceToCopyFrom->iDataSizeInBytes;
		}
		else
		{
			SError::showErrorMessageBoxAndLog("pResourceToCopyFrom is nullptr, could not find the specified resource.");
			return;
		}
	}

	pComputeShader->finishedCopyingComputeResults(vDataPointers, vDataSizes);
}

bool SApplication::doesComponentExists(SComponent* pComponent)
{
	std::lock_guard<std::mutex> guard(mtxDraw);

	for (size_t i = 0; i < vAllRenderableSpawnedOpaqueComponents.size(); i++)
	{
		if (vAllRenderableSpawnedOpaqueComponents[i] == pComponent)
		{
			return true;
		}
	}

	for (size_t i = 0; i < vAllRenderableSpawnedTransparentComponents.size(); i++)
	{
		if (vAllRenderableSpawnedTransparentComponents[i] == pComponent)
		{
			return true;
		}
	}

	return false;
}

bool SApplication::doesComputeShaderExists(SComputeShader* pShader)
{
	std::lock_guard<std::mutex> guard(mtxDraw);

	for (size_t i = 0; i < vUserComputeShaders.size(); i++)
	{
		if (vUserComputeShaders[i] == pShader)
		{
			return true;
		}
	}

	return false;
}

bool SApplication::nanosleep(long long ns)
{
	ns /= 100;

	HANDLE timer;
	LARGE_INTEGER li;

	if (!(timer = CreateWaitableTimer(NULL, TRUE, NULL)))
	{
		return true;
	}
		

	li.QuadPart = -ns;
	if (!SetWaitableTimer(timer, &li, 0, NULL, NULL, FALSE))
	{
		CloseHandle(timer);
		return true;
	}

	WaitForSingleObject(timer, INFINITE);

	CloseHandle(timer);

	return false;
}

std::vector<SUploadBuffer<SMaterialConstants>*> SApplication::createBundledMaterialResource(SShader* pShader, size_t iMaterialsCount)
{
	std::vector<SUploadBuffer<SMaterialConstants>*> vResources;

	for (size_t i = 0; i < vFrameResources.size(); i++)
	{
		vResources.push_back(vFrameResources[i]->addNewMaterialBundleResource(pShader, iMaterialsCount));
	}

	return vResources;
}

SMaterial* SApplication::registerMaterialBundleElement(const std::string& sMaterialName, bool& bErrorOccurred)
{
	bErrorOccurred = false;

	if (sMaterialName == "")
	{
		SError::showErrorMessageBoxAndLog("material name cannot be empty.");

		bErrorOccurred = true;

		return nullptr;
	}

	SMaterial* pMat = new SMaterial();
	pMat->sMaterialName = sMaterialName;
	pMat->bRegistered = true;
	pMat->bUsedInBundle = true;
	pMat->iUpdateCBInFrameResourceCount = SFRAME_RES_COUNT;

	return pMat;
}

void SApplication::setTransparentPSO()
{
	if (bUseFillModeWireframe)
	{
		pCommandList->SetPipelineState(pTransparentWireframePSO.Get());
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
}

bool SApplication::doFrustumCulling(SComponent* pComponent)
{
	pComponent->mtxWorldMatrixUpdate.lock();
	DirectX::XMMATRIX world    = DirectX::XMLoadFloat4x4(&pComponent->renderData.vWorld);
	pComponent->mtxWorldMatrixUpdate.unlock();

	DirectX::XMVECTOR worldDet = XMMatrixDeterminant(world);

	DirectX::XMMATRIX invWorld = DirectX::XMMatrixInverse(&worldDet, world);

	DirectX::XMMATRIX view = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&mainRenderPassCB.vView)); // transpose back (see updateMainPassCB()).
	DirectX::XMVECTOR viewDet = XMMatrixDeterminant(view);
	DirectX::XMMATRIX invView = DirectX::XMMatrixInverse(&viewDet, view);

	// View space to the object's local space.
	DirectX::XMMATRIX viewToObjectLocal = XMMatrixMultiply(invView, invWorld);

	// Transform the camera frustum from view space to the object's local space.
	DirectX::BoundingFrustum localSpaceFrustum;
	cameraBoundingFrustumOnLastMainPassUpdate.Transform(localSpaceFrustum, viewToObjectLocal);

	// Perform the box/frustum intersection test in local space.
	if (localSpaceFrustum.Contains(pComponent->boxCollision) != DirectX::DISJOINT)
	{
		// Draw this component.
		return true;
	}

	return false;
}

void SApplication::doFrustumCullingOnInstancedMesh(SMeshComponent* pMeshComponent, UINT64& iOutVisibleInstanceCount)
{
	std::lock_guard<std::mutex> lock(pMeshComponent->mtxInstancing);


	pMeshComponent->mtxWorldMatrixUpdate.lock();
	DirectX::XMMATRIX componentWorld = DirectX::XMLoadFloat4x4(&pMeshComponent->renderData.vWorld);
	pMeshComponent->mtxWorldMatrixUpdate.unlock();

	UINT64 iVisibleInstanceCount = 0;

	float fCullDistance = pMeshComponent->fCullDistance;

	for (size_t i = 0; i < pMeshComponent->vInstanceData.size(); i++)
	{
		DirectX::XMMATRIX instanceWorld =
			// because instance world is relative to the component's world
			DirectX::XMMatrixMultiply(DirectX::XMLoadFloat4x4(&pMeshComponent->vInstanceData[i].vWorld), componentWorld);


		DirectX::XMFLOAT4X4 vInstanceWorldFloat4x4;
		DirectX::XMStoreFloat4x4(&vInstanceWorldFloat4x4, instanceWorld);
		SVector vInstanceLocationInWorld(vInstanceWorldFloat4x4._41, vInstanceWorldFloat4x4._42, vInstanceWorldFloat4x4._43);

		if ((vInstanceLocationInWorld - camera.getCameraLocationInWorld()).length() >= fCullDistance)
		{
			continue;
		}


		DirectX::XMVECTOR instWorldDet = XMMatrixDeterminant(instanceWorld);

		DirectX::XMMATRIX invWorld = DirectX::XMMatrixInverse(&instWorldDet, instanceWorld);

		DirectX::XMMATRIX view = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&mainRenderPassCB.vView)); // transpose back (see updateMainPassCB()).
		DirectX::XMVECTOR viewDet = XMMatrixDeterminant(view);
		DirectX::XMMATRIX invView = DirectX::XMMatrixInverse(&viewDet, view);

		// View space to the object's local space.
		DirectX::XMMATRIX viewToObjectLocal = XMMatrixMultiply(invView, invWorld);

		// Transform the camera frustum from view space to the object's local space.
		DirectX::BoundingFrustum localSpaceFrustum;
		cameraBoundingFrustumOnLastMainPassUpdate.Transform(localSpaceFrustum, viewToObjectLocal);

		// Perform the box/frustum intersection test in local space.
		if (localSpaceFrustum.Contains(pMeshComponent->boxCollision) != DirectX::DISJOINT)
		{
			// Draw this instance.
			pMeshComponent->vFrameResourcesInstancedData[iCurrentFrameResourceIndex]->
				copyDataToElement(iVisibleInstanceCount, pMeshComponent->vInstanceData[i]);

			iVisibleInstanceCount++;
		}
	}

	iOutVisibleInstanceCount = iVisibleInstanceCount;
}

SApplication::SApplication(HINSTANCE hInstance)
{
	hApplicationInstance = hInstance;
	pApp           = this;

	pVideoSettings = new SVideoSettings(this);
	pProfiler      = new SProfiler(this);

	pCurrentLevel  = new SLevel(this);

	ScissorRect = { 0, 0, iMainWindowWidth, iMainWindowHeight };

	ScreenViewport.TopLeftX = 0;
	ScreenViewport.TopLeftY = 0;
	ScreenViewport.Width = static_cast<float>(iMainWindowWidth);
	ScreenViewport.Height = static_cast<float>(iMainWindowHeight);
	ScreenViewport.MinDepth = fMinDepth;
	ScreenViewport.MaxDepth = fMaxDepth;

	vGUILayers.push_back(SGUILayer{ 0, std::vector<SGUIObject*>() });

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

	// Clear loaded textures.

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


	// Clear compiledShaders.

	for (size_t i = 0; i < vCompiledUserShaders.size(); i++)
	{
		releaseShader(vCompiledUserShaders[i]);
	}

	vCompiledUserShaders.clear();


	// Clear compute shaders.

	for (size_t i = 0; i < vUserComputeShaders.size(); i++)
	{
		delete vUserComputeShaders[i];
	}

	vUserComputeShaders.clear();


	// Clear materials.

	for (size_t i = 0; i < vRegisteredMaterials.size(); i++)
	{
		delete vRegisteredMaterials[i];
	}

	vRegisteredMaterials.clear();


	// Clear GUI.

	for (size_t i = 0; i < vGUILayers.size(); i++)
	{
		for (size_t j = 0; j < vGUILayers[i].vGUIObjects.size(); j++)
		{
			delete vGUILayers[i].vGUIObjects[j];
		}
	}

	vGUILayers.clear();


	delete pVideoSettings;
	delete pProfiler;


	if (pAudioEngine)
	{
		delete pAudioEngine;
	}
}

void SApplication::initDisableD3DDebugLayer()
{
	bD3DDebugLayerEnabled = false;
}

void SApplication::initCompileShadersInRelease()
{
	bCompileShadersInRelease = true;
}

bool SApplication::init(const std::wstring& sMainWindowClassName)
{
	this->sMainWindowClassName = sMainWindowClassName;

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

	pDXTKGraphicsMemory = std::make_unique<DirectX::GraphicsMemory>(pDevice.Get());

	bInitCalled = true;

	// Do the initial resize code.
	onResize();

	pBlurEffect = std::make_unique<SBlurEffect>(pDevice.Get(), iMainWindowWidth, iMainWindowHeight, BackBufferFormat);

	HRESULT hresult = pCommandList->Reset(pCommandListAllocator.Get(), nullptr);
	if (FAILED(hresult))
	{
		SError::showErrorMessageBoxAndLog(hresult);
		return true;
	}

	if (createRootSignature())
	{
		return true;
	}

	if (createBlurRootSignature())
	{
		return true;
	}

	if (createShadersAndInputLayout())
	{
		return true;
	}

	createFrameResources();

	if (createCBVSRVUAVHeap())
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
		SError::showErrorMessageBoxAndLog(hresult);
		return true;
	}

	ID3D12CommandList* vCommandListsToExecute[] = { pCommandList.Get() };
	pCommandQueue->ExecuteCommandLists(_countof(vCommandListsToExecute), vCommandListsToExecute);

	

	// Wait for all command to finish.

	if (flushCommandQueue())
	{
		return true;
	}


	// Init audio engine.

	pAudioEngine = new SAudioEngine();
	if (pAudioEngine->init(true))
	{
		return true;
	}

	// Check default font.

	std::ifstream defaultFontFile(sPathToDefaultFont);
	if (defaultFontFile.is_open() == false)
	{
		SError::showErrorMessageBoxAndLog("can't find default engine font at res/default_font.spritefont.");
		return true;
	}

#if defined(DEBUG) || defined(_DEBUG)
	// Init SProfiler GUI.

	if (pProfiler->initNeededGUIObjects())
	{
		return true;
	}
#endif

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

		iWindowCenterX = iMainWindowWidth / 2;
		iWindowCenterY = iMainWindowHeight / 2;

		if (bInitCalled)
		{
			if( wParam == SIZE_MINIMIZED)
			{
				bWindowMaximized = false;
				bWindowMinimized = true;

				onMinimizeEvent();
			}
			else if( wParam == SIZE_MAXIMIZED)
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
	case WM_KILLFOCUS:
	{
		// Lost focus.

		onLoseFocus();

		return 0;
	}
	case WM_SETFOCUS:
	{
		// Gain focus.

		onGainFocus();

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

		if (pressedMouseKey.getButton() != SMouseButton::SMB_NONE)
		{
			mousekey.setOtherKey(wParam, pressedMouseKey);
		}
		else
		{
			mousekey.determineKey(wParam);
			pressedMouseKey.setKey(mousekey.getButton());
		}
		

		onMouseDown(mousekey, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

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

			pressedMouseKey.setKey(SMouseButton::SMB_NONE);
		}
		else
		{
			onMouseUp(keyDownLeft, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		}
		
		return 0;
	}
	case WM_MOUSEMOVE:
	{
		if (bMouseCursorShown == false)
		{
			POINT pos;
			pos.x = iWindowCenterX;
			pos.y = iWindowCenterY;

			ClientToScreen(hMainWindow, &pos);

			SetCursorPos(pos.x, pos.y);
		}

		return 0;
	}
	case WM_INPUT:
	{
		UINT dataSize = 0;

		GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, NULL, &dataSize, sizeof(RAWINPUTHEADER));

		if (dataSize > 0)
		{
			LPBYTE lpb = new BYTE[dataSize];

			if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, lpb, &dataSize, sizeof(RAWINPUTHEADER)) != dataSize)
			{
				SError::showErrorMessageBoxAndLog("incorrect size was returned from GetRawInputData().");
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
			} 


			delete[] lpb;
		}

		// Don't return, because we need to call DefWindowProc to cleanup.
		break;
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

		return 0;
	}
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	{
		// From the winapi docs:
		// "One flag that might be useful is bit 30, the "previous key state" flag, which is set to 1 for repeated key-down messages."
		if ((lParam & (1 << 30)) && bDisableKeyboardRepeat)
		{
			return 0;
		}

		SKeyboardKey key(wParam, lParam);
		if (key.getButton() != SKeyboardButton::SKB_NONE)
		{
			onKeyboardButtonDown(key);
		}

		return 0;
	}
	case WM_KEYUP:
	case WM_SYSKEYUP:
	{
		SKeyboardKey key(wParam, lParam);
		if (key.getButton() != SKeyboardButton::SKB_NONE)
		{
			onKeyboardButtonUp(key);
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
		gamePhysicsTimer.reset();

		bRunCalled = true;

		gamePhysicsTimer.tick();

		promiseFinishedPhysics = std::promise<bool>();
		futureFinishedPhysics = promiseFinishedPhysics.get_future();
		
		std::thread t(&SApplication::internalPhysicsTickThread, this);
		t.detach();


		STimer frameTimer;
		gameTimer.tick();


		update(); // so pCurrentFrameResource will be assigned before onTick()
		draw();

		onRun();


		frameTimer.start();

#if defined(DEBUG) || defined(_DEBUG)
		std::chrono::time_point<std::chrono::steady_clock> timeWindowsMessage;
		bool bTimeWindowsMessageStarted = false;

		std::chrono::time_point<std::chrono::steady_clock> timeUserOnTick;
		std::chrono::time_point<std::chrono::steady_clock> timeOnDraw;
		std::chrono::time_point<std::chrono::steady_clock> timeOnCalcFPS;
		std::chrono::time_point<std::chrono::steady_clock> timeOnAudio;
		double dToMS = 1000000.0;
#endif

		while(msg.message != WM_QUIT)
		{
			// don't use GetMessage() as it puts the thread to sleep until message.
			if(PeekMessage( &msg, 0, 0, 0, PM_REMOVE ))
			{
#if defined(DEBUG) || defined(_DEBUG)

				if (bTimeWindowsMessageStarted == false)
				{
					bTimeWindowsMessageStarted = true;

					timeWindowsMessage = std::chrono::steady_clock::now();
				}

#endif

				TranslateMessage( &msg );
				DispatchMessage( &msg );
			}
			else
			{
#if defined(DEBUG) || defined(_DEBUG)

				if (bTimeWindowsMessageStarted)
				{
					bTimeWindowsMessageStarted = false;

					frameStats.fTimeSpentOnWindowMessagesInMS
						= static_cast<float>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - timeWindowsMessage).count()
							/ dToMS);
				}
				else
				{
					frameStats.fTimeSpentOnWindowMessagesInMS = 0.0f;
				}

#endif

				gameTimer.tick();




#if defined(DEBUG) || defined(_DEBUG)
				timeUserOnTick = std::chrono::steady_clock::now();
#endif
				if (bCallTick)
				{
					onTick(gameTimer.getDeltaTimeBetweenTicksInSec());
				}

#if defined(DEBUG) || defined(_DEBUG)
				frameStats.fTimeSpentOnUserOnTickFunctionInMS
					= static_cast<float>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - timeUserOnTick).count()
						/ dToMS);
#endif




#if defined(DEBUG) || defined(_DEBUG)
				timeOnAudio = std::chrono::steady_clock::now();
#endif
				mtxDraw.lock();
				pAudioEngine->update3DSound(getCamera());
				mtxDraw.unlock();

#if defined(DEBUG) || defined(_DEBUG)
				frameStats.fTimeSpentOn3DAudioUpdateInMS
					= static_cast<float>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - timeOnAudio).count()
						/ dToMS);
#endif


				update();



#if defined(DEBUG) || defined(_DEBUG)
				timeOnDraw = std::chrono::steady_clock::now();
#endif

				draw();

#if defined(DEBUG) || defined(_DEBUG)
				frameStats.fTimeSpentOnCPUDrawInMS
					= static_cast<float>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - timeOnDraw).count()
						/ dToMS);
#endif





#if defined(DEBUG) || defined(_DEBUG)
				timeOnCalcFPS = std::chrono::steady_clock::now();
#endif

				calculateFrameStats();

#if defined(DEBUG) || defined(_DEBUG)
				frameStats.fTimeSpentOnFPSCalcInMS
					= static_cast<float>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - timeOnCalcFPS).count()
						/ dToMS);
#endif




				if (fFPSLimit >= 1.0f)
				{
 					double dTimeToRenderFrameInNS = frameTimer.getElapsedTimeInNS();

					if (dDelayBetweenFramesInNS > dTimeToRenderFrameInNS)
					{
						timeBeginPeriod(1);

						double dTimeInNS = round((dDelayBetweenFramesInNS - dTimeToRenderFrameInNS));
						nanosleep(static_cast<long long>(dTimeInNS));

						timeEndPeriod(1);

#if defined(DEBUG) || defined(_DEBUG)
						frameStats.fTimeSpentInFPSLimitSleepInMS = static_cast<float>(dTimeInNS / dToMS);
#endif
					}

					frameTimer.start();
				}
#if defined(DEBUG) || defined(_DEBUG)
				else
				{
					frameStats.fTimeSpentInFPSLimitSleepInMS = 0.0f;
				}
#endif


#if defined(DEBUG) || defined(_DEBUG)
				pProfiler->setFrameStats(frameStats);
#endif
			}
		}

		bTerminatePhysics = true;
		futureFinishedPhysics.get();

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
		if (pApp->bInitCalled)
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
