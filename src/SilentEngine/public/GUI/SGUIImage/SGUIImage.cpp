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

	recalculateSizeToKeepScaling();

	sPathToTexture = sPathToImage;

	if (bIsRegistered)
	{
		// create SRVs to new texture
		SApplication::getApp()->refreshHeap();
	}

	return false;
}

void SGUIImage::setCustomOrigin(const SVector& vOrigin)
{
	if (bIsInteractable)
	{
		SError::showErrorMessageBoxAndLog("custom origin is disabled for interactable images.");
	}
	else
	{
		SGUIObject::setCustomOrigin(vOrigin);
	}
}

void SGUIImage::setInteractableEvents(std::function<void(SGUIImage*)> noFocus, std::function<void(SGUIImage*)> onHover, std::function<void(SGUIImage*)> onPressed)
{
	if (bIsInteractable == false)
	{
		SError::showErrorMessageBoxAndLog("setInteractableEvents() called for a non interactable image (see SGUIImage constructor).");
	}

	this->noFocus = noFocus;
	this->onHover = onHover;
	this->onPressed = onPressed;
}

void SGUIImage::setCut(const SVector& vSourceRect)
{
	if (vSourceRect.getX() < 0.0f || vSourceRect.getX() > 1.0f || vSourceRect.getY() < 0.0f || vSourceRect.getY() > 1.0f ||
		vSourceRect.getZ() < 0.0f || vSourceRect.getZ() > 1.0f || vSourceRect.getW() < 0.0f || vSourceRect.getW() > 1.0f)
	{
		SError::showErrorMessageBoxAndLog("cut values should be normalized.");
		return;
	}

	sourceRect = vSourceRect;
}

void SGUIImage::setRotation(float fRotationInDeg)
{
	this->fRotationInRad = DirectX::XMConvertToRadians(fRotationInDeg);
}

float SGUIImage::getRotation() const
{
	return DirectX::XMConvertToDegrees(fRotationInRad);
}

SVector SGUIImage::getSizeInPixels()
{
	if (pTexture)
	{
		DirectX::XMUINT2 texSize = DirectX::GetTextureSize(pTexture.Get());

		return SVector(static_cast<float>(texSize.x), static_cast<float>(texSize.y));
	}
	else
	{
		return SVector();
	}
}

void SGUIImage::setViewport(D3D12_VIEWPORT viewport)
{
	std::lock_guard<std::mutex> guard(mtxSprite);

	if (pSpriteBatch)
	{
		pSpriteBatch->SetViewport(viewport);
		
		recalculateSizeToKeepScaling();
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
	std::lock_guard<std::mutex> guard(mtxSprite);

	if (pTexture == nullptr)
	{
		SError::showErrorMessageBoxAndLog("an image resource is required to register the SGUIImage object, use loadImage() first.");
		return true;
	}

	recalculateSizeToKeepScaling();

    return false;
}

void SGUIImage::recalculateSizeToKeepScaling()
{
	if (vSizeToKeep.getX() < 0.0f || vSizeToKeep.getY() < 0.0f)
	{
		return;
	}

	if (pTexture == nullptr)
	{
		return;
	}

	float fTargetWidth = vSizeToKeep.getX() * SApplication::getApp()->iMainWindowWidth;
	float fTargetHeight = vSizeToKeep.getY() * SApplication::getApp()->iMainWindowHeight;

	DirectX::XMUINT2 texSize = DirectX::GetTextureSize(pTexture.Get());

	texSize.x *= static_cast<uint32_t>(scale.x);
	texSize.y *= static_cast<uint32_t>(scale.y);

	screenScale.x = fTargetWidth / texSize.x;
	screenScale.y = fTargetHeight / texSize.y;
}

SVector SGUIImage::getFullSizeInPixels()
{
	std::lock_guard<std::mutex> guard(mtxSprite);

	DirectX::XMUINT2 texSize = DirectX::GetTextureSize(pTexture.Get());

	texSize.x *= static_cast<uint32_t>(scale.x);
	texSize.y *= static_cast<uint32_t>(scale.y);

	return SVector(static_cast<float>(texSize.x), static_cast<float>(texSize.y));
}

SGUIImage::~SGUIImage()
{
}

SGUIImage::SGUIImage(const std::string& sObjectName, bool bInteractable) : SGUIObject(sObjectName)
{
	objectType = SGUIType::SGT_IMAGE;

	iIndexInHeap = -1;
	sourceRect = { 0.0f, 0.0f, 1.0f, 1.0f };

	bIsInteractable = bInteractable;
}
