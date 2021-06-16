// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "SGUIObject.h"

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

SGUIObject::SGUIObject(const std::string& sObjectName)
{
	this->sObjectName = sObjectName;
	iIndexInHeap = -1;
	iZLayer = 0;
	fRotationInRad = 0.0f;
	scale = DirectX::XMFLOAT2(1.0f, 1.0f);
	color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	pos = DirectX::SimpleMath::Vector2(0.5f, 0.5f);
	origin = DirectX::SimpleMath::Vector2(0.5f, 0.5f);
	sourceRect = { 0.0f, 0.0f, 1.0f, 1.0f };
	effects = DirectX::SpriteEffects::SpriteEffects_None;
	bIsVisible = false;
}

bool SGUIObject::loadImage(std::wstring_view sPathToImage)
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

	return false;
}

void SGUIObject::onResize()
{
	std::lock_guard<std::mutex> guard(mtxSprite);

	if (pSpriteBatch)
	{
		SApplication* pApp = SApplication::getApp();
		pSpriteBatch->SetViewport(pApp->ScreenViewport);
	}
}

void SGUIObject::onMSAAChange()
{
	std::lock_guard<std::mutex> guard(mtxSprite);

	if (pSpriteBatch)
	{
		loadImage(sPathToTexture);
	}
}

void SGUIObject::setVisible(bool bIsVisible)
{
	this->bIsVisible = bIsVisible;
}

void SGUIObject::setPosition(const SVector& vPos)
{
	if (vPos.getX() < 0.0f || vPos.getX() > 1.0f || vPos.getY() < 0.0f || vPos.getY() > 1.0f)
	{
		SError::showErrorMessageBoxAndLog("position values should be normalized.");
		return;
	}

	pos = DirectX::SimpleMath::Vector2(vPos.getX(), vPos.getY());
}

void SGUIObject::setRotation(float fRotationInDeg)
{
	this->fRotationInRad = DirectX::XMConvertToRadians(fRotationInDeg);
}

void SGUIObject::setScale(const SVector& vScale)
{
	scale = DirectX::XMFLOAT2(vScale.getX(), vScale.getY());
}

void SGUIObject::setTint(const SVector& vColor)
{
	color = DirectX::XMFLOAT4(vColor.getX(), vColor.getY(), vColor.getZ(), vColor.getW());
}

void SGUIObject::setFlip(bool bFlipHorizontally, bool bFlipVertically)
{
	if (bFlipHorizontally && bFlipVertically)
	{
		effects = DirectX::SpriteEffects::SpriteEffects_FlipBoth;
	}
	else if (bFlipHorizontally)
	{
		effects = DirectX::SpriteEffects::SpriteEffects_FlipHorizontally;
	}
	else if (bFlipVertically)
	{
		effects = DirectX::SpriteEffects::SpriteEffects_FlipVertically;
	}
	else
	{
		effects = DirectX::SpriteEffects::SpriteEffects_None;
	}
}

void SGUIObject::setCut(const SVector& vSourceRect)
{
	if (vSourceRect.getX() < 0.0f || vSourceRect.getX() > 1.0f || vSourceRect.getY() < 0.0f || vSourceRect.getY() > 1.0f ||
		vSourceRect.getZ() < 0.0f || vSourceRect.getZ() > 1.0f || vSourceRect.getW() < 0.0f || vSourceRect.getW() > 1.0f)
	{
		SError::showErrorMessageBoxAndLog("cut values should be normalized.");
		return;
	}

	sourceRect = vSourceRect;
}

void SGUIObject::setCustomOrigin(const SVector& vOrigin)
{
	if (vOrigin.getX() < 0.0f || vOrigin.getX() > 1.0f || vOrigin.getY() < 0.0f || vOrigin.getY() > 1.0f)
	{
		SError::showErrorMessageBoxAndLog("origin values should be normalized.");
		return;
	}

	origin = DirectX::SimpleMath::Vector2(vOrigin.getX(), vOrigin.getY());
}

void SGUIObject::setZLayer(int iZLayer)
{
	if (this->iZLayer == iZLayer)
	{
		return;
	}

	SApplication::getApp()->moveGUIObjectToLayer(this, iZLayer);
}

bool SGUIObject::isVisible() const
{
	return bIsVisible;
}

SVector SGUIObject::getPosition() const
{
	return SVector(pos.x, pos.y);
}

float SGUIObject::getRotation() const
{
	return DirectX::XMConvertToDegrees(fRotationInRad);
}

SVector SGUIObject::getScaling() const
{
	return SVector(scale.x, scale.y);
}

SVector SGUIObject::getTint() const
{
	return SVector(color.x, color.y, color.z, color.w);
}

SVector SGUIObject::getOrigin() const
{
	return SVector(origin.x, origin.y);
}

void SGUIObject::getFlip(bool& bFlippedHorizontally, bool& bFlippedVertically) const
{
	if (effects == DirectX::SpriteEffects::SpriteEffects_None)
	{
		bFlippedHorizontally = false;
		bFlippedVertically = false;
	}
	else if (effects == DirectX::SpriteEffects::SpriteEffects_FlipBoth)
	{
		bFlippedHorizontally = true;
		bFlippedVertically = true;
	}
	else if (effects == DirectX::SpriteEffects::SpriteEffects_FlipHorizontally)
	{
		bFlippedHorizontally = true;
		bFlippedVertically = false;
	}
	else if (effects == DirectX::SpriteEffects::SpriteEffects_FlipVertically)
	{
		bFlippedHorizontally = false;
		bFlippedVertically = true;
	}
}

int SGUIObject::getZLayer() const
{
	return iZLayer;
}

SGUIObject::~SGUIObject()
{
}