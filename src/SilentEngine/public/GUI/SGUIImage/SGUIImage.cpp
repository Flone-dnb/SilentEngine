// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "SGUIImage.h"

// STL
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

// DirectXTK
#include "ResourceUploadBatch.h"
#include "WICTextureLoader.h"
#include "DDSTextureLoader.h"
#include "CommonStates.h"
#include "DirectXHelpers.h"

// Custom
#include "SilentEngine/Public/SApplication/SApplication.h"
#include "SilentEngine/Private/SError/SError.h"

bool SGUIImage::loadImage(std::wstring_view sPathToImage)
{
	std::lock_guard<std::mutex> guard(mtxSprite);

	// See if the file exists.

	std::ifstream textureFile(sPathToImage);

	if (textureFile.is_open() == false)
	{
		SError::showErrorMessageBoxAndLog("the specified file not found.");
		return true;
	}
	else
	{
		textureFile.close();
	}


	// See if the file format is .dds.

	bool bIsDDS = true;
	if (fs::path(sPathToImage).extension().string() != ".dds")
	{
		bIsDDS = false;
	}



	// Upload.

	SApplication* pApp = SApplication::getApp();
	if (pApp == nullptr)
	{
		SError::showErrorMessageBoxAndLog("SApplication instance was not created yet, create one first and only then call this function.");
		return true;
	}

	DirectX::ResourceUploadBatch resourceUpload(pApp->pDevice.Get());

	resourceUpload.Begin();
	HRESULT hresult = S_OK;
	if (bIsDDS)
	{
		hresult = DirectX::CreateDDSTextureFromFile(pApp->pDevice.Get(), resourceUpload, sPathToImage.data(), pTexture.ReleaseAndGetAddressOf());
		if (FAILED(hresult))
		{
			SError::showErrorMessageBoxAndLog(hresult);
			return true;
		}
	}
	else
	{
		hresult = DirectX::CreateWICTextureFromFile(pApp->pDevice.Get(), resourceUpload, sPathToImage.data(), pTexture.ReleaseAndGetAddressOf());
		if (FAILED(hresult))
		{
			SError::showErrorMessageBoxAndLog(hresult);
			return true;
		}
	}


	DirectX::RenderTargetState rtState(pApp->BackBufferFormat, pApp->DepthStencilFormat);
	rtState.sampleDesc.Count = pApp->MSAA_Enabled ? pApp->MSAA_SampleCount : 1;
	rtState.sampleDesc.Quality = pApp->MSAA_Enabled ? (pApp->MSAA_Quality - 1) : 0;
	D3D12_BLEND_DESC* bd = nullptr;
	if (bIsDDS == false)
	{
		bd = const_cast<D3D12_BLEND_DESC*>(&DirectX::CommonStates::NonPremultiplied);
	}
	DirectX::SpriteBatchPipelineStateDescription pd(rtState, bd);

	pSpriteBatch = std::make_unique<DirectX::SpriteBatch>(pApp->pDevice.Get(), resourceUpload, pd);

	DirectX::XMUINT2 texSize = DirectX::GetTextureSize(pTexture.Get());

	if (bIsDDS)
	{
		// Check if the texture size is x4.

		if (texSize.x % 4 != 0 || texSize.y % 4 != 0
			|| texSize.x != texSize.y)
		{
			SError::showErrorMessageBoxAndLog("the .dds texture size should be a multiple of 4.");
			return true;
		}
	}


	auto uploadResourcesFinished = resourceUpload.End(pApp->pCommandQueue.Get());
	uploadResourcesFinished.wait();

	pSpriteBatch->SetViewport(pApp->ScreenViewport);


	sPathToTexture = sPathToImage;

	if (bIsRegistered)
	{
		// create SRVs to new texture
		SApplication::getApp()->refreshHeap();
	}

	return false;
}

void SGUIImage::setViewport(D3D12_VIEWPORT viewport)
{
	if (pSpriteBatch)
	{
		pSpriteBatch->SetViewport(viewport);
	}
}

void SGUIImage::onMSAAChange()
{
	if (pSpriteBatch)
	{
		loadImage(sPathToTexture);
	}
}

bool SGUIImage::checkRequiredResourcesBeforeRegister()
{
	if (pTexture == nullptr)
	{
		SError::showErrorMessageBoxAndLog("an image resource is required to register the SGUIImage object, use loadImage() first.");
		return true;
	}

    return false;
}

SGUIImage::~SGUIImage()
{
}

SGUIImage::SGUIImage(const std::string& sObjectName) : SGUIObject(sObjectName)
{
	objectType = SGUIType::SGT_IMAGE;

	iIndexInHeap = -1;
}
