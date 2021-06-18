// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "SGUISimpleText.h"

// STL
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

// DirectXTK
#include "ResourceUploadBatch.h"
#include "CommonStates.h"
#include "DirectXHelpers.h"

// Custom
#include "SilentEngine/Public/SApplication/SApplication.h"
#include "SilentEngine/Private/SError/SError.h"

SGUISimpleText::SGUISimpleText(const std::string& sObjectName) : SGUIObject(sObjectName)
{
	objectType = SGUIType::SGT_SIMPLE_TEXT;

	sPathToSpriteFont = L"";
	sRawText = L"";
	sWrappedText = L"";

	fMaxLineWidth = 0.0f;

	bDrawOutline = false;
	bAlignTextAtCenter = false;
	bInitFontCalled = false;
	bDrawShadow = false;
}

SGUISimpleText::~SGUISimpleText()
{
}

bool SGUISimpleText::setFont(const std::wstring& sPathToSprintFont)
{
	// See if the texture name is empty.

	if (sPathToSprintFont == L"")
	{
		SError::showErrorMessageBoxAndLog("the specified path is empty.");
		return true;
	}

	// See if the file exists.

	std::ifstream textureFile(sPathToSprintFont);

	if (textureFile.is_open() == false)
	{
		SError::showErrorMessageBoxAndLog("the specified file does not exist.");
		return true;
	}
	else
	{
		textureFile.close();
	}

	// See if the file format is .spritefont

	if (fs::path(sPathToSprintFont).extension().string() != ".spritefont")
	{
		SError::showErrorMessageBoxAndLog("the specified file should have the \".spritefont\" format.");
		return true;
	}

	this->sPathToSpriteFont = sPathToSprintFont;

	if (bIsRegistered)
	{
		SApplication::getApp()->refreshHeap();
		initFontResource();
	}

	return false;
}

void SGUISimpleText::setText(const std::wstring& sText)
{
	this->sRawText = sText;

	if (pSpriteFont)
	{
		sWrappedText = wrapText();
		recalculateSizeToKeepScaling();
	}
}

void SGUISimpleText::setDrawTextOutline(bool bDrawOutline, const SVector& vOutlineColor)
{
	this->bDrawOutline = bDrawOutline;
	this->outlineColor = DirectX::XMFLOAT4(vOutlineColor.getX(), vOutlineColor.getY(), vOutlineColor.getZ(), vOutlineColor.getW());
}

void SGUISimpleText::setDrawTextShadow(bool bDrawTextShadow)
{
	bDrawShadow = bDrawTextShadow;
}

void SGUISimpleText::setWordWrapMaxLineWidth(float fMaxLineWidth, bool bAlignTextAtCenter)
{
	if (fMaxLineWidth < 0.0f || fMaxLineWidth > 1.0f)
	{
		SError::showErrorMessageBoxAndLog("max line width should be in normalized range: [0.0f, 1.0f].");
		return;
	}

	this->fMaxLineWidth = fMaxLineWidth;
	this->bAlignTextAtCenter = bAlignTextAtCenter;

	if (pSpriteFont)
	{
		sWrappedText = wrapText();
	}
}

std::wstring SGUISimpleText::getText() const
{
	return sRawText;
}

SVector SGUISimpleText::getSizeInPixels() const
{
	if (pSpriteFont)
	{
		DirectX::SimpleMath::Vector2 texSize = DirectX::SimpleMath::Vector2(pSpriteFont->MeasureString(sWrappedText.c_str(), false));

		return SVector(texSize.x, texSize.y);
	}
	else
	{
		return SVector();
	}
}

void SGUISimpleText::setViewport(D3D12_VIEWPORT viewport)
{
	if (pSpriteBatch)
	{
		pSpriteBatch->SetViewport(viewport);

		recalculateSizeToKeepScaling();
	}
}

void SGUISimpleText::onMSAAChange()
{
	if (pSpriteBatch)
	{
		SApplication::getApp()->refreshHeap();
		initFontResource();
	}
}

bool SGUISimpleText::checkRequiredResourcesBeforeRegister()
{
	if (sPathToSpriteFont == L"")
	{
		SError::showErrorMessageBoxAndLog("a font is required to register the SGUISimpleText object, use setFont() first.");
		return true;
	}

	return false;
}

void SGUISimpleText::recalculateSizeToKeepScaling()
{
	if (vSizeToKeep.getX() < 0.0f || vSizeToKeep.getY() < 0.0f)
	{
		return;
	}

	if (pSpriteFont == nullptr)
	{
		return;
	}

	float fTargetWidth = vSizeToKeep.getX() * SApplication::getApp()->iMainWindowWidth;
	float fTargetHeight = vSizeToKeep.getY() * SApplication::getApp()->iMainWindowHeight;

	DirectX::SimpleMath::Vector2 texSize = DirectX::SimpleMath::Vector2(pSpriteFont->MeasureString(sWrappedText.c_str(), false));

	texSize.x *= scale.x;
	texSize.y *= scale.y;

	screenScale.x = fTargetWidth / texSize.x;
	screenScale.y = fTargetHeight / texSize.y;
}

std::wstring SGUISimpleText::wrapText()
{
	if (fMaxLineWidth < 0.001f)
	{
		return sRawText;
	}

	std::vector<std::wstring> vWords;

	// Split.
	std::wstring sCurrentWord = L"";
	for (size_t i = 0; i < sRawText.size(); i++)
	{
		if (sRawText[i] == L' ' && sCurrentWord != L"")
		{
			vWords.push_back(sCurrentWord);
			sCurrentWord = L"";
		}
		else
		{
			sCurrentWord += sRawText[i];
		}
	}

	if (sCurrentWord != L"")
	{
		vWords.push_back(sCurrentWord);
	}


	float fCurrentLineWidth = 0.0f;
	float fSpaceWidth = DirectX::SimpleMath::Vector2(pSpriteFont->MeasureString(L" ", false)).x;

	std::wstring sWrappedText = L"";

	SApplication* pApp = SApplication::getApp();
	float fWindowMaxLineWidth = fMaxLineWidth * pApp->ScreenViewport.Width;

	size_t iLastLineStartIndex = 0;

	for (size_t i = 0; i < vWords.size(); i++)
	{
		DirectX::SimpleMath::Vector2 wordSize(pSpriteFont->MeasureString(vWords[i].c_str()));

		if (fCurrentLineWidth + wordSize.x < fWindowMaxLineWidth)
		{
			sWrappedText.append(vWords[i] + L" ");
			fCurrentLineWidth += wordSize.x + fSpaceWidth;
		}
		else
		{
			sWrappedText += L"\n";

			iLastLineStartIndex = sWrappedText.size();

			sWrappedText.append(vWords[i] + L" ");
			fCurrentLineWidth = wordSize.x + fSpaceWidth;
		}
	}

	if (bAlignTextAtCenter)
	{
		float fDeltaInPixels = pApp->ScreenViewport.Width * 0.1f;
		if (fCurrentLineWidth < fWindowMaxLineWidth - fDeltaInPixels)
		{
			float fNeedToAdd = fWindowMaxLineWidth - fCurrentLineWidth;
			size_t iAddCount = fNeedToAdd / fSpaceWidth / 2;

			for (size_t i = 0; i < iAddCount; i++)
			{
				sWrappedText.insert(sWrappedText.begin() + iLastLineStartIndex, L' ');
			}
		}
	}

	return sWrappedText;
}

void SGUISimpleText::initFontResource()
{
	SApplication* pApp = SApplication::getApp();

	DirectX::ResourceUploadBatch resourceUpload(pApp->pDevice.Get());

	resourceUpload.Begin();

	pSpriteFont = std::make_unique<DirectX::SpriteFont>(pApp->pDevice.Get(), resourceUpload, sPathToSpriteFont.c_str(), cpuHandle, gpuHandle);

	DirectX::RenderTargetState rtState(pApp->BackBufferFormat, pApp->DepthStencilFormat);
	rtState.sampleDesc.Count = pApp->MSAA_Enabled ? pApp->MSAA_SampleCount : 1;
	rtState.sampleDesc.Quality = pApp->MSAA_Enabled ? (pApp->MSAA_Quality - 1) : 0;
	DirectX::SpriteBatchPipelineStateDescription pd(rtState);

	pSpriteBatch = std::make_unique<DirectX::SpriteBatch>(pApp->pDevice.Get(), resourceUpload, pd);

	// Upload the resources to the GPU.
	auto uploadResourcesFinished = resourceUpload.End(pApp->pCommandQueue.Get());

	// Wait for the upload thread to terminate
	uploadResourcesFinished.wait();

	pSpriteBatch->SetViewport(pApp->ScreenViewport);

	bInitFontCalled = true;

	sWrappedText = wrapText();

	recalculateSizeToKeepScaling();
}
